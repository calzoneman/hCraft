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

#ifndef _hCraft__ENTITY_H_
#define _hCraft__ENTITY_H_

#include "position.hpp"
#include <vector>


namespace hCraft {
	
//----
	/* 
	 * Entity metadata.
	 * NOTE: This is a pretty experimental implementation.
	 */
	
	enum entity_metadata_type
	{
		EMT_BYTE,
		EMT_SHORT,
		EMT_INT,
		EMT_FLOAT,
		EMT_STRING,
		EMT_SLOT,
		EMT_BLOCK,
	};
	
	struct entity_metadata_record
	{
		unsigned char type;
		unsigned char index;
		
		union
			{
				char  i8;
				short i16;
				int   i32;
				float f32;
				char* str; // heap-allocated.
				
				struct
					{
						short id;
						char  count;
						short damage;
					} slot;
				
				struct
					{
						int x;
						int y;
						int z;
					} block;
			} data;
		
	public:
		/* 
		 * Constructs a new metadata record.
		 */
		entity_metadata_record (unsigned char index);
		
		/* 
		 * `set' methods:
		 */
		
		void set_byte (char val);
		void set_short (short val);
		void set_int (int val);
		void set_float (float val);
		void set_string (const char *str);
		void set_slot (short id, char count, short damage);
		void set_block (int x, int y, int z);
	};
	
	/* 
	 * The entity metadata "dictionary".
	 */
	class entity_metadata
	{
		std::vector<entity_metadata_record> records;
		
	public:
		inline std::vector<entity_metadata_record>::iterator
		begin ()
			{ return records.begin (); }
			
		inline std::vector<entity_metadata_record>::iterator
		end ()
			{ return records.end (); }
			
	public:
		entity_metadata () { }
		entity_metadata (const entity_metadata &) = delete;
		
		/* 
		 * Class destructor.
		 */
		~entity_metadata ();
		
		
		void
		add (entity_metadata_record rec)
			{ this->records.push_back (rec); }
	};
	
//----
	
	
	
	/* 
	 * Supported types of entities.
	 */
	enum entity_type
	{
		ET_PLAYER,
		ET_MOB,
		ET_BOAT,
		ET_MINECART,
		ET_ITEM,
		ET_EXPERIENCE_ORB,
		ET_ARROW,
		ET_THROWABLE, // snowballs and chicken eggs
		ET_ENDER_PEARL,
		ET_EYE_OF_ENDER,
		ET_TNT,
		ET_FALLING, // sand/gravel/dragon egg
		ET_FISHING_ROD_BOBBER,
		ET_FIREBALL,
		ET_ENDER_CRYSTAL,
	};
	
	
	/* 
	 * An entity can be any movable dynamic object in a world that encompasses
	 * a certain "state" (e.g.: players, mobs, minecarts, etc...).
	 */
	class entity
	{
	protected:
		int eid;
		entity_pos pos;
		
		bool on_fire;
		bool crouched;
		bool riding;
		bool sprinting;
		bool right_action;
		
	public:
		inline int get_eid () { return this->eid; }
		inline entity_pos get_pos () { return this->pos; }
		inline void set_pos (const entity_pos& other) { this->pos = other; }
		
		inline bool is_on_fire () { return this->on_fire; }
		inline bool is_crouched () { return this->crouched; }
		inline bool is_riding () { return this->riding; }
		inline bool is_sprinting () { return this->sprinting; }
		inline bool is_right_action () { return this->right_action; }
		
	public:
		/* 
		 * Constructs a new entity with the specified identification number.
		 */
		entity (int eid);
		
		// copy constructor.
		entity (const entity&) = delete;
		
		/* 
		 * Class destructor.
		 */
		virtual ~entity ();
		
		
		
		/* 
		 * Returns the type of this entity.
		 */
		virtual entity_type get_type () = 0;
		
		/* 
		 * Constructs metdata records according to the entity's type.
		 */
		virtual void build_metadata (entity_metadata& dict);
	};
}

#endif

