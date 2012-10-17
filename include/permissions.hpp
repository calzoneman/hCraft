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

#ifndef _hCraft__PERMISSIONS_H_
#define _hCraft__PERMISSIONS_H_

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>


namespace hCraft {
	
#define PERM_FLAG_ALL     0x00000001U
#define PERM_FLAG_NONE    0x00000002U
#define PERM_ID_START     3

	/* 
	 * Represents a permission node (<comp1>.<comp2>. ... .<compN>) in a compact
	 * form.
	 */
	class permission
	{
	public:	
		int nodes[4];
		
	public:
		permission ()
		{
			nodes[0] = nodes[1] = nodes[2] = nodes[3] = PERM_ID_START;
		}
		
		inline bool valid () { return this->nodes[0] != -1; }
		
		inline bool
		operator== (const permission& other) const
			{ return (this->nodes[0] == other.nodes[0]) &&
							 (this->nodes[1] == other.nodes[1]) &&
							 (this->nodes[2] == other.nodes[2]) &&
							 (this->nodes[3] == other.nodes[3]); }
	};
	
	
	/* 
	 * Manages a collection of permissions nodes.
	 */
	class permission_manager
	{
		std::unordered_map<std::string, int> id_maps[4];
		std::vector<std::string> name_maps[4];
		
	public:
		/* 
		 * Class constructor.
		 */
		permission_manager ();
		
		
		
		/* 
		 * Registers the specified permission with the manager and returns the
		 * associated permission structure.
		 */
		permission add (const char *perm);
		
		/* 
		 * Returns the permission structure associated with the given string.
		 * If the string does not name a valid permission node, the return value
		 * of the structure's `valid ()' member function will return false.
		 */
		permission get (const char *perm);
		
		/* 
		 * Returns a human-readable representation of the given permission node.
		 */
		std::string to_string (permission perm);
	};
}


namespace std {
	
	template<>
	class hash<hCraft::permission>
	{
		std::hash<int> int_hash;
		
	public:
		size_t
		operator() (const hCraft::permission& perm) const
		{
			return int_hash (
				int_hash (perm.nodes[0]) +
				int_hash (perm.nodes[1]) +
				int_hash (perm.nodes[2]) +
				int_hash (perm.nodes[3]));
		}
	};
}

#endif

