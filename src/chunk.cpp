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

#include "chunk.hpp"
#include <cstring>


namespace hCraft {
	
	/* 
	 * Constructs a new empty subchunk, with all blocks set to air.
	 */
	subchunk::subchunk (bool init)
	{
		if (init)
			{
				std::memset (this->ids, 0x00, 4096);
				std::memset (this->meta, 0x00, 2048);
				std::memset (this->blight, 0x00, 2048);
				std::memset (this->slight, 0xFF, 2048);
				this->add = nullptr;
		
				this->add_count = 0;
				this->air_count = 4096;
			}
	}
	
	/* 
	 * Class destructor.
	 */
	subchunk::~subchunk ()
	{
		if (this->add)
			delete[] this->add;
	}
	
	
	
//----
	
	void
	subchunk::set_id (int x, int y, int z, unsigned short id)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		unsigned int half = index >> 1;
		
		unsigned short lo = id & 0xFF; // the lower 8 bits of the ID.
		unsigned short hi = id >> 8;   // the upper 4 bits of the ID (the "add").
		unsigned short prev_lo = this->ids[index];
		unsigned short prev_hi =
			(this->add_count > 0)
				? ((index & 1) ? (this->add[half] >> 4) : (this->add[half] & 0xF))
				: 0;
		unsigned short prev_id = (prev_hi << 8) | prev_lo;
		
		this->ids[index] = lo;
		if (hi != 0)
			{
				if (this->add_count == 0)
					{
						this->add = new unsigned char[2048];
						std::memset (this->add, 0x00, 2048);
					}
				
				if (index & 1)
					{ this->add[half] &= 0x0F; this->add[half] |= (hi << 4); }
				else
					{ this->add[half] &= 0xF0; this->add[half] |= hi; }
			}
		
		if (prev_id && !id)
			++ this->air_count;
		else if (!prev_id && id)
			-- this->air_count;
		
		if (prev_hi && !hi)
			-- this->add_count;
		else if (!prev_hi && hi)
			++ this->add_count;
	}
	
	unsigned short
	subchunk::get_id (int x, int y, int z)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		
		unsigned short id = this->ids[index];
		if (this->add_count > 0)
			{
				unsigned int half = index >> 1;
				id |=
					((index & 1) ? (this->add[half] >> 4) : (this->add[half] & 0xF))
					<< 8;
			}
		
		return id;
	}
	
	
	void
	subchunk::set_meta (int x, int y, int z, unsigned char val)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		unsigned int half = index >> 1;
		
		if (index & 1)
			{ this->meta[half] &= 0x0F; this->meta[half] |= (val << 4); }
		else
			{ this->meta[half] &= 0xF0; this->meta[half] |= val; }
	}
	
	unsigned char
	subchunk::get_meta (int x, int y, int z)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		unsigned int half = index >> 1;
		
		return (index & 1)
			? (this->meta[half] >> 4)
			: (this->meta[half] & 0xF);
	}
	
	
	void
	subchunk::set_block_light (int x, int y, int z, unsigned char val)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		unsigned int half = index >> 1;
		
		if (index & 1)
			{ this->blight[half] &= 0x0F; this->blight[half] |= (val << 4); }
		else
			{ this->blight[half] &= 0xF0; this->blight[half] |= val; }
	}
	
	unsigned char
	subchunk::get_block_light (int x, int y, int z)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		unsigned int half = index >> 1;
		
		return (index & 1)
			? (this->blight[half] >> 4)
			: (this->blight[half] & 0xF);
	}
	
	
	void
	subchunk::set_sky_light (int x, int y, int z, unsigned char val)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		unsigned int half = index >> 1;
		
		if (index & 1)
			{ this->slight[half] &= 0x0F; this->slight[half] |= (val << 4); }
		else
			{ this->slight[half] &= 0xF0; this->slight[half] |= val; }
	}
	
	unsigned char
	subchunk::get_sky_light (int x, int y, int z)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		unsigned int half = index >> 1;
		
		return (index & 1)
			? (this->slight[half] >> 4)
			: (this->slight[half] & 0xF);
	}
	
	
	void
	subchunk::set_id_and_meta (int x, int y, int z, unsigned short id, unsigned char meta)
	{
		unsigned int index = (y << 8) | (z << 4) | x; 
		unsigned int half = index >> 1;
		
		if (index & 1)
			{ this->meta[half] &= 0x0F; this->meta[half] |= (meta << 4); }
		else
			{ this->meta[half] &= 0xF0; this->meta[half] |= meta; }
		
		unsigned short lo = id & 0xFF; // the lower 8 bits of the ID.
		unsigned short hi = id >> 8;   // the upper 4 bits of the ID (the "add").
		unsigned short prev_lo = this->ids[index];
		unsigned short prev_hi =
			(this->add_count > 0)
				? ((index & 1) ? (this->add[half] >> 4) : (this->add[half] & 0xF))
				: 0;
		unsigned short prev_id = (prev_hi << 8) | prev_lo;

		this->ids[index] = lo;
		if (hi != 0)
			{
				if (this->add_count == 0)
					{
						this->add = new unsigned char[2048];
						std::memset (this->add, 0x00, 2048);
					}
		
				if (index & 1)
					{ this->add[half] &= 0x0F; this->add[half] |= (hi << 4); }
				else
					{ this->add[half] &= 0xF0; this->add[half] |= hi; }
			}

		if (prev_id && !id)
			++ this->air_count;
		else if (!prev_id && id)
			-- this->air_count;

		if (prev_hi && !hi)
			-- this->add_count;
		else if (!prev_hi && hi)
			++ this->add_count;
	}
	
	
	
