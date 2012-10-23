/* 
 * hCraft - A custom Minecraft server.
 * Copyright (C) 2012	Jacob Zhitomirsky
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "player.hpp"
#include "server.hpp"
#include "stringutils.hpp"
#include "wordwrap.hpp"
#include "commands/command.hpp"
#include "sql.hpp"
#include "utils.hpp"

#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <cstring>
#include <event2/buffer.h>
#include <sstream>
#include <set>
#include <functional>
#include <cmath>
#include <iomanip>
#include <cstdlib>


namespace hCraft {
	
	/* 
	 * Constructs a new player around the given socket.
	 */
	player::player (server &srv, struct event_base *evbase, evutil_socket_t sock,
		const char *ip)
		: srv (srv), log (srv.get_logger ()), sock (sock),
			entity (srv.next_entity_id ())
	{
		std::strcpy (this->ip, ip);
		
		this->username[0] = '@';
		std::strcpy (this->username + 1, this->ip);
		std::strcpy (this->nick, this->username);
		std::strcpy (this->colored_username, this->username);
		std::strcpy (this->colored_nick, this->nick);
		
		this->logged_in = false;
		this->fail = false;
		this->kicked = false;
		this->handshake = false;
		
		this->total_read = 0;
		this->read_rem = 1;
		
		this->curr_world = nullptr;
		this->curr_chunk = chunk_pos (0, 0);
		this->ping_waiting = false;
		
		this->last_ping = std::chrono::system_clock::now ();
		
		this->evbase = evbase;
		this->bufev  = bufferevent_socket_new (evbase, sock, 0);
		if (!this->bufev)
			{ this->fail = true; return; }
		
		// set timeouts
		{
			struct timeval read_tv, write_tv;
			
			read_tv.tv_sec = 10;
			read_tv.tv_usec = 0;
			
			write_tv.tv_sec = 10;
			write_tv.tv_usec = 0;
			
			bufferevent_set_timeouts (this->bufev, &read_tv, &write_tv);
		}
		
		bufferevent_setcb (this->bufev, &hCraft::player::handle_read,
			&hCraft::player::handle_write, &hCraft::player::handle_event, this);
		bufferevent_enable (this->bufev, EV_READ | EV_WRITE);
	}
	
	/* 
	 * Class destructor.
	 */
	player::~player ()
	{
		evutil_closesocket (this->sock);
		
		{
			std::lock_guard<std::mutex> guard {this->out_lock};
			while (!this->out_queue.empty ())
				{
					packet *top = this->out_queue.front ();
					delete top;
					this->out_queue.pop ();
				}
		}
	}
	
	
	
//----
	/* 
	 * libevent callback functions:
	 */
	
	void
	player::handle_read (struct bufferevent *bufev, void *ctx)
	{
		struct evbuffer *buf = bufferevent_get_input (bufev);
		player *pl = static_cast<player *> (ctx);
		size_t buf_size;
		int n;
		
		if (pl->bad ())	return;
		
		while ((buf_size = evbuffer_get_length (buf)) > 0)
			{
				n = evbuffer_remove (buf, pl->rdbuf + pl->total_read, pl->read_rem);
				if (n <= 0)
					{ pl->disconnect (); return; }
				
				// a small check...
				if (!pl->handshake && (pl->total_read == 0))
					{
						if (pl->rdbuf[0] != 0x02 && pl->rdbuf[0] != 0xFE)
							{
								pl->log (LT_WARNING) << "Expected handshake from @" << pl->get_ip () << std::endl;
								pl->disconnect ();
								return;
							}
					}
				
				pl->total_read += n;
				pl->read_rem = packet::remaining (pl->rdbuf, pl->total_read);
				if (pl->read_rem == -1)
					{
						pl->log (LT_WARNING) << "Received an invalid packet from @"
							<< pl->get_ip () << " (opcode: " << std::hex << std::setfill ('0')
							<< std::setw (2) << (pl->rdbuf[0] & 0xFF) << ")" << std::setfill (' ')
							<< std::endl;
						pl->disconnect ();
						return;
					}
				else if (pl->read_rem == 0)
					{
						/* finished reading packet */
						unsigned char *data = new unsigned char [pl->total_read];
						std::memcpy (data, pl->rdbuf, pl->total_read);
						pl->get_server ().get_thread_pool ().enqueue (
							[pl] (void *ctx)
								{
									std::unique_ptr<unsigned char[]> data (static_cast<unsigned char *> (ctx));
									
									try
										{
											pl->handle (data.get ());
										}
									catch (const std::exception& ex)
										{
											pl->log (LT_ERROR) << "Exception: " << ex.what () << std::endl;
											pl->disconnect ();
										}
								}, data);
						
						pl->total_read = 0;
						pl->read_rem = 1;
					}
			}
	}
	
	void
	player::handle_write (struct bufferevent *bufev, void *ctx)
	{
		player *pl = static_cast<player *> (ctx);
		if (pl->bad ()) return;
		
		int opcode;
		
		std::lock_guard<std::mutex> guard {pl->out_lock};
		if (!pl->out_queue.empty ())
			{
				// dispose of the packet that we just completed sending.
				packet *pack = pl->out_queue.front ();
				pl->out_queue.pop ();
				opcode = pack->data[0];
				delete pack;
				
				if (pl->kicked && (opcode == 0xFF))
					{
						if (pl->kick_msg[0] == '\0')
							pl->log () << pl->get_username () << " has been kicked." << std::endl;
						else
							pl->log () << pl->get_username () << " has been kicked: " << pl->kick_msg << std::endl;	
						
						pl->disconnect (true);
						return;
					}
				
				// if the queue has more packets, send the next one.
				if (!pl->out_queue.empty ())
					{
						packet *pack = pl->out_queue.front ();
						bufferevent_write (bufev, pack->data, pack->size);
					}
			}
	}
	
	void
	player::handle_event (struct bufferevent *bufev, short events, void *ctx)
	{
		player *pl = static_cast<player *> (ctx);
		if (pl->bad ()) return;
		
		if ((events & BEV_EVENT_ERROR) || (events & BEV_EVENT_TIMEOUT) ||
				(events & BEV_EVENT_EOF))
			{
				pl->disconnect ();
				return;
			}
	}
	
	
	
