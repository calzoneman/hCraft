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

#include "hwprovider.hpp"
#include "map.hpp"
#include "chunk.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <cctype>
#include <zlib.h>
#include <stdexcept>


namespace hCraft {
	
	inline int
	fast_floor (double x)
		{ return (x >= 0.0) ? (int)x : ((int)x - 1); }
	
	
	
	static unsigned int
	jenkins_hash (const unsigned char *data, unsigned int len)
	{
		unsigned int hash, i;
		for (hash = i = 0; i < len; ++i)
			{
				hash += data[i];
				hash += (hash << 10);
				hash ^= (hash >> 6);
			}
		
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
		
		return hash;
	}
	
	static unsigned int
	hash_coords (int x, int z)
	{
		unsigned char buf[8];
		
		buf[0] = (x) & 0xFF;
		buf[1] = (x >> 8) & 0xFF;
		buf[2] = (x >> 16) & 0xFF;
		buf[3] = (x >> 24) & 0xFF;
		buf[4] = (z) & 0xFF;
		buf[5] = (z >> 8) & 0xFF;
		buf[6] = (z >> 16) & 0xFF;
		buf[7] = (z >> 24) & 0xFF;
		
		return jenkins_hash (buf, 8);
	}
	
	
	
//----
	
	class binary_reader
	{
		std::istream& strm;
		
	public:
		binary_reader (std::istream& strm)
			: strm (strm)
			{ }
		
		//---
		inline void
		seek (std::ostream::off_type off, std::ios_base::seekdir dir = std::ios_base::beg)
			{
				this->strm.seekg (off, dir);
			}
		
		inline std::ostream::pos_type
		tell ()
			{ return this->strm.tellg (); }
		
		//---
		inline unsigned char
		read_byte ()
			{ return strm.get (); }
		
		inline unsigned short
		read_short ()
			{ return ((unsigned short)read_byte ())
						 | ((unsigned short)read_byte () << 8); }
		
		inline unsigned int
		read_int ()
			{ return ((unsigned int)read_byte ())
						 | ((unsigned int)read_byte () << 8)
						 | ((unsigned int)read_byte () << 16)
						 | ((unsigned int)read_byte () << 24); }
		
		inline unsigned long long
		read_long ()
			{ return ((unsigned long long)read_byte ())
						 | ((unsigned long long)read_byte () << 8)
						 | ((unsigned long long)read_byte () << 16)
						 | ((unsigned long long)read_byte () << 24)
						 | ((unsigned long long)read_byte () << 32)
						 | ((unsigned long long)read_byte () << 40)
						 | ((unsigned long long)read_byte () << 48)
						 | ((unsigned long long)read_byte () << 56); }
		
		inline float
		read_float ()
			{ unsigned int num = read_int (); return *((float *)&num); }
		
		inline double
		read_double ()
			{ unsigned long long num = read_long (); return *((double *)&num); }
		
		inline void
		read_bytes (unsigned char *data, unsigned int len)
		{
			this->strm.read ((char *)data, len);
		}
	};
	
	
	
//----
	
	class binary_writer
	{
		std::ostream& strm;
		int written;
		
	public:
		binary_writer () : strm (std::cout) { }
		binary_writer (std::ostream& strm)
			: strm (strm), written (0)
			{ }
		
		//---
		inline void
		seek (std::ostream::off_type off, std::ios_base::seekdir dir = std::ios_base::beg)
			{
				this->strm.seekp (off, dir);
				if (dir == std::ios_base::end)
					this->written = this->tell ();
			}
		
		inline std::ostream::pos_type
		tell ()
			{ return this->strm.tellp (); }
		
		inline void
		flush ()
			{ this->strm.flush (); }
		
		//---
		inline void
		write_byte (unsigned char val)
			{ this->strm.put (val); ++ written; }
		
		inline void
		write_short (unsigned short val)
			{ write_byte (val & 0xFF);
				write_byte ((val >> 8) & 0xFF); }
		
