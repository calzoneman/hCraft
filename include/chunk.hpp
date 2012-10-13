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

#ifndef _hCraft__CHUNK_H_
#define _hCraft__CHUNK_H_

#include "blocks.hpp"
#include <unordered_set>
#include <mutex>
#include <functional>


namespace hCraft {
	
	class entity;
	
	
	/* 
	 * Currently supported biome types.
	 */
	enum biome_type
	{
		BI_OCEAN = 0,
		BI_PLAINS,
		BI_DESERT,
		BI_EXTREME_HILLS,
		BI_FOREST,
		BI_TAIGA,
		BI_SWAMPLAND,
		BI_RIVER,
		BI_HELL,
		BI_SKY,
		BI_FROZEN_OCEAN,
		BI_FROZEN_RIVER,
		BI_ICE_PLAINS,
		BI_ICE_MOUNTAINS,
		BI_MUSHROOM_ISLAND,
		BI_MUSHROOM_ISLAND_SHORE,
		BI_BEACH,
		BI_DESERT_HILLS,
		BI_FOREST_HILLS,
		BI_TAIGA_HILLS,
		BI_EXTREME_HILLS_EDGE,
		BI_JUNGLE,
		BI_JUNGLE_HILLS,
	};
	
	
	/* 
	 * Every chunk is made out of 16 subchunks, each being 16x16x16 in size.
	 */
	struct subchunk
	{
		unsigned char ids[4096];
		unsigned char meta[2048];
		unsigned char blight[2048];
		unsigned char slight[2048];
		unsigned char *add;
		int add_count;
		int air_count;
		
	//----
	
		inline bool all_air () { return this->air_count == 4096; }
		inline bool has_add () { return this->add_count > 0; }
		
	//----
		
		/* 
		 * Constructs a new empty subchunk, with all blocks set to air.
		 */
		subchunk (bool init = true);
		
		/* 
		 * Class destructor.
		 */
		~subchunk ();
		
		
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
	};
	
	
	/* 
	 * The segments that make up a virtually infinite world. 16 blocks wide, 16
	 * blocks long and 256 blocks deep (65,536 blocks total). Each chunk is
	 * divided too into 16 "sub chunks", which are created when the need arises.
	 */
	class chunk
	{
		subchunk *subs[16];
		unsigned char biomes[256];
		short heightmap[256];
		
		std::unordered_set<entity *> entities;
		std::mutex entity_lock;
		
	public:
		bool modified;
		
	public:
		inline subchunk* get_sub (int index) { return this->subs[index]; }
		
		inline unsigned char* get_biome_array () { return this->biomes; }
		inline void set_biome (int x, int z, unsigned char val)
			{ this->biomes[(z << 4) | x] = val; }
		inline unsigned char get_biome (int x, int z)
			{ return this->biomes[(z << 4) | x]; }
		
		inline short get_height (int x, int z) { return this->heightmap[(z << 4) | x]; }
		
	public:
		/* 
		 * Constructs a new empty chunk, with all blocks set to air.
		 */
		chunk ();
		
		/* 
		 * Class destructor.
		 */
		~chunk ();
		
		
		/* 
		 * Creates (if does not already exist) and returns the sub-chunk located at
		 * the given vertical position.
		 */
		subchunk* create_sub (int index, bool init = true);
		
		
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
		 * Lighting (re)calculation.
		 */
		
		void relight (int x, int z);
		void relight ();
		
		
		/* 
		 * (Re)creates the chunk's heightmap.
		 */
		void recalc_heightmap ();
		
	//----
		
		/* 
		 * Adds the specified entity to the chunk's entity list.
		 */
		void add_entity (entity *e);
		
		/* 
		 * Removes the given entity from the chunk's entity list.
		 */
		void remove_entity (entity *e);
		
		/* 
		 * Calls the specified function on every entity in the chunk's entity list.
		 */
		void all_entities (std::function<void (entity *)> f);
	};
}

#endif

