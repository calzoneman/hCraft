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
#include <cstring>


namespace hCraft {
	
	/* 
	 * Constructs a new empty world with the given name.
	 */
	world::world (const char *name, map_generator *gen, map_provider *prov)
	{
		std::strcpy (this->name, name);
		this->mp = new map (gen, prov);
		this->players = new playerlist ();
	}
	
	/* 
	 * Class destructor.
	 */
	world::~world ()
	{
		delete this->players;
		delete this->mp;
	}
}