		inline void
		write_int (unsigned int val)
			{ write_byte (val & 0xFF);
				write_byte ((val >> 8) & 0xFF);
				write_byte ((val >> 16) & 0xFF);
				write_byte ((val >> 24) & 0xFF); }
		
		inline void
		write_long (unsigned long long val)
			{ write_byte (val & 0xFF);
				write_byte ((val >> 8) & 0xFF);
				write_byte ((val >> 16) & 0xFF);
				write_byte ((val >> 24) & 0xFF);
				write_byte ((val >> 32) & 0xFF);
				write_byte ((val >> 40) & 0xFF);
				write_byte ((val >> 48) & 0xFF);
				write_byte ((val >> 56) & 0xFF);}
		
		inline void
		write_float (float val)
			{ write_int (*(unsigned int *)&val); }
		
		inline void
		write_double (double val)
			{ write_long (*(unsigned long long *)&val); }
		
		inline void
		write_bytes (const unsigned char *data, unsigned int len)
		{
			this->strm.write ((const char *)data, len);
			this->written += len;
		}
			
	//----
		inline int
		pad_to (int mul)
		{
			if (this->written % mul == 0)
				return 0;
			
			int next = (this->written / mul + 1) * mul;
			int need = next - this->written;
			for (next = 0; next < need; ++next)
				this->write_byte (0);
			return need;
		}
	};
	
	
	
//----
		
	static void read_file (hw_superblock **, binary_reader); // forward def
	
	/* 
	 * Constructs a new map provider for the HWv1 format.
	 */
	hw_provider::hw_provider (const char *path, const char *world_name)
		: out_path (path)
	{
		if (this->out_path[this->out_path.size () - 1] != '/')
			this->out_path.push_back ('/');
		this->out_path.append (this->make_name (world_name));
		
		// read tables if the world file already exists
		{
			std::fstream strm (this->out_path, std::ios_base::in
				| std::ios_base::binary);
			if (strm.is_open ())
				{
					binary_reader reader {strm};
					read_file (this->sblocks, reader);
					strm.close ();
				}
		}
	}
	
	/* 
	 * Class destructor.
	 */
	hw_provider::~hw_provider ()
	{
		for (int i = 0; i < 4096; ++i)
			delete this->sblocks[i];
		this->close ();
	}
	
	
	
	/* 
	 * Opens the underlying file stream for reading\writing.
	 * By using open () and close (), multiple chunks can be read\written
	 * without reopening the map file everytime.
	 */
 	void
 	hw_provider::open (map &cmap)
 	{
 		if (this->strm.is_open ())
 			return;
 		
 		this->strm.open (this->out_path, std::ios_base::binary | std::ios_base::in
 			| std::ios_base::out);
 		if (!this->strm)
 			{
 				this->save_empty (cmap);
 				this->strm.open (this->out_path, std::ios_base::binary | std::ios_base::in
 					| std::ios_base::out);
 			}
 	}
	
	/* 
	 * Closes the underlying file stream.
	 */
	void
	hw_provider::close ()
	{
		if (this->strm.is_open ())
			{
				this->strm.flush ();
				this->strm.close ();
			}
	}
	
	
	
	/* 
	 * Adds required prefixes, suffixes, etc... to the specified world name so
	 * that the importer's claims_name () function returns true when passed to
	 * it.
	 */
	std::string
	hw_provider::make_name (const char *world_name)
	{
		std::string out;
		out.reserve (std::strlen (world_name) + 4);
		
		int c;
		while (c = *world_name++)
			{
				if (c == ' ')
					out.push_back ('_');
				else
					out.push_back (std::tolower (c));
			}
		
		out.append (".hw"); // extension
		
		return out;
	}
	
	/* 
	 * Checks whether the specified path name meets the format required by this
	 * exporter (could be a name prefix, suffix, extension, etc...).
	 */
	bool
	hw_provider::claims_name (const char *path)
	{
		return false;
	}
	
