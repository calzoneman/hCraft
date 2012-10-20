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
#include <vector>
#include <memory>
#include <sys/stat.h>

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
	
	/* 
	 * Attempts to determine the type of provider used by the world that has
	 * the specified name (the world must already exist).
	 */
	std::string
	world_provider::determine (const char *path, const char *world_name)
	{
		static std::vector<std::unique_ptr<world_provider_naming>> provs;
		static bool populated = false;
		
		if (!populated)
			{
				provs.emplace_back (new hw_provider_naming ());
				populated = true;
			}
		
		std::string full_name;
		for (std::unique_ptr<world_provider_naming>& ptr : provs)
			{
				world_provider_naming *checker = ptr.get ();
				
				full_name.clear ();
				full_name.append (path);
				if (full_name[full_name.size () - 1] != '/')
					full_name.push_back ('/');
				full_name.append (checker->make_name (world_name));
				
				struct stat st;
				if (stat (full_name.c_str (), &st))
					continue;
				
				bool is_dir = st.st_mode & S_IFDIR;
				bool format_dir = checker->is_directory_format ();
				if ((int)is_dir != (int)format_dir)
					continue;
				
				// match
				return checker->provider_name ();
			}
		
		return std::string ();
	}
}

