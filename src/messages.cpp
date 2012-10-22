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

#include "messages.hpp"
#include "rank.hpp"
#include "permissions.hpp"
#include <sstream>
#include <vector>
#include <algorithm>


namespace hCraft {
	
	std::string
	messages::insufficient_permissions (group_manager& groups, const char *perm)
	{
		permission_manager& perm_man = groups.get_permission_manager ();
		permission perm_struct       = perm_man.get (perm);
		if (!perm_struct.valid ())
			return "§4 * §cYou are not allowed to do that§7.";
		
		// create a vector of groups sorted in ascending order of power.
		std::vector<group *> sorted_groups;
		for (auto itr = groups.begin (); itr != groups.end (); ++itr)
			sorted_groups.push_back (itr->second);
		std::sort (sorted_groups.begin (), sorted_groups.end (),
			[] (const group *a, const group *b) -> bool	
				{ return (*a) < (*b); });
		
		// find the least powerful group that has the permission
		group *lgrp = nullptr;
		int   count = 0;
		for (group *grp : sorted_groups)
			{
				if (grp->has (perm_struct))
					{
						if (!lgrp)
							lgrp = grp;
						++ count;
					}
			}
		
		if (count == 0)
			return "§4 * §cYou are not allowed to do that§7.";
		
		std::ostringstream ss;
		const char *grp_name = lgrp->get_name ();
		bool insert_n =
			((grp_name[0] == 'a') || (grp_name[0] == 'i') ||
			 (grp_name[0] == 'e') || (grp_name[0] == 'o'));
		
		if (count == 1)
			ss << "§4 * §cYou must be a";
		else
			{
				if ((count == 2) && (lgrp != sorted_groups[sorted_groups.size () - 1])
					&& (sorted_groups[sorted_groups.size () - 1]->has (perm_struct)))
					ss << "§4 * §cYou must be a";
				else
					ss << "§4 * §cYou must be at least a";
			}
				
		if (insert_n) ss << 'n';
		ss << " §" << lgrp->get_color () << grp_name
			 << " §cto do that§7.";
		return ss.str ();
	}
}

