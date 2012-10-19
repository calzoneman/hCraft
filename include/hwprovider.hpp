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

#ifndef _hCraft__HWPROVIDER_H_
#define _hCraft__HWPROVIDER_H_

#include "worldprovider.hpp"
#include <fstream>


namespace hCraft {
	
//----
	struct hw_chunk
	{
		int offset;
		int x;
		int z;
		unsigned int sector_table[256];
		int size;
		
		hw_chunk (int x, int z)
		{
			this->x = x;
			this->z = z;
			this->size = 0;
			for (int i = 0; i < 256; ++i)
				sector_table[i] = 0;
		}
	};
	
	struct hw_region
	{
		int offset;
		int x, z;
		hw_chunk* chunks[1024];
		
		hw_region (int x, int z)
		{
			this->x = x;
			this->z = z;
			for (int i = 0; i < 1024; ++i)
				this->chunks[i] = nullptr;
		}
		
		~hw_region ()
		{
			for (int i = 0; i < 1024; ++i)
				delete this->chunks[i];
		}
	};
	
	struct hw_block
	{
		int offset;
		int x, z;
		hw_region* regions[1024];
		
		hw_block (int x, int z)
		{
			this->x = x;
			this->z = z;
			for (int i = 0; i < 1024; ++i)
				this->regions[i] = nullptr;
		}
		
		~hw_block ()
		{
			for (int i = 0; i < 1024; ++i)
				delete this->regions[i];
		}
	};
	
	struct hw_superblock
	{
		int offset;
		int x, z;
		hw_block* blocks[64];
		
		hw_superblock (int x, int z)
		{
			this->x = x;
			this->z = z;
			for (int i = 0; i < 64; ++i)
				this->blocks[i] = nullptr;
		}
		
		~hw_superblock ()
		{
			for (int i = 0; i < 64; ++i)
				delete this->blocks[i];
		}
	};
//----
	
	
	/* 
	 * World exporter for .hw (hCraft world) formats.
	 */
	class hw_provider: public world_provider
	{
		std::string out_path;
		hw_superblock *sblocks[4096];
		std::fstream strm;
		
	public:
		/* 
		 * Constructs a new world provider for the HWv1 format.
		 */
		hw_provider (const char *path, const char *world_name);
		
		/* 
		 * Class destructor.
		 */
		~hw_provider ();
		
		
		
		/* 
		 * Opens the underlying file stream for reading\writing.
		 * By using open () and close (), multiple chunks can be read\written
		 * without reopening the world file everytime.
		 */
		virtual void open (world &wr);
		
		/* 
		 * Closes the underlying file stream.
		 */
		virtual void close ();
		
		
		
		/* 
		 * Adds required prefixes, suffixes, etc... to the specified world name so
		 * that the importer's claims_name () function returns true when passed to
		 * it.
		 */
		virtual std::string make_name (const char *world_name);
		
		/* 
		 * Checks whether the specified path name meets the format required by this
		 * exporter (could be a name prefix, suffix, extension, etc...).
		 */
		virtual bool claims_name (const char *path);
		
		/* 
		 * Opens the file located at path @{path} and performs a check to see if it
		 * is of the same format created by this exporter.
		 */
		virtual bool claims (const char *path);
		
		
		
		/* 
		 * Saves only the specified chunk.
		 */
		virtual void save (world& wr, chunk *ch, int x, int z);
		
		/* 
		 * Saves the specified world without writing out any chunks.
		 */
		virtual void save_empty (world &wr);
		
		
		
		/* 
		 * Attempts to load the chunk located at the specified coordinates into
		 * @{ch}. Returns true on success, and false if the chunk is not present
		 * within the world file.
		 */
		virtual bool load (world &wr, chunk *ch, int x, int z);
	};
}

#endif

