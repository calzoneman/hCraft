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

#include "entity.hpp"
#include <cstring>


namespace hCraft {
	
	/* 
	 * Constructs a new metadata record.
	 */
	entity_metadata_record::entity_metadata_record (unsigned char index)
	{
		this->index = index;
		this->data.str = nullptr;
	}
	
	
	
	/* 
	 * `set' methods:
	 */
	
	void
	entity_metadata_record::set_byte (char val)
	{
		this->type = EMT_BYTE;
		this->data.i8 = val;
	}
	
	void
	entity_metadata_record::set_short (short val)
	{
		this->type = EMT_SHORT;
		this->data.i16 = val;
	}
	
	void
	entity_metadata_record::set_int (int val)
	{
		this->type = EMT_INT;
		this->data.i32 = val;
	}
	
	void
	entity_metadata_record::set_float (float val)
	{
		this->type = EMT_FLOAT;
		this->data.f32 = val;
	}
	
	void
	entity_metadata_record::set_string (const char *str)
	{
		this->type = EMT_STRING;
		this->data.str = new char[std::strlen (str) + 1];
		std::strcpy (this->data.str, str);
	}
	
	void
	entity_metadata_record::set_slot (short id, char count, short damage)
	{
		this->type = EMT_SLOT;
		this->data.slot.id = id;
		this->data.slot.count = count;
		this->data.slot.damage = damage;
	}
	
	void
	entity_metadata_record::set_block (int x, int y, int z)
	{
		this->type = EMT_BLOCK;
		this->data.block.x = x;
		this->data.block.y = y;
		this->data.block.z = z;
	}
	
	
	
//----
	
	/* 
	 * Class destructor.
	 */
	entity_metadata::~entity_metadata ()
	{
		for (auto& rec : this->records)
			{
				if (rec.type == EMT_STRING)
					delete[] rec.data.str;
			}
	}
	
	
	
//----
	
	/* 
	 * Constructs a new entity with the specified identification number.
	 */
	entity::entity (int eid)
	{
		this->eid = eid;
	}
	
	/* 
	 * Class destructor.
	 */
	entity::~entity ()
	{
		
	}
	
	
	
	/* 
	 * Constructs metdata records according to the entity's type.
	 */
	void
	entity::build_metadata (entity_metadata& dict)
	{
		unsigned char flags = 0;
		if (this->on_fire) flags |= 0x01;
		if (this->crouched) flags |= 0x02;
		if (this->riding) flags |= 0x04;
		if (this->sprinting) flags |= 0x08;
		if (this->right_action) flags |= 0x10;
		
		entity_metadata_record rec (0);
		rec.set_byte (flags);
		dict.add (rec);
	}
}

