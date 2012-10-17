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

#include "permissions.hpp"
#include <cstring>

#include <iostream> // DEBUG


namespace hCraft {
	
	/* 
	 * Class constructor.
	 */
	permission_manager::permission_manager ()
	{
	}
	
	
	
	static int
	get_wildcard (const char *str)
	{
		switch (*str)
			{
				case '*': return PERM_FLAG_ALL;
				case '-': return PERM_FLAG_NONE;
			}
		return -1;
	}
	
	/* 
	 * Registers the specified permission with the manager and returns the
	 * associated permission structure.
	 */
	permission
	permission_manager::add (const char *perm)
	{
		const char *ptr = perm;
		char tmp[33];
		
		permission result {};
		
		int depth = 0;
		for ( ; *ptr != '\0' && depth < 4; ++ depth)
			{
				std::unordered_map<std::string, int>& id_map = this->id_maps[depth];
				std::vector<std::string>& name_map = this->name_maps[depth];
				
				const char *dot = std::strchr (ptr, '.');
				if (dot)
					{
						std::memcpy (tmp, ptr, dot - ptr);
						tmp[dot - ptr] = '\0';
						ptr = dot + 1;
					}
				else
					{
						int len = std::strlen (ptr);
						if (len > 32)
							{
								std::memcpy (tmp, ptr, 32);
								tmp[32] = '\0';
							}
						else
							std::strcpy (tmp, ptr);
						ptr += len;
					}
				
				// test for wildcards
				int wc = get_wildcard (tmp);
				if (wc != -1)
					{
						result.nodes[depth] = wc;
						continue;
					}
				
				auto itr = id_map.find (tmp);
				if (itr != id_map.end ())
					{
						result.nodes[depth] = itr->second;
						continue;
					}
				
				// register new node;
				int id = name_map.size () + PERM_ID_START;
				name_map.push_back (tmp);
				id_map[tmp] = id;
				result.nodes[depth] = id;
			}
		
		for (; depth < 4; ++depth)
			result.nodes[depth] = 0;
		
		return result;
	}
	
	/* 
	 * Returns the permission structure associated with the given string.
	 * If the string does not name a valid permission node, the return value
	 * of the structure's `valid ()' member function will return false.
	 */
	permission
	permission_manager::get (const char *perm)
	{
		const char *ptr = perm;
		char tmp[33];
		
		permission result {};
		
		int depth = 0;
		for ( ; *ptr != '\0' && depth < 4; ++ depth)
			{
				std::unordered_map<std::string, int>& id_map = this->id_maps[depth];
				std::vector<std::string>& name_map = this->name_maps[depth];
				
				const char *dot = std::strchr (ptr, '.');
				if (dot)
					{
						std::memcpy (tmp, ptr, dot - ptr);
						tmp[dot - ptr] = '\0';
						ptr = dot + 1;
					}
				else
					{
						int len = std::strlen (ptr);
						if (len > 32)
							{
								std::memcpy (tmp, ptr, 32);
								tmp[32] = '\0';
							}
						else
							std::strcpy (tmp, ptr);
						ptr += len;
					}
				
				// test for wildcards
				int wc = get_wildcard (tmp);
				if (wc != -1)
					{
						result.nodes[depth] = wc;
						continue;
					}
				
				auto itr = id_map.find (tmp);
				if (itr == id_map.end ())
					{
						result.nodes[0] = -1;
						return result;
					}
				
				result.nodes[depth] = itr->second;
			}
		
		for (; depth < 4; ++depth)
			result.nodes[depth] = 0;
		
		return result;
	}
	
	
	
	/* 
	 * Returns a human-readable representation of the given permission node.
	 */
	std::string
	permission_manager::to_string (permission perm)
	{
		std::string str;
		for (int i = 0; i < 4; ++i)
			{
				int node = perm.nodes[i];
				if (node == 0)
					break;
				
				if (i > 0)
					str.push_back ('.');
				
				if (node == PERM_FLAG_ALL)
					str.push_back ('*');
				else if (node == PERM_FLAG_NONE)
					str.push_back ('-');
				else
					{
						str.append (this->name_maps[i][node - PERM_ID_START]);
					}
			}
		
		return str;
	}
}