//----
	
	/* 
	 * Marks the player invalid, forcing the server that spawned the player to
	 * eventually destroy it.
	 */
	void
	player::disconnect (bool silent)
	{
		if (this->bad ()) return;
		this->fail = true;
		
		if (!silent)
			log () << this->get_username () << " has disconnected." << std::endl;
			
		this->get_server ().get_players ().remove (this);
		if (this->curr_world)
			{
				this->curr_world->get_players ().remove (this);
				if (this->handshake && !silent)
					{
						std::ostringstream ss;
						ss << "§e[§c-§e] §" << this->get_rank ().main_group->get_color () << this->get_username ()
							<< " §ehas left the server";
						this->get_server ().get_players ().message_nowrap (ss.str ());
					}
				
				chunk *curr_chunk = this->curr_world->get_chunk (
					this->curr_chunk.x, this->curr_chunk.z);
				if (curr_chunk)
					curr_chunk->remove_entity (this);
				
				// despawn from other players.
				std::lock_guard<std::mutex> guard {this->visible_player_lock};
				for (player *pl : this->visible_players)
					{
						this->despawn_from (pl);
					}
			}
	}
	
	/* 
	 * Kicks the player with the given message.
	 */
	void
	player::kick (const char *msg, const char *log_msg)
	{
		if (log_msg)
			std::strcpy (this->kick_msg, log_msg);
		else
			std::strcpy (this->kick_msg, msg);
		
		this->kicked = true;
		this->send (packet::make_kick (msg));
	}
	
	
	
	/* 
	 * Inserts the specified packet into the player's queue of outgoing packets.
	 */
	void
	player::send (packet *pack)
	{
		if (this->bad ())
			{ delete pack; return; }
		
		std::lock_guard<std::mutex> guard {this->out_lock};
		this->out_queue.push (pack);
		if (this->out_queue.size () == 1)
			{
				// initiate write
				bufferevent_write (this->bufev, pack->data, pack->size);
			}
	}
	
	
	
	/* 
	 * Sends the player to the given world.
	 */
	void
	player::join_world (world* w)
	{
		this->join_world_at (w, w->get_spawn ());
	}
	
	
	/* 
	 * Sends the player to the given world at the specified location.
	 */
	void
	player::join_world_at (world *w, entity_pos destpos)
	{
		std::lock_guard<std::mutex> guard {this->join_lock};
		bool had_prev_world = (this->curr_world != nullptr);
		
		/* 
		 * Unload chunks from previous world.
		 */
		if (this->curr_world)
			{
				// we first stop the player from moving.
				entity_pos curr_pos = this->get_pos ();
				this->send (packet::make_player_pos_and_look (
					curr_pos.x, curr_pos.y, curr_pos.z, curr_pos.y + 1.65, curr_pos.r,
					curr_pos.l, true));
				this->set_pos (curr_pos);
				
				// despawn self from other players (and vice-versa).
				player *me = this;
				this->curr_world->get_players ().remove (this);
				this->curr_world->get_players ().all (
					[me] (player *pl)
						{
							if (me->visible_to (pl))
								{
									me->despawn_from (pl);
									pl->despawn_from (me);
								}
						});
				
				// this ensures smooth transitions between worlds:
				this->stream_common_chunks (w, destpos);
			}
		
		this->curr_world = w;
		this->set_pos (destpos);
		this->curr_world->get_players ().add (this);
		this->stream_chunks ();
		
		entity_pos epos = this->get_pos ();
		block_pos bpos = epos;
		
		this->send (packet::make_spawn_pos (bpos.x, bpos.y, bpos.z));
		if (!had_prev_world)
			this->send (packet::make_player_pos_and_look (
				epos.x, epos.y, epos.z, epos.y + 1.65, epos.r, epos.l, true));
			
		log () << this->get_username () << " joined world \"" << w->get_name () << "\"" << std::endl;
	}
	
	
	
