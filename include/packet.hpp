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

#ifndef _hCraft__PACKET_H_
#define _hCraft__PACKET_H_

#include <cstdint>


namespace hCraft {
	
	class chunk;
	class player;
	class entity_metadata;
	
	
	/* 
	 * A byte array wrapper that provides methods to encode binary data into it.
	 */
	struct packet
	{
		unsigned char *data;
		unsigned int size;
		unsigned int pos;
		unsigned int cap;
		
		/* 
		 * Constructs a new packet that can hold up to the specified amount of bytes.
		 */
		packet (unsigned int size);
		
		/* 
		 * Class copy constructor.
		 */
		packet (const packet &other);
		
		/* 
		 * Class destructor.
		 */
		~packet ();
		
		
		
		/* 
		 * put methods:
		 */
		
		void put_byte (uint8_t val);
		void put_short (uint16_t val);
		void put_int (uint32_t val);
		void put_long (uint64_t val);
		void put_float (float val);
		void put_double (double val);
		void put_string (const char *str);
		void put_bytes (const unsigned char *val, unsigned int len);
		void put_bool (bool val)
			{ put_byte (val ? 1 : 0); }
		
	//----
		
		/* 
		 * Checks the specified byte array and determines how many more bytes should
		 * be read in-order to complete reading the packet. Note that this function
		 * might be called numerous times, since packets often contain variable-
		 * length data (such as strings, slot data, etc...). Aside from checking the
		 * remaining amount of bytes left, the function can also be used to check
		 * whether the first byte of the array (the packet's opcode) is associated
		 * with any valid packet (it returns -1 to indicate that it is not).
		 */
		static int remaining (const unsigned char *data, unsigned int have);
		
		
		
	//---
		/* 
		 * Packet creation:
		 */
		
		static packet* make_ping (int id);
		
		static packet* make_login (int eid, const char *level_type, char game_mode,
			char dimension, char difficulty, unsigned char max_players);
		
		static packet* make_message (const char *msg);
		
		static packet* make_spawn_pos (int x, int y, int z);
		
		static packet* make_player_pos_and_look (double x, double y, double z,
			double stance, float r, float l, bool on_ground);
		
		static packet* make_animation (int eid, char animation);
		
		static packet* make_spawn_named_entity (int eid, const char *name, double x,
			double y, double z, float r, float l, short current_item,
			entity_metadata& meta);
		
		static packet* make_destroy_entity (int eid);
		
		static packet* make_entity_relative_move (int eid, char dx, char dy, char dz);
		
		static packet* make_entity_look (int eid, float r, float l);
		
		static packet* make_entity_look_and_move (int eid, char dx, char dy, char dz,
			float r, float l);
		
		static packet* make_entity_teleport (int eid, int x, int y, int z,
			float r, float l);
		
		static packet* make_entity_head_look (int eid, float yaw);
		
		static packet* make_chunk (int x, int z, chunk *ch);
		
		static packet* make_empty_chunk (int x, int z);
		
		static packet* make_block_change (int x, unsigned char y, int z,
			unsigned short id, unsigned char meta);
		
		static packet* make_player_list_item (const char *name, bool online,
			short ping_ms);
		
		static packet* make_kick (const char *str);
	};
	
	
	/* 
	 * Data decoder for packets.
	 */
	class packet_reader
	{
		const unsigned char *data;
		unsigned int pos;
		
	public:
		/* 
		 * Constructs a new packet reader around the given byte array.
		 */
		packet_reader (const unsigned char *data);
		
		
		inline unsigned int
		seek (unsigned int new_pos)
		{
			unsigned int prev_pos = this->pos;
			this->pos = new_pos;
			return prev_pos;
		}
		
		
		/* 
		 * read methods:
		 */
		
		uint8_t read_byte ();
		uint16_t read_short ();
		uint32_t read_int ();
		uint64_t read_long ();
		float read_float ();
		double read_double ();
		int read_string (char *out, int max_chars = 65535);
	};
}

#endif

