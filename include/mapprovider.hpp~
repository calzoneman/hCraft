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

#ifndef _hCraft__MAPPROVIDER_H_
#define _hCraft__MAPPROVIDER_H_

#include "position.hpp"
#include <vector>
#include <utility>
#include <string>


namespace hCraft {
	
	class chunk;
	class map;
	
	
	/* 
	 * Abstract base class for all map importer\exporter implementations.
	 */
	class map_provider
	{
	public:
		virtual ~map_provider () { }
		
		
		
		/* 
		 * Opens the underlying file stream for reading\writing.
		 * By using open () and close (), multiple chunks can be read\written
		 * without reopening the map file everytime.
		 */
		virtual void open (map &cmap) = 0;
		
		/* 
		 * Closes the underlying file stream.
		 */
		virtual void close () = 0;
		
		
		
		/* 
		 * Saves only the specified chunk.
		 */
		virtual void save (map& cmap, chunk *ch, int x, int z) = 0;
		
		/* 
		 * Saves the specified map without writing out any chunks.
		 */
		virtual void save_empty (map &cmap) = 0;
		
		
		
		/* 
		 * Adds required prefixes, suffixes, etc... to the specified world name so
		 * that the importer's claims_name () function returns true when passed to
		 * it.
		 */
		virtual std::string make_name (const char *world_name) = 0;
		
		/* 
		 * Checks whether the specified path name meets the format required by this
		 * exporter (could be a name prefix, suffix, extension, etc...).
		 */
		virtual bool claims_name (const char *path) = 0;
		
		/* 
		 * Opens the file located at path @{path} and performs a check to see if it
		 * is of the same format created by this exporter.
		 */
		virtual bool claims (const char *path) = 0;
		
		
		
		/* 
		 * Attempts to load the chunk located at the specified coordinates into
		 * @{ch}. Returns true on success, and false if the chunk is not present
		 * within the map file.
		 */
		virtual bool load (map &cmap, chunk *ch, int x, int z) = 0;
		
		
		
		/* 
		 * Returns a new instance of the map provider named @{name}.
		 * @{path} specifies the directory to which the map should be exported to\
		 * imported from.
		 */
		static map_provider* create (const char *name, const char *path,
			const char *world_name);
	};
}

#endif

