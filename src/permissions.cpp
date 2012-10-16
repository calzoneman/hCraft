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


namespace hCraft {
	
	/* 
	 * Class constructor.
	 */
	permission_manager::permission_manager ()
	{
		this->node_ids[0] = this->node_ids[1] = this->node_ids[2] =
			this->node_ids[3] = 2;
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
		std::unordered_map<std::string, int>* curr_map = this->node_maps;
		while (*ptr != '\0' && depth < 4)
			{
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
						result.nodes[depth++] = wc;
						continue;
					}
				
				auto itr = curr_map->find (tmp);
				if (itr != curr_map->end ())
					{
						result.nodes[depth++] = itr->second;
						continue;
					}
				
				// register new node;
				int id = this->node_ids[depth]++;
				curr_map->operator[] (tmp) = id;
				result.nodes[depth++] = id;
			}
		
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
		std::unordered_map<std::string, int>* curr_map = this->node_maps;
		while (*ptr != '\0' && depth < 4)
			{
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
						result.nodes[depth++] = wc;
						continue;
					}
				
				auto itr = curr_map->find (tmp);
				if (itr == curr_map->end ())
					{
						result.nodes[0] = -1;
						break;
					}
				
				result.nodes[depth++] = itr->second;
			}
		
		return result;
	}
}