	/* 
	 * Opens the file located at path @{path} and performs a check to see if it
	 * is of the same format created by this exporter.
	 */
	bool
	hw_provider::claims (const char *path)
	{
		return false;
	}
	
	
	
//----
	
	static hw_superblock*
	find_or_create_superblock (int x, int z, hw_superblock **sblocks,
		binary_writer writer, bool create = true)
	{
		unsigned int hash = hash_coords (x, z);
		unsigned int hash_m = hash & 0xFFF;
		
		hw_superblock *sblock = sblocks[hash_m];
		if (sblock != nullptr)
			{
				if (sblock->x == x && sblock->z == z)
					return sblock;
				
				// linear probe
				int i;
				for (i = 0; i < 4096; ++i)
					{
						int m = (hash_m + i) & 0xFFF;
						sblock = sblocks[m];
						if (sblock == nullptr || (sblock->x == x && sblock->z == z))
							break;
					}
				if (i == 4096)
					return nullptr;
				hash_m = (hash_m + i) & 0xFFF;
			}
		if (sblock) return sblock;
		
		if (create)
			{
				sblocks[hash_m] = new hw_superblock (x, z);
				sblock = sblocks[hash_m];
		
				// create the superblock
				writer.seek (0, std::ios_base::end);
				sblock->offset = writer.tell () / 512;
				for (int i = 0; i < 64; ++i)
					{
						writer.write_int (0); // x
						writer.write_int (0); // z
						writer.write_int (0xFFFFFFFFU); // offset
					}
				writer.pad_to (512);
		
				// update file
				writer.seek (512 + (12 * hash_m));
				writer.write_int (x);
				writer.write_int (z);
				writer.write_int (sblock->offset);
			}
		
		return sblock;
	}
	
	static hw_block*
	find_or_create_block (int x, int z, hw_superblock **sblocks,
		binary_writer writer, bool create = true)
	{
		hw_superblock *sblock = find_or_create_superblock (
			fast_floor (x / 8.0), fast_floor (z / 8.0), sblocks,writer, create);
		if (!sblock) return nullptr;
		
		unsigned int hash = hash_coords (x, z);
		unsigned int hash_m = hash & 0x3F;
		
		hw_block *block = sblock->blocks[hash_m];
		if (block != nullptr)
			{
				if (block->x == x && block->z == z)
					return block;
				
				// linear probe
				int i;
				for (i = 0; i < 64; ++i)
					{
						int m = (hash_m + i) & 0x3F;
						block = sblock->blocks[m];
						if (block == nullptr || (block->x == x && block->z == z))
							break;
					}
				if (i == 64)
					return nullptr;
				hash_m = (hash_m + i) & 0x3F;
			}
		if (block) return block;
		
		if (create)
			{
				sblock->blocks[hash_m] = new hw_block (x, z);
				block = sblock->blocks[hash_m];
		
				// create the block
				writer.seek (0, std::ios_base::end);
				block->offset = writer.tell () / 512;
				for (int i = 0; i < 1024; ++i)
					{
						writer.write_int (0); // x
						writer.write_int (0); // z
						writer.write_int (0xFFFFFFFFU); // offset
					}
				writer.pad_to (512);
		
				// update file
				writer.seek ((sblock->offset * 512) + (12 * hash_m));
				writer.write_int (x);
				writer.write_int (z);
				writer.write_int (block->offset);
			}
		
		return block;
	}
	