//--
	class chunk_pos_less
	{
		chunk_pos origin;
		
	private:
		double
		distance (const chunk_pos c1, const chunk_pos c2) const
		{
			double xd = (double)c1.x - (double)c2.x;
			double zd = (double)c1.z - (double)c2.z;
			return std::sqrt (xd * xd + zd * zd);
		}
		
	public:
		chunk_pos_less (chunk_pos origin)
			: origin (origin)
			{ }
		
		// checks whether lhs < rhs.
		bool
		operator() (const chunk_pos lhs, const chunk_pos rhs) const
		{
			return distance (lhs, origin) < distance (rhs, origin);
		}
	};
	
	/* 
	 * Loads new close chunks to the player and unloads those that are too
	 * far away.
	 */
	void
	player::stream_chunks (int radius)
	{
		std::lock_guard<std::mutex> wguard {this->world_lock};
		std::multiset<chunk_pos, chunk_pos_less> to_load {chunk_pos_less (this->get_pos ())};
		auto prev_chunks = this->known_chunks;
		
		chunk_pos center = this->get_pos ();
		int r_half = radius / 2;
		for (int cx = (center.x - r_half); cx <= (center.x + r_half); ++cx)
			for (int cz = (center.z - r_half); cz <= (center.z + r_half); ++cz)
				{
					chunk_pos cpos = chunk_pos (cx, cz);
					if (this->known_chunks.count (cpos) == 0)
						{
							to_load.insert (cpos);
						}
					prev_chunks.erase (cpos);
				}
		
		for (auto cpos : to_load)
			{
				this->known_chunks.insert (cpos);
				chunk *ch = this->get_world ()->load_chunk (cpos.x, cpos.z);
				this->send (packet::make_chunk (cpos.x, cpos.z, ch));
				
				// spawn self to other players and vice-versa.
				player *me = this;
				ch->all_entities (
					[me] (entity *e)
						{
							if (e->get_type () == ET_PLAYER)
								{
									player* pl = dynamic_cast<player *> (e);
									if (pl == me) return;
									
									me->spawn_to (pl);
									pl->spawn_to (me);
								}
						});
			}
		to_load.clear ();
		
		for (auto cpos : prev_chunks)
			{
				this->known_chunks.erase (cpos);
				this->send (packet::make_empty_chunk (cpos.x, cpos.z));
				
				// despawn self from other players and vice-versa.
				chunk *ch = this->get_world ()->load_chunk (cpos.x, cpos.z);
				
				player *me = this;
				ch->all_entities (
					[me] (entity *e)
						{
							if (e->get_type () == ET_PLAYER)
								{
									player *pl = dynamic_cast<player *> (e);
									if (pl == me) return;
									
									me->despawn_from (pl);
									pl->despawn_from (me);
								}
						});
			}
		prev_chunks.clear ();
		
		chunk *prev_chunk = this->get_world ()->get_chunk (this->curr_chunk.x, this->curr_chunk.z);
		if (prev_chunk)
			prev_chunk->remove_entity (this);
		
		chunk *new_chunk = this->get_world ()->load_chunk (center.x, center.z);
		new_chunk->add_entity (this);
		this->curr_chunk.set (center.x, center.z);
	}
	
	/* 
	 * Used when transitioning players between worlds or teleporting.
	 * This sends common chunks (shared by two worlds in their position) without
	 * unloading them first.
	 */
	void
	player::stream_common_chunks (world *wr, entity_pos dest_pos, int radius)
	{
		std::lock_guard<std::mutex> wguard {this->world_lock};
		
		auto spawn_pos = dest_pos;
		chunk_pos center = spawn_pos;
		
		chunk *prev_chunk = this->get_world ()->get_chunk (this->curr_chunk.x, this->curr_chunk.z);
		if (prev_chunk)
			prev_chunk->remove_entity (this);
		
		std::multiset<chunk_pos, chunk_pos_less> to_load {chunk_pos_less (spawn_pos)};
		int r_half = radius / 2;
		for (int cx = (center.x - r_half); cx <= (center.x + r_half); ++cx)
			for (int cz = (center.z - r_half); cz <= (center.z + r_half); ++cz)
				{
					chunk_pos cpos = chunk_pos (cx, cz);
					to_load.insert (cpos);
				}
		
		// keep chunks that are shared between both worlds, remove others.
		for (auto itr = to_load.begin (); itr != to_load.end (); )
			{
				chunk_pos cpos = *itr;
				if (std::find (this->known_chunks.begin (), this->known_chunks.end (),
					cpos) == this->known_chunks.end ())
					{
						// not contained by both worlds, remove.
						itr = to_load.erase (itr);
					}
				else
					++ itr;
			}
		
		for (auto cpos : to_load)
			{
				chunk *ch = wr->load_chunk (cpos.x, cpos.z);
				this->send (packet::make_chunk (cpos.x, cpos.z, ch));
			}
		
		this->send (packet::make_player_pos_and_look (
			dest_pos.x, dest_pos.y, dest_pos.z, dest_pos.y + 1.65, dest_pos.r,
				dest_pos.l, true));
		this->set_pos (dest_pos);
		
		// spawn self to other players and vice-versa.
		for (auto cpos : to_load)
			{
				player *me = this;
				chunk *ch = wr->get_chunk (cpos.x, cpos.z);
				if (ch)
					{
						ch->all_entities (
							[me] (entity *e)
								{
									if (e->get_type () == ET_PLAYER)
										{
											player* pl = dynamic_cast<player *> (e);
											if (pl == me) return;
									
											me->spawn_to (pl);
											pl->spawn_to (me);
										}
								});
					}
			}
		
		// unload all other chunks
		for (auto itr = this->known_chunks.begin (); itr != this->known_chunks.end (); )
			{
				chunk_pos cpos = *itr;
				if (std::find (to_load.begin (), to_load.end (), cpos) == to_load.end ())
					{
						this->send (packet::make_empty_chunk (cpos.x, cpos.z));
						itr = this->known_chunks.erase (itr);
					}
				else
					++ itr;
			}
		
		to_load.clear ();
	}
	
