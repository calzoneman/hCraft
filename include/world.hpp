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

#include "map.hpp"
#include "playerlist.hpp"
#include <thread>
#include <queue>
#include <mutex>


namespace hCraft {
	
	class player;
	
	
	/* 
	 * The whereabouts of a block that must be modified in the world's underlying
	 * map instance and sent to close players.
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
	 * Represents a world.
	 * Wraps around a `map' instance and provides block\entity physics on-top of
	 * it.
	 */
	class world
	{
		char name[33]; // 32 chars max
		map *mp;
		playerlist *players;
		std::thread th;
		bool should_run;
		
		std::queue<block_update> updates;
		std::mutex update_lock;
		
	public:
		inline const char* get_name () { return this->name; }
		inline map& get_map () { return *this->mp; }
		inline playerlist& get_players () { return *this->players; }
		
	private:
		/* 
		 * The function ran by the world's thread.
		 */
		void worker ();
		
	public:
		/* 
		 * Constructs a new empty world with the given name.
		 */
		world (const char *name, map_generator *gen, map_provider *prov);
		
		/* 
		 * Class destructor.
		 */
		~world ();
		
		
		
		/* 
		 * Enqueues an update that should be made to a block in the world's
		 * underlying map instance and sent to nearby players.
		 */
		void queue_update (int x, int y, int z, unsigned short id,
			unsigned char meta, player *pl = nullptr);
	};
}

#endif