	static hw_region*
	find_or_create_region (int x, int z, hw_superblock **sblocks,
		binary_writer writer, bool create = true)
	{
		hw_block *block = find_or_create_block (
			fast_floor (x / 32.0), fast_floor (z / 32.0), sblocks, writer, create);
		if (!block) return nullptr;
		
		unsigned int hash = hash_coords (x, z);
		unsigned int hash_m = hash & 0x3FF;
		
		hw_region *region = block->regions[hash_m];
		if (region != nullptr)
			{
				if (region->x == x && region->z == z)
					return region;
				
				// linear probe
				int i;
				for (i = 0; i < 1024; ++i)
					{
						int m = (hash_m + i) & 0x3FF;
						region = block->regions[m];
						if (region == nullptr || (region->x == x && region->z == z))
							break;
					}
				if (i == 1024)
					return nullptr;
				hash_m = (hash_m + i) & 0x3FF;
			}
		if (region) return region;
		
		if (create)
			{
				block->regions[hash_m] = new hw_region (x, z);
				region = block->regions[hash_m];
		
				// create the region
				writer.seek (0, std::ios_base::end);
				region->offset = writer.tell () / 512;
				for (int i = 0; i < 1024; ++i)
					{
						writer.write_int (0); // x
						writer.write_int (0); // z
						writer.write_int (0xFFFFFFFFU); // offset
					}
				writer.pad_to (512);
		
				// update file
				writer.seek ((block->offset * 512) + (12 * hash_m));
				writer.write_int (x);
				writer.write_int (z);
				writer.write_int (region->offset);
			}
		
		return region;
	}
	
	static hw_chunk*
	find_or_create_chunk (int x, int z, hw_superblock **sblocks,
		binary_writer writer, bool create = true)
	{
		hw_region *region = find_or_create_region (
			fast_floor (x / 32.0), fast_floor (z / 32.0), sblocks, writer, create);
		if (!region) return nullptr;
		
		unsigned int hash = hash_coords (x, z);
		unsigned int hash_m = hash & 0x3FF;
		
		hw_chunk *ch = region->chunks[hash_m];
		if (ch != nullptr)
			{
				if (ch->x == x && ch->z == z)
					return ch;
				
				// linear probe
				int i;
				for (i = 0; i < 1024; ++i)
					{
						int m = (hash_m + i) & 0x3FF;
						ch = region->chunks[m];
						if (ch == nullptr || (ch->x == x && ch->z == z))
							break;
					}
				if (i == 1024)
					return nullptr;
				hash_m = (hash_m + i) & 0x3FF;
			}
		if (ch) return ch;
		
		if (create)
			{
				region->chunks[hash_m] = new hw_chunk (x, z);
				ch = region->chunks[hash_m];
		
				// create the chunk
				writer.seek (0, std::ios_base::end);
				ch->offset = writer.tell () / 512;
				
				writer.write_int (ch->size);
				for (int j = 0; j < 256; ++j)
					writer.write_int (ch->sector_table[j]);
					
				writer.pad_to (512);
		
				// update file
				writer.seek ((region->offset * 512) + (12 * hash_m));
				writer.write_int (x);
				writer.write_int (z);
				writer.write_int (ch->offset);
			}
		
		return ch;
	}
	
	
	
//----
	
	static int
	_write_short (unsigned char *ptr, unsigned short val)
	{
		ptr[0] = val & 0xFF;
		ptr[1] = (val >> 8) & 0xFF;
		return 2;
	}
	
	static int
	_write_int (unsigned char *ptr, unsigned int val)
	{
		ptr[0] = val & 0xFF;
		ptr[1] = (val >> 8) & 0xFF;
		ptr[2] = (val >> 16) & 0xFF;
		ptr[3] = (val >> 24) & 0xFF;
		return 4;
	}
	
	
	static unsigned short
	_read_short (const unsigned char *ptr)
	{
		return ((unsigned short)ptr[0])
				 | ((unsigned short)ptr[1] << 8);
	}
	
	static unsigned int
	_read_int (const unsigned char *ptr)
	{
		return ((unsigned int)ptr[0])
				 | ((unsigned int)ptr[1] << 8)
				 | ((unsigned int)ptr[2] << 16)
				 | ((unsigned int)ptr[3] << 24);
	}
	
	
	
//----
	