//--
	
	
	
	/* 
	 * Moves the player to the specified position.
	 */
	void
	player::move_to (entity_pos dest)
	{
		block_pos b_dest = dest;
		int w_width = this->get_world ()->get_width ();
		int w_depth = this->get_world ()->get_depth ();
		if ((w_width > 0) || (w_depth > 0))
			{
				bool changed = false;
				if (w_width > 0) {
					if (dest.x < 0.0) { dest.x = 1.0; changed = true; }
					else if (dest.x >= w_width) { dest.x = w_width - 1; changed = true; }
				}
				if (w_depth > 0) {
					if (dest.z < 0.0) { dest.z = 1.0; changed = true; }
					else if (dest.z >= w_depth) { dest.z = w_depth - 1; changed = true; }
				}
				
				if (changed)
					{
						this->send (packet::make_player_pos_and_look (
							dest.x, dest.y, dest.z, dest.y + 1.65, dest.r, dest.l, dest.on_ground));
					}
			}
		
		entity_pos prev_pos = this->get_pos ();
		this->set_pos (dest);
		
		chunk_pos curr_cpos = dest;
		chunk_pos prev_cpos = prev_pos;
		if ((curr_cpos.x != prev_cpos.x) || (curr_cpos.z != prev_cpos.z))
			{
				this->stream_chunks ();
			}
		
		if (prev_pos == dest)
			return;
		
		double x_delta = dest.x - prev_pos.x;
		double y_delta = dest.y - prev_pos.y;
		double z_delta = dest.z - prev_pos.z;
		float r_delta = dest.r - prev_pos.r;
		float l_delta = dest.l - prev_pos.l;
		
		if (x_delta == 0.0 && y_delta == 0.0 && z_delta == 0.0)
			{
				// position hasn't changed.
				
				if (r_delta == 0.0f && l_delta == 0.0f)
					{
						// the player hasn't moved at all.
						
					}
				else
					{
						// only orientation changed.
						std::lock_guard<std::mutex> guard {this->visible_player_lock};
						for (player *pl : this->visible_players)
							{
								pl->send (packet::make_entity_look (this->get_eid (), dest.r, dest.l));
								pl->send (packet::make_entity_head_look (this->get_eid (), dest.r));
							}
					}
			}
		else
			{
				// position has changed.
				
				std::lock_guard<std::mutex> guard {this->visible_player_lock};
				for (player *pl : this->visible_players)
					{
						pl->send (packet::make_entity_teleport (this->get_eid (),
							std::round (dest.x * 32.0), std::round (dest.y * 32.0),
							std::round (dest.z * 32.0), dest.r, dest.l));
						pl->send (packet::make_entity_head_look (this->get_eid (), dest.r));
					}
			}
	}
	
	/* 
	 * Teleports the player to the given position.
	 */
	void
	player::teleport_to (entity_pos dest)
	{
		block_pos b_dest = dest;
		this->move_to (dest);
		this->send (packet::make_player_pos_and_look (
			dest.x, dest.y, dest.z, dest.y + 1.65, dest.r, dest.l, dest.on_ground));
	}
	
	/* 
	 * Checks whether this player can be seen by player @{pl}.
	 */
	bool
	player::visible_to (player *pl)
	{
		chunk_pos me_pos = this->get_pos ();
		chunk_pos pl_pos = pl->get_pos ();
		
		return (
			(utils::iabs (me_pos.x - pl_pos.x) <= player::chunk_radius ()) &&
			(utils::iabs (me_pos.z - pl_pos.z) <= player::chunk_radius ()));
	}
	
	
	
