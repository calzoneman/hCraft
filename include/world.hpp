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

#ifndef _hCraft__WORLD_H_
#define _hCraft__WORLD_H_

#include "position.hpp"
#include "chunk.hpp"
#include "worldgenerator.hpp"
#include "worldprovider.hpp"

#include <unordered_map>
#include <mutex>
#include <queue>
#include <memory>
#include <thread>


namespace hCraft {
	
	class player;
	class playerlist;
	
	
	/* 
	 * The whereabouts of a block that must be modified and sent to close players.
	 */
	struct block_update
	{
		int x;
		int y;
		int z;
		unsigned short id;
		unsigned char  meta;
		player *pl; // the player that initated the update.
		
		block_update (int x, int y, int z, unsigned short id, unsigned char meta,
			player *pl)
		{
			this->x = x;
			this->y = y;
			this->z = z;
			this->id = id;
			this->meta = meta;
			this->pl = pl;
		}
	};
	
	
	/* 
	 * The world provides methods to easily retreive or modify chunks, and
	 * get/set individual blocks within those chunks. In addition to that,
	 * the world also manages a list of players.
	 */
	class world
	{
		char name[33]; // 32 chars max
		playerlist *players;
		
		std::unique_ptr<std::thread> th;
		bool th_running;
		
		std::queue<block_update> updates;
		std::mutex update_lock;
		std::unordered_map<unsigned long long, chunk *> chunks;
		std::mutex chunk_lock;
		
		int width;
		int depth;
		entity_pos spawn_pos;
		chunk *edge_chunk;
		
		world_generator *gen;
		world_provider *prov;
		
	public:
		inline const char* get_name () { return this->name; }
		inline playerlist& get_players () { return *this->players; }
		
		inline int get_width () const { return this->width; }
		inline int get_depth () const { return this->depth; }
		void set_width (int width);
		void set_depth (int depth);
		void set_size (int width, int depth)
			{ set_width (width); set_depth (depth); }
		
		inline entity_pos get_spawn () const { return this->spawn_pos; }
		inline void set_spawn (const entity_pos& pos) { this->spawn_pos = pos; }
		
	private:
		/* 
		 * The function ran by the world's thread.
		 */
		void worker ();
		
	public:
		/* 
		 * Constructs a new empty world.
		 */
		world (const char *name, world_generator *gen, world_provider *provider);
		
		/* 
		 * Class destructor.
		 */
		~world ();
		
		
		
		/* 
		 * Starts the world's "physics"-handling thread.
		 */
		void start ();
		
		/* 
		 * Stops the world's thread.
		 */
		void stop ();
		
		
		
		/* 
		 * Saves all modified chunks to disk.
		 */
		void save_all ();
		
		
		
		/* 
		 * Loads up a grid of radius x radius chunks around the given point
		 * (specified in chunk coordinates).
		 */
		void load_grid (chunk_pos cpos, int radius);
		
		/* 
		 * Calls load_grid around () {x: 0, z: 0}, and attempts to find a suitable
		 * spawn position. 
		 */
		void prepare_spawn (int radius);
		
		
		
		/* 
		 * Inserts the specified chunk into this world at the given coordinates.
		 */
		void put_chunk (int x, int z, chunk *ch);
		
		/* 
		 * Searches the chunk world for a chunk located at the specified coordinates.
		 */
		chunk* get_chunk (int x, int z);
		
		/* 
		 * Returns the chunk located at the given block coordinates.
		 */
		chunk* get_chunk_at (int bx, int bz);
		
		/* 
		 * Same as get_chunk (), but if the chunk does not exist, it will be either
		 * loaded from a file (if such a file exists), or completely generated from
		 * scratch.
		 */
		chunk* load_chunk (int x, int z);
		
		
		
		/* 
		 * Block interaction: 
		 */
		
		void set_id (int x, int y, int z, unsigned short id);
		unsigned short get_id (int x, int y, int z);
		
		void set_meta (int x, int y, int z, unsigned char val);
		unsigned char get_meta (int x, int y, int z);
		
		void set_block_light (int x, int y, int z, unsigned char val);
		unsigned char get_block_light (int x, int y, int z);
		
		void set_sky_light (int x, int y, int z, unsigned char val);
		unsigned char get_sky_light (int x, int y, int z);
		
		void set_id_and_meta (int x, int y, int z, unsigned short id, unsigned char meta);
		
	//----
		
		/* 
		 * Enqueues an update that should be made to a block in this world
		 * and sent to nearby players.
		 */
		void queue_update (int x, int y, int z, unsigned short id,
			unsigned char meta, player *pl = nullptr);
	};
}

#endif