	static unsigned char*
	make_chunk_data (chunk *ch, unsigned int *out_size)
	{
		unsigned short primary_bitmap = 0, add_bitmap = 0;
		unsigned int   data_size = 0;
		int i;
		
		// calculate size needed for array and create bitmaps
		for (i = 0; i < 16; ++i)
			{
				subchunk *sub = ch->get_sub (i);
				if (sub && !sub->all_air ())
					{
						data_size += 10240;
						primary_bitmap |= (1 << i);
						
						if (sub->has_add ())
							{
								add_bitmap |= (1 << i);
								data_size += 2048;
							}
					}
			}
		data_size += 256; // biome array
		data_size += 4; // bitmaps
		
		/* 
		 * Create and fill the array:
		 */
		unsigned char *data = new unsigned char[data_size];
		unsigned int n = 0;
		
		n += _write_short (data + n, primary_bitmap);
		n += _write_short (data + n, add_bitmap);
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->ids, 4096); n += 4096; }
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->meta, 2048); n += 2048; }
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->blight, 2048); n += 2048; }
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->slight, 2048); n += 2048; }
		
		for (i = 0; i < 16; ++i)
			if (add_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->add, 2048); n += 2048; }
		
		std::memcpy (data + n, ch->get_biome_array (), 256);
		n += 256;
		
		*out_size = n;
		return data;
	}
	
	
	
	static void
	write_to_sector (hw_chunk *hch, unsigned int index, unsigned char *data,
		unsigned int len, binary_writer writer)
	{
		unsigned int sectors_used = hch->size / 4096;
		if (hch->size % 4096 != 0)
			++ sectors_used;
		
		if (index >= sectors_used)
			{
				int sector_offset;
				
				// create the sector
				writer.seek (0, std::ios_base::end);
				sector_offset = writer.tell () / 512;
				writer.write_bytes (data, len);
				if (len < 4096)
					{
						int rem = 4096 - len;
						while (rem-- > 0)
							writer.write_byte (0);
					}
				
				hch->sector_table[index] = sector_offset;
				
				// update file
				writer.seek ((hch->offset * 512) + 4 + (index * 4));
				writer.write_int (sector_offset);
			}
		else
			{
				// overwrite existing sector
				writer.seek (hch->sector_table[index] * 512);
				writer.write_bytes (data, len);
			}
	}
	
	static void
	write_in_sectors (hw_chunk *hch, unsigned char *data, unsigned int data_size,
		binary_writer writer)
	{
		unsigned int sectors_needed = data_size / 4096;
		if (data_size % 4096 != 0)
			++ sectors_needed;
		
		int i, sector_size;
		
		for (i = 0; i < sectors_needed; ++i)
			{
				if (i == (sectors_needed - 1))
					{
						sector_size = data_size % 4096;
						if (sector_size == 0)
							sector_size = 4096;
					}
				else
					sector_size = 4096;
				
				write_to_sector (hch, i, data + (4096 * i), sector_size, writer);
			}
		
		if (hch->size != data_size)
			{
				hch->size = data_size;
				writer.seek (hch->offset * 512);
				writer.write_int (data_size);
			}
	}
	
	static void
	save_chunk (chunk *ch, int x, int z, hw_superblock **sblocks,
		binary_writer writer)
	{
		unsigned char *compressed;
		unsigned long compressed_size;
		
		unsigned int data_size = 0;
		unsigned char *data = make_chunk_data (ch, &data_size);
		
		compressed_size = compressBound (data_size);
		compressed = new unsigned char[compressed_size];
		if (compress2 (compressed, &compressed_size, data, data_size,
			Z_BEST_COMPRESSION) != Z_OK)
			{
				delete[] data;
				delete[] compressed;
				throw std::runtime_error ("failed to compress chunk");
			}
		delete[] data;
		
		hw_chunk *hch = find_or_create_chunk (x, z, sblocks, writer);
		if (hch)
			write_in_sectors (hch, compressed, compressed_size, writer);
		delete[] compressed;
	}
	
	
	
	/* 
	 * Saves only the specified chunk.
	 */
	void
	hw_provider::save (map& cmap, chunk *ch, int x, int z)
	{
		bool close_when_done = false;
		if (!this->strm.is_open ())
			{
				this->open (cmap);
				if (!this->strm)
					return;
				close_when_done = true;
			}
		
		binary_writer writer {this->strm};
		save_chunk (ch, x, z, this->sblocks, writer);
		
		if (close_when_done)
			{
				this->close ();
			}
	}
	
	
	