//----
	
	static void
	get_ping_name (char col, const char *username, char *out)
	{
		std::string str;
		str.append ("§");
		str.push_back (col);
		str.append (username);
		if (str.size () > 16)
			{
				str.resize (13);
				str.append ("...");
			}
		std::strcpy (out, str.c_str ());
	}
	
	/* 
	 * Sends a ping packet to the player and waits for a response.
	 */
	void
	player::ping ()
	{
		this->ping_waiting = true;
		this->last_ping = std::chrono::system_clock::now ();
		this->ping_id = std::chrono::system_clock::to_time_t (this->last_ping) & 0xFFFF;
		this->send (packet::make_ping (this->ping_id));
	}
	
	/* 
	 * Sends a ping packet to the player only if the specified amount of
	 * milliseconds have passed since the last ping packet has been sent.
	 * 
	 * If the player is still waiting for a ping response, the function
	 * will kick the player.
	 */
	void
	player::try_ping (int ms)
	{
		if ((this->last_ping + std::chrono::milliseconds (ms))
			<= std::chrono::system_clock::now ())
			{
				if (this->ping_waiting)
					{
						this->kick ("§cPing timeout");
						return;
					}
				
				this->ping ();
			}
	}
	
	
	
//----
	
	/* 
	 * Spawns self to the specified player.
	 */
	void
	player::spawn_to (player *pl)
	{
		if (pl == this)
			return;
		
		std::string col_name;
		col_name.append ("§");
		col_name.push_back (this->get_rank ().main_group->get_color ());
		col_name.append (this->get_username ());
		
		char ping_name[24];
		get_ping_name (this->get_rank ().main_group->get_color (), this->get_username (),
			ping_name);
		
		entity_pos me_pos = this->get_pos ();
		entity_metadata me_meta;
		this->build_metadata (me_meta);
		pl->send (packet::make_spawn_named_entity (
			this->get_eid (), col_name.c_str (),
			me_pos.x, me_pos.y, me_pos.z, me_pos.r, me_pos.l, 0, me_meta));
		pl->send (packet::make_entity_head_look (this->get_eid (), me_pos.r));
		pl->send (packet::make_player_list_item (ping_name, true, this->ping_time_ms));
		
		{
			std::lock_guard<std::mutex> guard {pl->visible_player_lock};
			pl->visible_players.insert (this);
		}
	}
	
	/* 
	 * Despawns self from the specified player.
	 */
	void
	player::despawn_from (player *pl)
	{
		if (pl == this)
			return;
		
		char ping_name[24];
		get_ping_name (this->get_rank ().main_group->get_color (), this->get_username (),
			ping_name);
		
		pl->send (packet::make_destroy_entity (this->get_eid ()));
		pl->send (packet::make_player_list_item (ping_name, false, 0));
		std::lock_guard<std::mutex> guard {pl->visible_player_lock};
		pl->visible_players.erase (this);
	}
	
	
	
//----
	
	/* 
	 * Sends the given message to the player.
	 */
	void
	player::message (const char *msg, const char *prefix, bool first_line)
	{
		std::vector<std::string> lines;
		
		wordwrap::wrap_prefix (lines, msg, 64, prefix, first_line);
		
		for (auto& line : lines)
			{
				this->send (packet::make_message (line.c_str ()));
			}
	}
	
	void
	player::message (const std::string& msg, const char *prefix, bool first_line)
	{
		this->message (msg.c_str (), prefix, first_line);
	}
	
	void
	player::message_spaced (const char *msg, bool remove_from_first)
	{
		std::vector<std::string> lines;
		
		wordwrap::wrap_spaced (lines, msg, 64, remove_from_first);
		
		for (auto& line : lines)
			{
				this->send (packet::make_message (line.c_str ()));
			}
	}
	
	void
	player::message_spaced (const std::string& msg, bool remove_from_first)
	{
		this->message_spaced (msg.c_str (), remove_from_first);
	}
	
	void
	player::message_nowrap (const char *msg)
	{
		this->send (packet::make_message (msg));
	}
	
	void
	player::message_nowrap (const std::string& msg)
	{
		this->send (packet::make_message (msg.c_str ()));
	}
	
	
	
//----
	
	/* 
	 * Checks whether the player's rank has the given permission node.
	 */
	bool
	player::has (const char *perm)
	{
		return this->get_rank ().has (perm);
	}
	
	/* 
	 * Utility function used by commands.
	 * Returns true if the player has the given permission; otherwise, it prints
	 * an error message to the player and return false.
	 */
	bool
	player::perm (const char *perm)
	{
		if (this->has (perm))
			return true;
		
		this->message_nowrap (messages::insufficient_permissions (
			this->get_server ().get_groups (), perm));
		return false;
	}
	
	
	