//----
	
	/* 
	 * Constructs a new empty chunk, with all blocks set to air.
	 */
	chunk::chunk ()
	{
		for (int i = 0; i < 16; ++i)
			{
				this->subs[i] = nullptr;
			}
		
		std::memset (this->heightmap, 0, 256 * sizeof (short));
		std::memset (this->biomes, BI_PLAINS, 256);
		this->modified = true;
	}
	
	/* 
	 * Class destructor.
	 */
	chunk::~chunk ()
	{
		for (int i = 0; i < 16; ++i)
			{
				if (this->subs[i])
					delete this->subs[i];
			}
	}
	
	
	
	/* 
	 * Creates (if does not already exist) and returns the sub-chunk located at
	 * the given vertical position.
	 */
	subchunk*
	chunk::create_sub (int index, bool init)
	{
		subchunk *sub = this->get_sub (index);
		if (sub) return sub;
		
		return (this->subs[index] = new subchunk (init));
	}
	
	
	
//----
	
	void
	chunk::set_id (int x, int y, int z, unsigned short id)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			{
				if (id == 0)
					return;
				else
					sub = this->subs[sy] = new subchunk ();
			}
		
		//if (sub->get_id (x, y & 0xF, z) != id)
			this->modified = true;
		sub->set_id (x, y & 0xF, z, id);
	}
	
	unsigned short
	chunk::get_id (int x, int y, int z)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			return 0;
		
		return sub->get_id (x, y & 0xF, z);
	}
	
	
	void
	chunk::set_meta (int x, int y, int z, unsigned char val)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			{
				if (val == 0)
					return;
				else
					sub = this->subs[sy] = new subchunk ();
			}
		
		//if (sub->get_meta (x, y & 0xF, z) != val)
			this->modified = true;
		sub->set_meta (x, y & 0xF, z, val);
	}
	
	unsigned char
	chunk::get_meta (int x, int y, int z)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			return 0;
		
		return sub->get_meta (x, y & 0xF, z);
	}
	
	
	void
	chunk::set_block_light (int x, int y, int z, unsigned char val)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			{
				if (val == 0)
					return;
				else
					sub = this->subs[sy] = new subchunk ();
			}
		
		//if (sub->get_block_light (x, y & 0xF, z) != val)
			this->modified = true;
		sub->set_block_light (x, y & 0xF, z, val);
	}
	
	unsigned char
	chunk::get_block_light (int x, int y, int z)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			return 0;
		
		return sub->get_block_light (x, y & 0xF, z);
	}
	
	
	void
	chunk::set_sky_light (int x, int y, int z, unsigned char val)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			{
				if (val == 0xF)
					return;
				else
					sub = this->subs[sy] = new subchunk ();
			}
		
		//if (sub->get_sky_light (x, y & 0xF, z) != val)
			this->modified = true;
		sub->set_sky_light (x, y & 0xF, z, val);
	}
	
	unsigned char
	chunk::get_sky_light (int x, int y, int z)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			return 0xF;
		
		return sub->get_sky_light (x, y & 0xF, z);
	}
	
	
	void
	chunk::set_id_and_meta (int x, int y, int z, unsigned short id, unsigned char meta)
	{
		int sy = y >> 4;
		subchunk *sub = this->subs[sy];
		if (!sub)
			{
				if (id == 0) // && meta == 0
					return;
				else
					sub = this->subs[sy] = new subchunk ();
			}
		
		this->modified = true;
		sub->set_id_and_meta (x, y & 0xF, z, id, meta);
	}
	
	
	
//----
	
	/* 
	 * Lighting (re)calculation.
	 */
	
	void
	chunk::relight (int x, int z)
	{
		// the top-most layer is always lit.
		this->set_sky_light (x, 255, z, 15);
		
		char cur_opacity = 15;
		for (int y = 254; y >= 0; --y)
			{
				if (cur_opacity > 0)
					{
						cur_opacity -= block_info::from_id (this->get_id (x, y, z))->opacity;
					}
				this->set_sky_light (x, y, z, (cur_opacity > 0) ? cur_opacity : 0);
			}
	}
	
	void
	chunk::relight ()
	{
		for (int x = 0; x < 16; ++x)
			for (int z = 0; z < 16; ++z)
				{
					this->relight (x, z);
				}
	}
	
	
	
	/* 
	 * (Re)creates the chunk's heightmap.
	 */
	void
	chunk::recalc_heightmap ()
	{
		int y;
		short h;
		
		for (int x = 0; x < 16; ++x)
			for (int z = 0; z < 16; ++z)
				{
					h = 0;
					for (y = 255; y >= 0; --y)
						{
							if (this->get_id (x, y, z) != 0)
								{ h = y + 1; break; }
						}
					this->heightmap[(z << 4) | x] = h;
				}
	}
	
	
	
//----
	
	/* 
	 * Adds the specified entity to the chunk's entity list.
	 */
	void
	chunk::add_entity (entity *e)
	{
		std::lock_guard<std::mutex> guard {this->entity_lock};
		this->entities.insert (e);
	}
	
	/* 
	 * Removes the given entity from the chunk's entity list.
	 */
	void
	chunk::remove_entity (entity *e)
	{
		std::lock_guard<std::mutex> guard {this->entity_lock};
		this->entities.erase (e);
	}
	
	/* 
	 * Calls the specified function on every entity in the chunk's entity list.
	 */
	void
	chunk::all_entities (std::function<void (entity *)> f)
	{
		std::lock_guard<std::mutex> guard {this->entity_lock};
		for (entity* e : this->entities)
			f (e);
	}
}

