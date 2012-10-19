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

#include "world.hpp"
#include "utils.hpp"
#include "playerlist.hpp"
#include "player.hpp"
#include "packet.hpp"
#include <stdexcept>
#include <cassert>
#include <cstring>


namespace hCraft {
	
	static unsigned long long
	chunk_key (int x, int z)
		{ return ((unsigned long long)z << 32) | (unsigned long long)x; }
	
	static void
	chunk_coords (unsigned long long key, int* x, int* z)
		{ *x = key & 0xFFFFFFFFU; *z = key >> 32; }
	
	
	
	/* 
	 * Constructs a new empty world.
	 */
	world::world (const char *name, world_generator *gen, world_provider *provider)
	{
		assert (std::strlen (name) <= 32);
		std::strcpy (this->name, name);
		
		this->gen = gen;
		this->width = 0;
		this->depth = 0;
		
		this->prov = provider;
		this->edge_chunk = nullptr;
		
		this->players = new playerlist ();
		this->th_running = false;
	}
	
	/* 
	 * Class destructor.
	 */
	world::~world ()
	{
		delete this->players;
		
		delete this->gen;
		if (this->edge_chunk)
			delete this->edge_chunk;
		
		if (this->prov)
			delete this->prov;
		
		{
			std::lock_guard<std::mutex> guard {this->chunk_lock};
			for (auto itr = this->chunks.begin (); itr != this->chunks.end (); ++itr)
				{
					chunk *ch = itr->second;
					delete ch;
				}
			this->chunks.clear ();
		}
	}
	
	
	
	/* 
	 * Starts the world's "physics"-handling thread.
	 */
	void
	world::start ()
	{
		if (this->th_running)
			return;
		
		this->th_running = true;
		this->th.reset (new std::thread (
			std::bind (std::mem_fn (&hCraft::world::worker), this)));
	}
	
	/* 
	 * Stops the world's thread.
	 */
	void
	world::stop ()
	{
		if (!this->th_running)
			return;
		
		this->th_running = false;
		if (this->th->joinable ())
			this->th->join ();
		this->th.reset ();
	}
	
	
	
	/* 
	 * The function ran by the world's thread.
	 */
	void
	world::worker ()
	{
		while (this->th_running)
			{
				{
					std::lock_guard<std::mutex> guard {this->update_lock};
					if (!this->updates.empty ())
						{
							block_update &update = this->updates.front ();
							this->set_id_and_meta (update.x, update.y, update.z,
								update.id, update.meta);
							
							chunk *ch = this->get_chunk_at (update.x, update.z);
							if (ch)
								{
									ch->relight (utils::mod (update.x, 16), utils::mod (update.z, 16));
									this->get_players ().all (
										[&update] (player *pl)
											{
												pl->send (packet::make_block_change (update.x, update.y,
													update.z, update.id, update.meta));
											}, update.pl);
								}
							
							this->updates.pop ();
						}
				}
				
				std::this_thread::sleep_for (std::chrono::milliseconds (10));
			}
	}
	
	
	
	void
	world::set_width (int width)
	{
		if (width % 16 != 0)
			width += width % 16;
		this->width = width;
		
		if (this->width > 0 && !this->edge_chunk)
			{
				this->edge_chunk = new chunk ();
				this->gen->generate_edge (*this, this->edge_chunk);
				this->edge_chunk->recalc_heightmap ();
				this->edge_chunk->relight ();
			}
	}
	
	void
	world::set_depth (int depth)
	{
		if (depth % 16 != 0)
			depth += depth % 16;
		this->depth = depth;
		
		if (this->depth > 0 && !this->edge_chunk)
			{
				this->edge_chunk = new chunk ();
				this->gen->generate_edge (*this, this->edge_chunk);
				this->edge_chunk->recalc_heightmap ();
				this->edge_chunk->relight ();
			}
	}
	
	
	
	/* 
	 * Saves all modified chunks to disk.
	 */
	void
	world::save_all ()
	{
		if (this->prov == nullptr)
			return;
		
		std::lock_guard<std::mutex> guard {this->chunk_lock};
		this->prov->open (*this);
		for (auto itr = this->chunks.begin (); itr != this->chunks.end (); ++itr)
			{
				chunk *ch = itr->second;
				if (ch->modified)
					{
						int x, z;
						chunk_coords (itr->first, &x, &z);
						this->prov->save (*this, ch, x, z);
						ch->modified = false;
					}
			}
		this->prov->close ();
	}
	
	
	
	/* 
	 * Loads up a grid of radius x radius chunks around the given point
	 * (specified in chunk coordinates).
	 */
	void
	world::load_grid (chunk_pos cpos, int radius)
	{
		int r_half = radius >> 1;
		int cx, cz;
		
		for (cx = (cpos.x - r_half); cx <= (cpos.x + r_half); ++cx)
			for (cz = (cpos.z - r_half); cz <= (cpos.z + r_half); ++cz)
				{
					this->load_chunk (cx, cz);
				}
	}
	
	/* 
	 * Calls load_grid around () {x: 0, z: 0}, and attempts to find a suitable
	 * spawn position. 
	 */
	void
	world::prepare_spawn (int radius)
	{
		this->load_grid (chunk_pos (0, 0), radius);
		block_pos best {0, 0, 0};
		
		int cx, cz, x, z;
		short h;
		
		for (cx = 0; cx <= 2; ++cx)
			for (cz = 0; cz <= 2; ++cz)
				{
					chunk *ch = this->load_chunk (cx, cz);
					for (x = 0; x < 16; ++x)
						for (z = 0; z < 16; ++z)
							{
								h = ch->get_height (x, z);
								if (ch->get_id (x, h - 1, z) != 0 && ((h + 1) > best.y))
									best.set ((cx * 16) + x, h + 1, (cz * 16) + z);
							}
				}
		
		this->spawn_pos = best;
	}
	
	
	
