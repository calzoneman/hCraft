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

#include "worldprovider.hpp"
#include <unordered_map>
#include <string>

#include "hwprovider.hpp"


namespace hCraft {
	
	static world_provider*
	create_hw_provider (const char *path, const char *world_name)
		{ return new hw_provider (path, world_name); }
	
	/* 
	 * Returns a new instance of the world provider named @{name}.
	 * @{path} specifies the directory to which the world should be exported to\
	 * imported from.
	 */
	world_provider*
	world_provider::create (const char *name, const char *path,
			const char *world_name)
	{
		static std::unordered_map<std::string, world_provider* (*) (const char *, const char *)> creators {
			{ "hw", create_hw_provider },
		};
		
		auto itr = creators.find (name);
		if (itr != creators.end ())
			return itr->second (path, world_name);
		
		return nullptr;
	}
}

