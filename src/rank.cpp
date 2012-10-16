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

#include "rank.hpp"
#include <cstring>


namespace hCraft {
	
	/* 
	 * Class constructor.
	 */
	group::group (permission_manager& perm_man, int power, const char *name)
		: perm_man (perm_man)
	{
		this->power = power;
		std::strcpy (this->name, name);
		this->col = 'f';
		this->prefix[0] = '\0';
		this->suffix[0] = '\0';
		this->chat = true;
		this->build = true;
		this->move = true;
	}
	
	
	
	void
	group::set_prefix (const char *val)
	{
		int len = std::strlen (val);
		if (len > 32)
			{
				std::memcpy (this->prefix, val, 32);
				this->prefix[32] = '\0';
			}
		else
			std::strcpy (this->prefix, val);
	}
	
	void
	group::set_suffix (const char *val)
	{
		int len = std::strlen (val);
		if (len > 32)
			{
				std::memcpy (this->suffix, val, 32);
				this->suffix[32] = '\0';
			}
		else
			std::strcpy (this->suffix, val);
	}
	
	
	
	/* 
	 * Adds the specified permission node into the group's permissions list.
	 */
	void
	group::add (permission perm)
	{
		// TODO: do not insert duplicates.
		this->perms.insert (perm);
	}
	
	void
	group::add (const char *perm)
	{
		permission res = this->perm_man.get (perm);
		if (!res.valid ())
			return;
		this->add (res);
	}
	
	
	/* 
	 * Adds all permission nodes contained within the specified null-terminated
	 * array into the group's permissions list.
	 */
	void
	group::add (const char **perms)
	{
		const char** ptr = perms;
		while (*ptr)
			this->add (*ptr++);
	}
	
	/* 
	 * Copies and inserts all permissions from the specified group into this
	 * one.
	 */
	void
	group::add (const group& other)
	{
		for (permission p : other.perms)
			this->add (p);
	}
	
	
	
	/* 
	 * Checks whether this group has the given permission node.
	 */
	bool
	group::has (permission perm)
	{
		auto itr = this->perms.find (perm);
		if (itr != this->perms.end ())
			return true;
		
		for (permission p : this->perms)
			{
				int node;
				bool match = true;
				for (int i = 0; i < 4; ++i)
					{
						node = p.nodes[i];
						if (node == PERM_FLAG_ALL)
							{ match = true; break; }
						else if (node == PERM_FLAG_NONE)
							{ match = false; break; }
						
						if (node != perm.nodes[i])
							{ match = false; break; }
					}
				
				if (match)
					return true;
			}
		
		return false;
	}
	
	bool
	group::has (const char *str)
	{
		permission res = this->perm_man.get (str);
		if (!res.valid ())
			return false;
		return this->has (res);
	}
}