//----
	
	static void
	save_empty_imp (map &cmap, std::ostream& strm, hw_superblock **sblock_table)
	{
		binary_writer writer (strm);
		
		writer.write_int (0x31765748); // Magic ID ("HWv1")
		
		// world dimensions
		writer.write_int (cmap.get_width ());
		writer.write_int (cmap.get_depth ());
		
		// spawn pos
		auto spawn_pos = cmap.get_spawn ();
		writer.write_double (spawn_pos.x);
		writer.write_double (spawn_pos.y);
		writer.write_double (spawn_pos.z);
		writer.write_float (spawn_pos.r);
		writer.write_float (spawn_pos.l);
		
		writer.write_int (0); // chunk count
		
		writer.pad_to (512);
		
		// super-block table
		for (int i = 0; i < 4096; ++i)
			{
				writer.write_int (0); // x
				writer.write_int (0); // z
				writer.write_int (0xFFFFFFFFU); // offset
			}
		
		writer.flush ();
		
		for (int i = 0; i < 4096; ++i)
			sblock_table[i] = nullptr;
	}
	
	/* 
	 * Saves the specified map without writing out any chunks.
	 */
	void
	hw_provider::save_empty (map &cmap)
	{
		std::ofstream strm (this->out_path, std::ios_base::binary | std::ios_base::out
			| std::ios_base::trunc);
		if (!strm)
			throw std::runtime_error ("failed to open world file");
		
		save_empty_imp (cmap, strm, this->sblocks);
		strm.close ();
	}
	
	
	