//----
	
	/* 
	 * Loads information about the player from the server's SQL database.
	 */
	void
	player::load_data ()
	{
		sql::statement stmt = this->get_server ().sql ().create (
			"SELECT * FROM `players` WHERE `name`=?;");
		stmt.bind_text (1, this->get_username ());
		
		if (stmt.step () == sql::row)
			{
				this->rnk.set ((const char *)stmt.column_text (2), this->get_server ().get_groups ());
				std::strcpy (this->nick, (const char *)stmt.column_text (3));
			}
		else
			{
				this->rnk.set (this->get_server ().get_groups ().default_rank);
				std::string grp_str;
				this->rnk.get_string (grp_str);
				
				std::strcpy (this->nick, this->username);
				
				sql::statement stmt = this->get_server ().sql ().create (
					"INSERT INTO `players` (`name`, `groups`, `nick`) VALUES (?, ?, ?)");
				stmt.bind_text (1, this->get_username ());
				stmt.bind_text (2, grp_str.c_str (), -1, sql::dctor_transient);
				stmt.bind_text (3, this->get_nickname ());
				stmt.execute ();
			}
		
		std::string str;
		str.append ("§");
		str.push_back (this->rnk.main ()->get_color ());
		str.append (this->nick);
		std::strcpy (this->colored_nick, str.c_str ());
	}
	
	
	
	/* 
	 * Modifies the player's nickname.
	 */
	void
	player::set_nickname (const char *nick, bool modify_sql)
	{
		if (std::strlen (nick) > 36)
			return;
		
		std::strcpy (this->nick, nick);
		
		std::string str;
		str.append ("§");
		str.push_back (this->rnk.main ()->get_color ());
		str.append (this->nick);
		std::strcpy (this->colored_nick, str.c_str ());
		
		if (modify_sql)
			{
				std::ostringstream ss;
				ss << "UPDATE `players` SET `nick`='"
				 << nick << "' WHERE `name`='"
				 << this->get_username () << "';";
				this->get_server ().sql ().execute (ss.str ().c_str ());
			}
	}
	
	
	
