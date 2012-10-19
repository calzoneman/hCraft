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

#include "worldgenerator.hpp"
#include <unordered_map>
#include <string>
#include <chrono>

// generators:
#include "flatgrass.hpp"


namespace hCraft {
	
	void
	world_generator::generate_edge (world& wr, chunk *out)
	{
		int x, y, z;
		int height = 64;
		
		for (x = 0; x < 16; ++x)
			for (z = 0; z < 16; ++z)
				{
					for (y = 0; y < height; ++y)
						out->set_id (x, y, z, BT_BEDROCK);
					out->set_id (x, y, z, BT_STILL_WATER);
					
					out->set_biome (x, z, BI_HELL);
				}
	}
	
	
	
//----
	
	static world_generator*
	create_flatgrass (long seed)
		{ return new flatgrass_world_generator (seed); }
	
	
	/* 
	 * Finds and instantiates a new world generator from the given name.
	 */
	world_generator*
	world_generator::create (const char *name, long seed)
	{
		static std::unordered_map<std::string, world_generator* (*) (long)> generators {
				{ "flatgrass", create_flatgrass },
			};
		
		auto itr = generators.find (name);
		if (itr != generators.end ())
			return itr->second (seed);
		return nullptr;
	}
	
	world_generator*
	world_generator::create (const char *name)
	{
		return world_generator::create (name,
			std::chrono::system_clock::to_time_t (
				std::chrono::system_clock::now ()) & 0xFFFFFFFF);
	}
}