	/* 
	 * Inserts the specified chunk into this world at the given coordinates.
	 */
	void
	world::put_chunk (int x, int z, chunk *ch)
	{
		unsigned long long key = chunk_key (x, z);
		
		std::lock_guard<std::mutex> guard {this->chunk_lock};
		auto itr = this->chunks.find (key);
		if (itr != this->chunks.end ())
			{
				chunk *prev = itr->second;
				if (prev == ch) return;
				delete prev;
				this->chunks.erase (itr);
			}
		
		this->chunks[key] = ch;
	}
	
	/* 
	 * Searches the chunk world for a chunk located at the specified coordinates.
	 */
	chunk*
	world::get_chunk (int x, int z)
	{
		if (((this->width > 0) && (((x * 16) >= this->width) || (x < 0))) ||
				((this->depth > 0) && (((z * 16) >= this->depth) || (z < 0))))
			return this->edge_chunk;
		
		unsigned long long key = chunk_key (x, z);
		
		std::lock_guard<std::mutex> guard {this->chunk_lock};
		auto itr = this->chunks.find (key);
		if (itr != this->chunks.end ())
			return itr->second;
		
		return nullptr;
	}
	
	/* 
	 * Returns the chunk located at the given block coordinates.
	 */
	chunk*
	world::get_chunk_at (int bx, int bz)
	{
		return this->get_chunk (utils::div (bx, 16), utils::div (bz, 16));
	}
	
	/* 
	 * Same as get_chunk (), but if the chunk does not exist, it will be either
	 * loaded from a file (if such a file exists), or completely generated from
	 * scratch.
	 */
	chunk*
	world::load_chunk (int x, int z)
	{
		chunk *ch = this->get_chunk (x, z);
		if (ch) return ch;
		
		ch = new chunk ();
		
		// try to load from disk
		this->prov->open (*this);
		if (this->prov->load (*this, ch, x, z))
			{
				this->prov->close ();
				this->put_chunk (x, z, ch);
				ch->recalc_heightmap ();
				ch->relight ();
				return ch;
			}
		
		this->prov->close ();
		this->put_chunk (x, z, ch);
		this->gen->generate (*this, ch, x, z);
		ch->recalc_heightmap ();
		ch->relight ();
		return ch;
	}
	
	
	
//----
	/* 
	 * Block interaction: 
	 */
	
	void
	world::set_id (int x, int y, int z, unsigned short id)
	{
		chunk *ch = this->load_chunk (utils::div (x, 16), utils::div (z, 16));
		ch->set_id (utils::mod (x, 16), y, utils::mod (z, 16), id);
	}
	
	unsigned short
	world::get_id (int x, int y, int z)
	{
		chunk *ch = this->get_chunk (utils::div (x, 16), utils::div (z, 16));
		if (!ch)
			return 0;
		return ch->get_id (utils::mod (x, 16), y, utils::mod (z, 16));
	}
	
	
	void
	world::set_meta (int x, int y, int z, unsigned char val)
	{
		chunk *ch = this->load_chunk (utils::div (x, 16), utils::div (z, 16));
		ch->set_meta (utils::mod (x, 16), y, utils::mod (z, 16), val);
	}
	
	unsigned char
	world::get_meta (int x, int y, int z)
	{
		chunk *ch = this->get_chunk (utils::div (x, 16), utils::div (z, 16));
		if (!ch)
			return 0;
		return ch->get_meta (utils::mod (x, 16), y, utils::mod (z, 16));
	}
	
	
	void
	world::set_block_light (int x, int y, int z, unsigned char val)
	{
		chunk *ch = this->load_chunk (utils::div (x, 16), utils::div (z, 16));
		ch->set_block_light (utils::mod (x, 16), y, utils::mod (z, 16), val);
	}
	
	unsigned char
	world::get_block_light (int x, int y, int z)
	{
		chunk *ch = this->get_chunk (utils::div (x, 16), utils::div (z, 16));
		if (!ch)
			return 0;
		return ch->get_block_light (utils::mod (x, 16), y, utils::mod (z, 16));
	}
	
	
	void
	world::set_sky_light (int x, int y, int z, unsigned char val)
	{
		chunk *ch = this->load_chunk (utils::div (x, 16), utils::div (z, 16));
		ch->set_sky_light (utils::mod (x, 16), y, utils::mod (z, 16), val);
	}
	
	unsigned char
	world::get_sky_light (int x, int y, int z)
	{
		chunk *ch = this->get_chunk (utils::div (x, 16), utils::div (z, 16));
		if (!ch)
			return 0xF;
		return ch->get_sky_light (utils::mod (x, 16), y, utils::mod (z, 16));
	}
	
	
	void
	world::set_id_and_meta (int x, int y, int z, unsigned short id, unsigned char meta)
	{
		chunk *ch = this->load_chunk (utils::div (x, 16), utils::div (z, 16));
		ch->set_id_and_meta (utils::mod (x, 16), y, utils::mod (z, 16), id, meta);
	}
	
	
	
//----
	
	/* 
	 * Enqueues an update that should be made to a block in this world
	 * and sent to nearby players.
	 */
	void
	world::queue_update (int x, int y, int z, unsigned short id,
		unsigned char meta, player *pl)
	{
		std::lock_guard<std::mutex> guard {this->update_lock};
		this->updates.emplace (x, y, z, id, meta, pl);
	}
}

