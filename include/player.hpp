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

#ifndef _hCraft__PLAYER_H_
#define _hCraft__PLAYER_H_

#include "entity.hpp"
#include "logger.hpp"
#include "packet.hpp"
#include "world.hpp"
#include "rank.hpp"
#include "messages.hpp"

#include <queue>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <event2/event.h>
#include <event2/bufferevent.h>


namespace hCraft {
	
	class server;
	
	/* 
	 * Represents a player.
	 */
	class player: public entity
	{
		server& srv;
		logger& log;
		
		struct event_base *evbase;
		struct bufferevent *bufev;
		evutil_socket_t sock;
		
		rank rnk;
		char ip[16];
		bool logged_in;
		bool handshake;
		bool fail; // true if the player is no longer valid, and must be disposed of.
		
		char username[17];
		char colored_username[24];
		char nick[37]; // 36 chars max
		char colored_nick[48];
		
		char kick_msg[384];
		bool kicked;
		
		unsigned char rdbuf[384];
		int total_read;
		int read_rem;
		
		bool ping_waiting;
		std::chrono::time_point<std::chrono::system_clock> last_ping;
		int ping_id;
		int ping_time_ms;
		
		std::queue<packet *> out_queue;
		std::mutex out_lock;
		
		world *curr_world;
		chunk_pos curr_chunk;
		std::mutex world_lock;
		std::mutex join_lock;
		std::unordered_set<chunk_pos, chunk_pos_hash> known_chunks;
		std::unordered_set<player *> visible_players;
		std::mutex visible_player_lock;
		
		std::ostringstream msgbuf;
		
	private:
		/* 
		 * libevent callback functions:
		 */
		static void handle_read (struct bufferevent *bufev, void *ctx);
		static void handle_write (struct bufferevent *bufev, void *ctx);
		static void handle_event (struct bufferevent *bufev, short events, void *ctx);
		
		/* 
		 * Packet handlers:
		 */
		static void handle_packet_00 (player *pl, packet_reader reader);
		static void handle_packet_02 (player *pl, packet_reader reader);
		static void handle_packet_03 (player *pl, packet_reader reader);
		static void handle_packet_0a (player *pl, packet_reader reader);
		static void handle_packet_0b (player *pl, packet_reader reader);
		static void handle_packet_0c (player *pl, packet_reader reader);
		static void handle_packet_0d (player *pl, packet_reader reader);
		static void handle_packet_0e (player *pl, packet_reader reader);
		static void handle_packet_12 (player *pl, packet_reader reader);
		static void handle_packet_13 (player *pl, packet_reader reader);
		static void handle_packet_fe (player *pl, packet_reader reader);
		static void handle_packet_ff (player *pl, packet_reader reader);
		
		/* 
		 * Executes the packet handler for the most recently read packet
		 * (stored in `rdbuf').
		 */
		void handle (const unsigned char *data);
		
	//----
		
		/* 
		 * Sends a ping packet to the player and waits for a response.
		 */
		void ping ();
		
		/* 
		 * Sends a ping packet to the player only if the specified amount of
		 * milliseconds have passed since the last ping packet has been sent.
		 * 
		 * If the player is still waiting for a ping response, the function
		 * will kick the player.
		 */
		void try_ping (int ms);
		
	//----
		
		/* 
		 * Moves the player to the specified position.
		 */
		void move_to (entity_pos dest);
		
		/* 
		 * Used when transitioning players between worlds or teleporting.
		 * This sends common chunks (shared by two worlds in their position) without
		 * unloading them first.
		 */
		void stream_common_chunks (world *wr, entity_pos dest_pos,
			int radius = player::chunk_radius ());
		
	//----
		
		/* 
		 * Loads information about the player from the server's SQL database.
		 */
		void load_data ();
		
	public:
		inline server& get_server () { return this->srv; }
		inline logger& get_logger () { return this->log; }
		inline const char* get_ip () { return this->ip; }
		inline const char* get_username () { return this->username; }
		inline const char* get_colored_username () { return this->colored_username; }
		inline const char* get_nickname () { return this->nick; }
		inline const char* get_colored_nickname () { return this->colored_nick; }
		inline const rank& get_rank () { return this->rnk; }
		
		inline world* get_world () { return this->curr_world; }
		static constexpr int chunk_radius () { return 7; }
		
		inline int get_ping () { return this->ping_time_ms; }
		
		// whether the player isn't valid anymore, and should be destroyed.
		inline bool bad () { return this->fail; }
		
		virtual entity_type get_type () { return ET_PLAYER; }
		
	public:
		/* 
		 * Constructs a new player around the given socket.
		 */
		player (server &srv, struct event_base *evbase, evutil_socket_t sock,
			const char *ip);
		
		// copy constructor.
		player (const player &) = delete;
		
		/* 
		 * Class destructor.
		 */
		~player ();
		
		
		
		/* 
		 * Marks the player invalid, forcing the server that spawned the player to
		 * eventually destroy it.
		 */
		void disconnect (bool silent = false);
		
		/* 
		 * Kicks the player with the given message.
		 */
		void kick (const char *msg, const char *log_msg = nullptr);
		
		
		
		/* 
		 * Inserts the specified packet into the player's queue of outgoing packets.
		 */
		void send (packet *pack);
		
		
		
		/* 
		 * Sends the player to the given world.
		 */
		void join_world (world* w);
		
		/* 
		 * Sends the player to the given world at the specified location.
		 */
		void join_world_at (world *w, entity_pos destpos);
		
		
		/* 
		 * Loads new close chunks to the player and unloads those that are too
		 * far away.
		 */
		void stream_chunks (int radius = player::chunk_radius ());
		
		/* 
		 * Teleports the player to the given position.
		 */
		void teleport_to (entity_pos dest);
		
		/* 
		 * Checks whether this player can be seen by player @{pl}.
		 */
		bool visible_to (player *pl);
		
		
		
		/* 
		 * Spawns self to the specified player.
		 */
		void spawn_to (player *pl);
		
		/* 
		 * Despawns self from the specified player.
		 */
		void despawn_from (player *pl);
		
		
		
		/* 
		 * Sends the given message to the player.
		 */
		void message (const char *msg, const char *prefix = "§7 > §f", bool first_line = false);
		void message (const std::string& msg, const char *prefix = "§7 > §f", bool first_line = false);
		void message_spaced (const char *msg, bool remove_from_first = false);
		void message_spaced (const std::string& msg, bool remove_from_first = false);
		void message_nowrap (const char *msg);
		void message_nowrap (const std::string& msg);
		
	//----
		
		/* 
		 * Checks whether the player's rank has the given permission node.
		 */
		bool has (const char *perm);
		
		/* 
		 * Utility function used by commands.
		 * Returns true if the player has the given permission; otherwise, it prints
		 * an error message to the player and return false.
		 */
		bool perm (const char *perm);
		
		
		
	//----
		
		/* 
		 * Modifies the player's nickname.
		 */
		void set_nickname (const char *nick, bool modify_sql = true);
	};
}

#endif