//----
	
	static bool
	ms_passed (std::chrono::time_point<std::chrono::system_clock> pt, int ms)
	{
		auto now = std::chrono::system_clock::now ();
		return ((pt + std::chrono::milliseconds (ms)) <= now);
	}
	
	
	
	/* 
	 * Packet handlers:
	 */
	
	// dummy handler, doesn't do anything.
	void
	handle_packet_xx (player *pl, packet_reader reader)
		{ return; }
	
	
	void
	player::handle_packet_00 (player *pl, packet_reader reader)
	{
		if (!pl->logged_in)
			{ pl->disconnect (); return; }
		
		int id = reader.read_byte ();
		
		if (!pl->ping_waiting)
			return;
		
		if ((id != 0) && id != pl->ping_id)
			{
				pl->kick ("§cPing timeout");
				return;
			}
		
		pl->ping_time_ms = std::chrono::duration_cast<std::chrono::milliseconds> (
			std::chrono::system_clock::now () - pl->last_ping).count ();
		pl->ping_waiting = false;
		
		// update self
		char ping_name[24];
		get_ping_name (pl->get_rank ().main_group->get_color (), pl->get_username (),
			ping_name);
		pl->send (packet::make_player_list_item (ping_name, true, pl->ping_time_ms));
		
		// update other players
		player *me = pl;
		pl->get_world ()->get_players ().all (
			[me] (player *pl)
				{
					char ping_name[24];
					get_ping_name (me->get_rank ().main_group->get_color (), me->get_username (),
						ping_name);
					pl->send (packet::make_player_list_item (ping_name, true, me->ping_time_ms));
				});
	}
	
	void
	player::handle_packet_02 (player *pl, packet_reader reader)
	{
		int protocol_version = reader.read_byte ();
		if (protocol_version != 39)
			{
				if (protocol_version < 39)
					pl->kick ("§cOutdated client", "outdated protocol version");
				else
					pl->kick ("§ahCraft has not been updated yet", "newer protocol version");
				return;
			}
		
		char username[17];
		/*
		int username_len = reader.read_string (username, 16);
		if (username_len < 2)
			{
				pl->log (LT_WARNING) << "@" << pl->get_ip () << " connected with an invalid username." << std::endl;
				pl->disconnect ();
				return;
			}
		*/
		// Used when testing
		{
			static const char *names[] =
				{ "BizarreCake", "testdude", "testdude2", "testdude3", "testdude4" };
			static int index = 0, count = 5;
			const char *cur = names[index++];
			if (index >= count)
				index = 0;
			std::strcpy (username, cur);
		}
		
		
		pl->log () << "Player " << username << " has logged in from @" << pl->get_ip () << std::endl;
		std::strcpy (pl->username, username);
		
		pl->load_data ();
		pl->handshake = true;
		
		{
			std::string str;
			str.append ("§");
			str.push_back (pl->rnk.main ()->get_color ());
			str.append (pl->username);
			std::strcpy (pl->colored_username, str.c_str ());
		}
		
		if (!pl->get_server ().done_connecting (pl))
			return;
		
		pl->send (packet::make_login (pl->get_eid (), "hCraft", 1, 0, 0,
			(pl->get_server ().get_config ().max_players > 64)
				? 64 : (pl->get_server ().get_config ().max_players)));
		pl->logged_in = true;
		
		// insert self into player list ping
		{
			char ping_name[24];
			get_ping_name (pl->get_rank ().main_group->get_color (), pl->get_username (),
				ping_name);
			pl->send (packet::make_player_list_item (ping_name, true, pl->ping_time_ms));
		}
		
		{
			std::ostringstream ss;
			ss << "§e[§a+§e] " << pl->get_colored_nickname ()
				 << " §ehas joined the server§f!";
			pl->get_server ().get_players ().message_nowrap (ss.str ());
		}
		pl->join_world (pl->get_server ().get_main_world ());
	}
	
	void
	player::handle_packet_03 (player *pl, packet_reader reader)
	{
		if (!pl->logged_in)
			{ pl->disconnect (); return; }
		
		bool is_global_message = false;
		
		char text[384];
		int  text_len = reader.read_string (text, 360);
		if (text_len <= 0)
			{
				pl->log (LT_WARNING) << "Received an invalid string from player " << pl->get_username () << std::endl;
				pl->disconnect ();
				return;
			}
		
		std::string msg {text};
		
		if (msg[msg.size () - 1] == '\\')
			{
				msg.pop_back ();
				if (sutils::is_empty (msg))
					{ msg.push_back ('\\'); goto continue_write; }
				
				pl->msgbuf << msg;
				pl->message_nowrap ("§7 * §8Message appended to buffer§7.");
				return;
			}
		
		if (pl->msgbuf.tellp () > 0)
			{
				pl->msgbuf << msg;
				msg.assign (pl->msgbuf.str ());
				pl->msgbuf.str (std::string ());
				pl->msgbuf.clear ();
			}
		
		// handle commands
		if (msg[0] == '/')
			{
				command_reader cread {msg};
				const std::string& cname = cread.command_name ();
				if (cname.empty ())
					{
						pl->message_nowrap ("§c * §ePlease enter a command§f.");
						return;
					}
				
				command *cmd = pl->get_server ().get_commands ().find (cname.c_str ());
				if (!cmd)
					{
						if (cname.size () > 32)
							pl->message_nowrap ("§c * §eNo such command§f.");
						else
							pl->message (("§c * §eNo such command§f: §c" + cname + "§f.").c_str ());
						return;
					}
				
				cmd->execute (pl, cread);
				return;
			}
		
		// handle chat modes
		if (msg[0] == '#')
			{
				// server-wide message
				
				if (sutils::ltrim (msg.erase (0, 1)).empty ())
					{
						msg.push_back ('#');
						goto continue_write;
					}
				
				is_global_message = true;
			}
		else if (msg[0] == '@')
			{
				// TODO: handle private messages
			}
			
	continue_write:
		std::ostringstream ss;
		
		if (is_global_message)
			ss << "§c# ";
		
		group *mgrp = pl->get_rank ().main ();
		ss << mgrp->get_mprefix ();
		for (group *grp : pl->get_rank ().get_groups ())
			ss << grp->get_prefix ();
		ss << pl->get_colored_nickname ();
		ss << mgrp->get_msuffix ();
		for (group *grp : pl->get_rank ().get_groups ())
			ss << grp->get_suffix ();
		ss << "§f: " << msg;
		
		std::string out = ss.str ();
		
		playerlist *target;
		if (is_global_message)
			{
				pl->get_logger () (LT_CHAT) << "#" << pl->get_username () << ": " << msg << std::endl;
				target = &pl->get_server ().get_players ();
			}
		else
			{
				pl->get_logger () (LT_CHAT) << pl->get_username () << ": " << msg << std::endl;
				target = &pl->get_world ()->get_players ();
			}
		
		target->all (
			[&out] (player *pl)
				{
					pl->message (out.c_str ());
				});
	}
	
	void
	player::handle_packet_0a (player *pl, packet_reader reader)
	{
		if (!pl->handshake)
			{ pl->disconnect (); return; }
		
		bool on_ground;
		
		on_ground = reader.read_byte ();
		
		entity_pos curr_pos = pl->get_pos ();
		entity_pos new_pos {curr_pos.x, curr_pos.y, curr_pos.z, curr_pos.r,
			curr_pos.l, on_ground};
		pl->move_to (new_pos);
		
		pl->try_ping (5000);
	}
	
	void
	player::handle_packet_0b (player *pl, packet_reader reader)
	{
		if (!pl->logged_in)
			{ pl->disconnect (); return; }
		
		double x, y, z, stance;
		bool on_ground;
		
		x = reader.read_double ();
		y = reader.read_double ();
		stance = reader.read_double ();
		z = reader.read_double ();
		on_ground = reader.read_byte ();
		
		entity_pos curr_pos = pl->get_pos ();
		entity_pos new_pos {x, y, z, curr_pos.r, curr_pos.l, on_ground};
		pl->move_to (new_pos);
		
		pl->try_ping (5000);
	}
	
	void
	player::handle_packet_0c (player *pl, packet_reader reader)
	{
		if (!pl->logged_in)
			{ pl->disconnect (); return; }
		
		float r, l;
		bool on_ground;
		
		r = reader.read_float ();
		l = reader.read_float ();
		on_ground = reader.read_byte ();
		
		entity_pos curr_pos = pl->get_pos ();
		entity_pos new_pos {curr_pos.x, curr_pos.y, curr_pos.z, r, l, on_ground};
		pl->move_to (new_pos);
		
		pl->try_ping (5000);
	}
	
	void
	player::handle_packet_0d (player *pl, packet_reader reader)
	{
		if (!pl->logged_in)
			{ pl->disconnect (); return; }
		
		double x, y, z, stance;
		float r, l;
		bool on_ground;
		
		x = reader.read_double ();
		y = reader.read_double ();
		stance = reader.read_double ();
		z = reader.read_double ();
		r = reader.read_float ();
		l = reader.read_float ();
		on_ground = reader.read_byte ();
		
		entity_pos curr_pos = pl->get_pos ();
		entity_pos new_pos {x, y, z, r, l, on_ground};
		pl->move_to (new_pos);
		
		pl->try_ping (5000);
	}
	
	void
	player::handle_packet_0e (player *pl, packet_reader reader)
	{
		if (!pl->logged_in)
			{ pl->disconnect (); return; }
		
		char status;
		int x;
		unsigned char y;
		int z;
		char face;
		
		status = reader.read_byte ();
		x = reader.read_int ();
		y = reader.read_byte ();
		z = reader.read_int ();
		face = reader.read_byte ();
		
		int w_width = pl->get_world ()->get_width ();
		int w_depth = pl->get_world ()->get_depth ();
		if (((w_width > 0) && ((x >= w_width) || (x < 0))) ||
				((w_depth > 0) && ((z >= w_depth) || (z < 0))))
			{
				pl->send (packet::make_block_change (
					x, y, z,
					pl->get_world ()->get_id (x, y, z),
					pl->get_world ()->get_meta (x, y, z)));
				return;
			}
		
		{
			int xx, yy, zz;
			for (xx = (x - 3); xx <= (x + 3); ++xx)
				for (yy = (y - 3); yy <= (y + 3); ++yy)
					for (zz = (z - 3); zz <= (z + 3); ++zz)
						pl->get_world ()->queue_update (xx, yy, zz, 0, 0);
		}
		//pl->get_world ()->queue_update (x, y, z, 0, 0, pl);
	}
	
	void
	player::handle_packet_12 (player *pl, packet_reader reader)
	{
		if (!pl->logged_in)
			{ pl->disconnect (); return; }
		
		int eid = reader.read_int ();
		char animation = reader.read_byte ();
		
		if (animation == 1)
			{
				pl->get_world ()->get_players ().send_to_all (
					packet::make_animation (pl->get_eid (), animation), pl);
			}
	}
	
	void
	player::handle_packet_13 (player *pl, packet_reader reader)
	{
		if (!pl->logged_in)
			{ pl->disconnect (); return; }
	}
	
	void
	player::handle_packet_fe (player *pl, packet_reader reader)
	{
		std::ostringstream ss;
		auto& cfg = pl->get_server ().get_config ();
		auto& players = pl->get_server ().get_players ();
		
		ss << cfg.srv_motd << "§" << players.count () << "§" << cfg.max_players;
		pl->kick (ss.str ().c_str (), "server list ping");
	}
	
	void
	player::handle_packet_ff (player *pl, packet_reader reader)
	{
		pl->disconnect ();
	}
	
	/* 
	 * Executes the packet handler for the most recently read packet
	 * (stored in `rdbuf').
	 */
	void
	player::handle (const unsigned char *data)
	{
		static void (*handlers[0x100]) (player *, packet_reader) =
			{
				handle_packet_00, handle_packet_xx, handle_packet_02, handle_packet_03, // 0x03
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x07
				handle_packet_xx, handle_packet_xx, handle_packet_0a, handle_packet_0b, // 0x0B
				handle_packet_0c, handle_packet_0d, handle_packet_0e, handle_packet_xx, // 0x0F
				
				handle_packet_xx, handle_packet_xx, handle_packet_12, handle_packet_13, // 0x13
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x17
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x1B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x1F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x23
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x27
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x2B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x2F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x33
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x37
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x3B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x3F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x43
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x47
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x4B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x4F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x53
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x57
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x5B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x5F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x63
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x67
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x6B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x6F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x73
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x77
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x7B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x7F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x83
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x87
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x8B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x8F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x93
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x97
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x9B
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0x9F
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xA3
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xA7
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xAB
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xAF
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xB3
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xB7
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xBB
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xBF
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xC3
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xC7
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xCB
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xCF
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xD3
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xD7
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xDB
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xDF
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xE3
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xE7
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xEB
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xEF
				
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xF3
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xF7
				handle_packet_xx, handle_packet_xx, handle_packet_xx, handle_packet_xx, // 0xFB
				handle_packet_xx, handle_packet_xx, handle_packet_fe, handle_packet_ff, // 0xFF
			};
		
		packet_reader reader {data};
		handlers[reader.read_byte ()] (this, reader);
	}
}

