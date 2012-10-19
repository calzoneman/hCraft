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

#include "flatgrass.hpp"
#include <functional>


namespace hCraft {
	
	/* 
	 * Constructs a new flatgrass world generator.
	 */
	flatgrass_world_generator::flatgrass_world_generator (long seed)
	{
		this->seed = seed;
	}
	
	
	
	/* 
	 * Generates flatgrass terrain on the specified chunk.
	 */
	void
	flatgrass_world_generator::generate (world& wr, chunk *out, int cx, int cz)
	{
		int x, y, z;
		unsigned int xz_hash = std::hash<long> () (((long)cz << 32) | cx) & 0xFFFFFFFF;
		
		this->rnd.seed (this->seed + xz_hash);
		std::uniform_int_distribution<> dist (1, 20);
		
		int height = 64;
		for (x = 0; x < 16; ++x)
			for (z = 0; z < 16; ++z)
				{
					out->set_id (x, 0, z, BT_BEDROCK);
					for (y = 1; y < (height - 5); ++y)
						out->set_id (x, y, z, BT_STONE);
					for (; y < height; ++y)
						out->set_id (x, y, z, BT_DIRT);
					out->set_id (x, y, z, BT_GRASS);
					
					if (dist (this->rnd) > 15)
						out->set_id_and_meta (x, y + 1, z, BT_TALL_GRASS, 1);
					
					out->set_biome (x, z, BI_HELL);
				}
	}
}