//----
	
	static void
	read_tables (hw_superblock **sblocks, binary_reader reader)
	{
		reader.seek (512);
		
		int sb_x, sb_z, sb_offset;
		for (int i = 0; i < 4096; ++i)
			{
				sb_x = reader.read_int ();
				sb_z = reader.read_int ();
				sb_offset = reader.read_int ();
				if (sb_offset == 0xFFFFFFFFU)
					{
						sblocks[i] = nullptr;
						continue;
					}
				
				hw_superblock *sblock = new hw_superblock (sb_x, sb_z);
				sblocks[i] = sblock;
				sblock->offset = sb_offset;
				
				int saved_pos = reader.tell ();
				int b_x, b_z, b_offset;
				reader.seek (sblock->offset * 512);
				for (int i = 0; i < 64; ++i)
					{
						b_x = reader.read_int ();
						b_z = reader.read_int ();
						b_offset = reader.read_int ();
						if (b_offset == 0xFFFFFFFFU)
							{
								sblock->blocks[i] = nullptr;
								continue;
							}
						
						hw_block *block = new hw_block (b_x, b_z);
						sblock->blocks[i] = block;
						block->offset = b_offset;
						
						int saved_pos = reader.tell ();
						int r_x, r_z, r_offset;
						reader.seek (block->offset * 512);
						for (int i = 0; i < 1024; ++i)
							{
								r_x = reader.read_int ();
								r_z = reader.read_int ();
								r_offset = reader.read_int ();
								if (r_offset == 0xFFFFFFFFU)
									{
										block->regions[i] = nullptr;
										continue;
									}
								
								hw_region *region = new hw_region (r_x, r_z);
								block->regions[i] = region;
								region->offset = r_offset;
								
								int saved_pos = reader.tell ();
								int c_x, c_z, c_offset;
								reader.seek (region->offset * 512);
								for (int i = 0; i < 1024; ++i)
									{
										c_x = reader.read_int ();
										c_z = reader.read_int ();
										c_offset = reader.read_int ();
										if (c_offset == 0xFFFFFFFFU)
											{
												region->chunks[i] = nullptr;
												continue;
											}
										
										hw_chunk *ch = new hw_chunk (c_x, c_z);
										region->chunks[i] = ch;
										ch->offset = c_offset;
										
										int saved_pos = reader.tell ();
										reader.seek (c_offset * 512);
										
										ch->size = reader.read_int ();
										for (int i = 0; i < 256; ++i)
											ch->sector_table[i] = reader.read_int ();
										
										reader.seek (saved_pos);
									}
								reader.seek (saved_pos);
							}
						reader.seek (saved_pos);
					}
				reader.seek (saved_pos);
			}
	}
	
	static void
	read_file (hw_superblock **sblocks, binary_reader reader)
	{
		read_tables (sblocks, reader);
	}
	
	
	
	static unsigned char*
	combine_sectors (hw_chunk *hch, binary_reader reader, unsigned int *out_size)
	{
		unsigned int compressed_size = hch->size;
		*out_size = compressed_size;
		
		unsigned char *compressed = new unsigned char[compressed_size];
		unsigned int n = 0;
		
		int sector_index = 0;
		while (compressed_size > 0)
			{
				int need = (compressed_size >= 4096) ? 4096 : compressed_size;
				reader.seek (hch->sector_table[sector_index] * 512);
				reader.read_bytes (compressed + n, need);
				
				n += need;
				compressed_size -= need;
			}
		
		return compressed;
	}
	
	static void
	fill_chunk (chunk *ch, const unsigned char *data)
	{
		unsigned int n = 0, i;
		unsigned short primary_bitmap, add_bitmap;
		
		primary_bitmap = _read_short (data + 0);
		add_bitmap = _read_short (data + 2);
		n += 4;
		
		// create sub-chunks
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{
					subchunk *sub = ch->create_sub (i, false);
					sub->air_count = 4096;
					sub->add_count = 0;
					sub->add = nullptr;
				}
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (ch->get_sub (i)->ids, data + n, 4096); n += 4096; }
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (ch->get_sub (i)->meta, data + n, 2048); n += 2048; }
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (ch->get_sub (i)->blight, data + n, 2048); n += 2048; }
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (ch->get_sub (i)->slight, data + n, 2048); n += 2048; }
		for (i = 0; i < 16; ++i)
			{
				subchunk *sub = ch->get_sub (i);
				if (add_bitmap & (1 << i))
					{
						sub->add = new unsigned char[2048];
						std::memcpy (ch->get_sub (i)->add, data + n, 2048); n += 2048;
					}
			}
		
		// biomes
		std::memcpy (ch->get_biome_array (), data + n, 256);
		n += 256;
		
		// calculate add\air count
		for (i = 0; i < 16; ++i)
			{
				subchunk *sub = ch->get_sub (i);
				if (sub)
					{
						for (int j = 0; j < 4096; ++j)
							{
								if (sub->ids[j] != 0)
									-- sub->air_count;
								if (sub->add && sub->add[j >> 1] != 0)
									{
										if (sub->add[j >> 1] >> 4)
											++ sub->add_count;
										if (sub->add[j >> 1] & 0xF)
											++ sub->add_count;
									}
							}
					}
			}
	}
	
	/* 
	 * Attempts to load the chunk located at the specified coordinates into
	 * @{ch}. Returns true on success, and false if the chunk is not present
	 * within the map file.
	 */
	bool
	hw_provider::load (map &cmap, chunk *ch, int x, int z)
	{
		if (!this->strm.is_open ())
			return false;
		
		binary_writer writer; // not actually used
		hw_chunk *hch = find_or_create_chunk (x, z, this->sblocks, writer, false);
		if (!hch) return false;
		
		binary_reader reader {this->strm};
		
		unsigned int compressed_size = 0;
		unsigned char *compressed = combine_sectors (hch, reader, &compressed_size);
		
		unsigned long data_size = 524288;
		unsigned char *data = new unsigned char[data_size];
		if (uncompress (data, &data_size, compressed, compressed_size) != Z_OK)
			{
				delete[] compressed;
				delete[] data;
				throw std::runtime_error ("failed to decompress chunk");
			}
		delete[] compressed;
		
		fill_chunk (ch, data);
		delete[] data;
		return true;
	}
}

