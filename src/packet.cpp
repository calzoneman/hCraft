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

#include "packet.hpp"
#include "chunk.hpp"
#include "entity.hpp"
#include <cstring>
#include <zlib.h>
#include <cmath>


namespace hCraft {
	
	/* 
	 * Constructs a new packet that can hold up to the specified amount of bytes.
	 */
	packet::packet (unsigned int size)
	{
		this->size = 0;
		this->pos  = 0;
		this->cap  = size;
		this->data = new unsigned char[size];
	}
	
	/* 
	 * Class copy constructor.
	 */
	packet::packet (const packet &other)
	{
		this->size = other.size;
		this->pos  = other.pos;
		this->cap  = other.cap;
		
		this->data = new unsigned char [other.cap];
		std::memcpy (this->data, other.data, other.size);
	}
	
	/* 
	 * Class destructor.
	 */
	packet::~packet ()
	{
		delete[] this->data;
	}
	
	
	
	/* 
	 * put methods:
	 */
	
	void
	packet::put_byte (uint8_t val)
	{
		this->data[(this->pos) ++] = val;
		++ this->size;
	}
	
	void
	packet::put_short (uint16_t val)
	{
		this->data[this->pos + 0] = (val >>  8) & 0xFF;
		this->data[this->pos + 1] = (val      ) & 0xFF;
		
		this->pos += 2; this->size += 2;
	}
	
	void
	packet::put_int (uint32_t val)
	{
		this->data[this->pos + 0] = (val >> 24) & 0xFF;
		this->data[this->pos + 1] = (val >> 16) & 0xFF;
		this->data[this->pos + 2] = (val >>  8) & 0xFF;
		this->data[this->pos + 3] = (val      ) & 0xFF;
		
		this->pos += 4; this->size += 4;
	}
	
	void
	packet::put_long (uint64_t val)
	{
		this->data[this->pos + 0] = (val >> 56) & 0xFF;
		this->data[this->pos + 1] = (val >> 48) & 0xFF;
		this->data[this->pos + 2] = (val >> 40) & 0xFF;
		this->data[this->pos + 3] = (val >> 32) & 0xFF;
		this->data[this->pos + 4] = (val >> 24) & 0xFF;
		this->data[this->pos + 5] = (val >> 16) & 0xFF;
		this->data[this->pos + 6] = (val >>  8) & 0xFF;
		this->data[this->pos + 7] = (val      ) & 0xFF;
		
		this->pos += 8; this->size += 8;
	}
	
	void
	packet::put_float (float val)
	{
		this->put_int (*((uint32_t *)&val));
	}
	
	void
	packet::put_double (double val)
	{
		this->put_long (*((uint64_t *)&val));
	}
	
	void
	packet::put_string (const char *str)
	{
		// don't emit the length of the string, we will do that after we convert
		// the string from UTF-8 to the format used by Minecraft.
		int len_pos = this->pos;
		this->pos += 2;
		
		/* 
		 * Convert UTF-8 to UCS-2:
		 */
		int str_len = std::strlen (str);
		int real_len = 0;
		int i, c, o;
		for (i = 0; i < str_len; ++i)
			{
				c = str[i] & 0xFF;
				if ((c >> 7) == 0)
					{
						this->put_short (c);
						++ real_len;
					}
				else if ((c >> 5) == 0x6)
					{
						o = (c & 0x1F);            // first byte
						c = str[++i] & 0xFF;
						o = (o << 6) | (c & 0x3F); // second byte
						
						this->put_short (o);
						++ real_len;
					}
				else if ((c >> 4) == 0xE)
					{
						o = c & 0xF;               // first byte
						c = str[++i] & 0xFF;
						o = (o << 6) | (c & 0x3F); // second byte
						c = str[++i] & 0xFF;
						o = (o << 6) | (c & 0x3F); // third byte
						
						this->put_short (o);
						++ real_len;
					}
				
				// characters with more than 16 bits of data are not encoded.
			}
		
		// now that we went over the string, we can safely emit its length.
		int cur_pos = this->pos;
		this->pos = len_pos;
		this->put_short (real_len);
		this->pos = cur_pos;
	}
	
	void
	packet::put_bytes (const unsigned char *val, unsigned int len)
	{
		std::memcpy (this->data + this->pos, val, len);
		this->pos += len;
		this->size += len;
	}
	
	
	
//----
	
	static uint16_t
	_read_short (const unsigned char *ptr)
	{
		return ((uint16_t)ptr[0] << 8)
				 | ((uint16_t)ptr[1]);
	}
	
	/* 
	 * Checks the specified byte array and determines how many more bytes should
	 * be read in-order to complete reading the packet. Note that this function
	 * might be called numerous times, since packets often contain variable-
	 * length data (such as strings, slot data, etc...). Aside from checking the
	 * remaining amount of bytes left, the function can also be used to check
	 * whether the first byte of the array (the packet's opcode) is associated
	 * with any valid packet (it returns -1 to indicate that it is not).
	 */
	int
	packet::remaining (const unsigned char *data, unsigned int have)
	{
		static const char* rem_table[] =
			{
			/* 
			 * o = opcode
			 * ? = boolean
			 * b = byte
			 * s = short
			 * i = int
			 * l = long
			 * f = float
			 * d = double
			 * z = string
			 * q = slot
			 * a = array
			 */
			
				"oi"         , ""           , "obzzi"      , "oz"         , // 0x03
				""           , ""           , ""           , "oii?"       , // 0x07
				""           , ""           , "o?"         , "odddd?"     , // 0x0B
				"off?"       , "oddddff?"   , "obibib"     , "oibibqbbb"  , // 0x0F
				
				"os"         , ""           , "oib"        , "oib"        , // 0x13
				""           , ""           , ""           , ""           , // 0x17
				""           , ""           , ""           , ""           , // 0x1B
				""           , ""           , ""           , ""           , // 0x1F
				
				""           , ""           , ""           , ""           , // 0x23
				""           , ""           , ""           , ""           , // 0x27
				""           , ""           , ""           , ""           , // 0x2B
				""           , ""           , ""           , ""           , // 0x2F
				
				""           , ""           , ""           , ""           , // 0x33
				""           , ""           , ""           , ""           , // 0x37
				""           , ""           , ""           , ""           , // 0x3B
				""           , ""           , ""           , ""           , // 0x3F
				
				""           , ""           , ""           , ""           , // 0x43
				""           , ""           , ""           , ""           , // 0x47
				""           , ""           , ""           , ""           , // 0x4B
				""           , ""           , ""           , ""           , // 0x4F
				
				""           , ""           , ""           , ""           , // 0x53
				""           , ""           , ""           , ""           , // 0x57
				""           , ""           , ""           , ""           , // 0x5B
				""           , ""           , ""           , ""           , // 0x5F
				
				""           , ""           , ""           , ""           , // 0x63
				""           , "ob"         , "obs?s?q"    , ""           , // 0x67
				""           , ""           , "obs?"       , "osq"        , // 0x6B
				"obb"        , ""           , ""           , ""           , // 0x6F
				
				""           , ""           , ""           , ""           , // 0x73
				""           , ""           , ""           , ""           , // 0x77
				""           , ""           , ""           , ""           , // 0x7B
				""           , ""           , ""           , ""           , // 0x7F
				
				""           , ""           , "oisizzzz"   , ""           , // 0x83
				""           , ""          , ""           , ""           , // 0x87
				""           , ""           , ""           , ""           , // 0x8B
				""           , ""           , ""           , ""           , // 0x8F
				
				""           , ""           , ""           , ""           , // 0x93
				""           , ""           , ""           , ""           , // 0x97
				""           , ""           , ""           , ""           , // 0x9B
				""           , ""           , ""           , ""           , // 0x9F
				
				""           , ""           , ""           , ""           , // 0xA3
				""           , ""           , ""           , ""           , // 0xA7
				""           , ""           , ""           , ""           , // 0xAB
				""           , ""           , ""           , ""           , // 0xAF
				
				""           , ""           , ""           , ""           , // 0xB3
				""           , ""           , ""           , ""           , // 0xB7
				""           , ""           , ""           , ""           , // 0xBB
				""           , ""           , ""           , ""           , // 0xBF
				
				""           , ""           , ""           , ""           , // 0xC3
				""           , ""           , ""           , ""           , // 0xC7
				""           , ""           , "obbb"       , "oz"         , // 0xCB
				"ozbbb"      , "ob"         , ""           , ""           , // 0xCF
				
				""           , ""           , ""           , ""           , // 0xD3
				""           , ""           , ""           , ""           , // 0xD7
				""           , ""           , ""           , ""           , // 0xDB
				""           , ""           , ""           , ""           , // 0xDF
				
				""           , ""           , ""           , ""           , // 0xE3
				""           , ""           , ""           , ""           , // 0xE7
				""           , ""           , ""           , ""           , // 0xEB
				""           , ""           , ""           , ""           , // 0xEF
				
				""           , ""           , ""           , ""           , // 0xF3
				""           , ""           , ""           , ""           , // 0xF7
				""           , ""           , "oza"        , ""           , // 0xFB
				""           , ""           , "o"          , "oz"         , // 0xFF
			};
		
		const char *str = rem_table[(*data) & 0xFF];
		if (!str[0] || str[0] == ' ')
			return -1;
		
		short tmp;
		
		int c;
		int need = 0;
		while (c = *str++)
			{
				switch (c)
					{
						case 'o': case 'b': case '?': ++ need; break;
						case 's': need += 2; break;
						case 'f': case 'i': need += 4; break;
						case 'd': case 'l': need += 8; break;
						
						// string
						case 'z':
							need += 2;
							if (have < need)
								goto done;
							need += _read_short (data + need - 2) * 2;
							break;
						
						// array (prefixed with 16-bit integer describing length)
						case 'a':
							need += 2;
							if (have < need)
								goto done;
							need += _read_short (data + need - 2);
							break;
						
						// slot data
						case 'q':
							need += 2;
							if (have < need)
								goto done;
							tmp = _read_short (data + need - 2); // id
							
							if (tmp == -1)
								break; // done
							else if (tmp == 0)
								return -1; // shouldn't happen
							
							need += 5;
							if (have < need)
								goto done;
							tmp = _read_short (data + need - 2); // metadata length
							
							if (tmp == -1)
								break; // done
							else if (tmp == 0)
								return -1; // shouldn't happen
							
							need += tmp;
							break;
					}
			}
		
	done:
		return need - have;
	}
	
	
	
//----
	
	/* 
	 * Constructs a new packet reader around the given byte array.
	 */
	packet_reader::packet_reader (const unsigned char *data)
	{
		this->data = data;
		this->pos = 0;
	}
	
	
	
	/* 
	 * read methods:
	 */
	
	uint8_t
	packet_reader::read_byte ()
		{ return this->data[(this->pos) ++]; }
	
	uint16_t
	packet_reader::read_short ()
	{
		return (((uint16_t)read_byte ()) <<  8)
				 | (((uint16_t)read_byte ()));
	}
	
	uint32_t
	packet_reader::read_int ()
	{
		return (((uint32_t)read_byte ()) << 24)
				 | (((uint32_t)read_byte ()) << 16)
				 | (((uint32_t)read_byte ()) <<  8)
				 | (((uint32_t)read_byte ()));
	}
	
	uint64_t
	packet_reader::read_long ()
	{
		return (((uint64_t)read_byte ()) << 56)
				 | (((uint64_t)read_byte ()) << 48)
				 | (((uint64_t)read_byte ()) << 40)
				 | (((uint64_t)read_byte ()) << 32)
				 | (((uint64_t)read_byte ()) << 24)
				 | (((uint64_t)read_byte ()) << 16)
				 | (((uint64_t)read_byte ()) <<  8)
				 | (((uint64_t)read_byte ()));
	}
	
	float
	packet_reader::read_float ()
	{
		uint32_t num = this->read_int ();
		return *((float *)&num);
	}
	
	double 
	packet_reader::read_double ()
	{
		uint64_t num = this->read_long ();
		return *((double *)&num);
	}
	
	int
	packet_reader::read_string (char *out, int max_chars)
	{
		int len, i, c;
		int out_pos = 0;
		
		/* 
		 * Convert from UCS-2 back to UTF-8.
		 */
		len = this->read_short ();
		for (i = 0; i < len; ++i)
			{
				if (i == max_chars)
					{
						out[out_pos] = '\0';
						return -1;
					}
					
				c = this->read_short ();
				if (c < 0x80)
					{
						out[out_pos ++] = c & 0x7F;
					}
				else if (c < 0x800)
					{
						out[out_pos ++] = 0xC0 | (c >> 6);
						out[out_pos ++] = 0x80 | (c & 0x3F);
					}
				else
					{
						out[out_pos ++] = 0xE0 | (c >> 12);
						out[out_pos ++] = 0x80 | ((c >> 6) & 0x3F);
						out[out_pos ++] = 0x80 | (c & 0x3F);
					}
			}
		
		out[out_pos ++] = '\0';
		return len;
	}
	
	
	
//---
	/* 
	 * Packet creation:
	 */
	
	// calculates the total size of an entity metadata dictionary in bytes.
	static int
	entity_metadata_size (entity_metadata &dict)
	{
		int size = 0;
		
		for (auto& rec : dict)
			{
				++ size; // type + index
				switch (rec.type)
					{
						case EMT_BYTE: ++ size; break;
						case EMT_SHORT: size += 2; break;
						case EMT_FLOAT: case EMT_INT: size += 4; break;
						case EMT_STRING:
							size += 2 + (std::strlen (rec.data.str) * 2);
							break;
						case EMT_SLOT: size += 5; break;
						case EMT_BLOCK: size += 12; break;
					}
				
				++ size; // for the trailing `127' byte.
			}
		
		return size;
	}
	
	static void
	encode_entity_metadata (packet *pack, entity_metadata &dict)
	{
		for (auto& rec : dict)
			{
				pack->put_byte ((rec.type << 5) | rec.index);
				switch (rec.type)
					{
						case EMT_BYTE: pack->put_byte (rec.data.i8); break;
						case EMT_SHORT: pack->put_short (rec.data.i16); break;
						case EMT_INT: pack->put_int (rec.data.i32); break;
						case EMT_FLOAT: pack->put_float (rec.data.f32); break;
						case EMT_STRING: pack->put_string (rec.data.str); break;
						case EMT_SLOT:
							pack->put_short (rec.data.slot.id);
							pack->put_byte (rec.data.slot.count);
							pack->put_short (rec.data.slot.damage);
							break;
						case EMT_BLOCK:
							pack->put_int (rec.data.block.x);
							pack->put_int (rec.data.block.y);
							pack->put_int (rec.data.block.z);
							break;
					}
			}
		pack->put_byte (127);
	}
	
	
	
	packet*
	packet::make_ping (int id)
	{
		packet* pack = new packet (5);
		
		pack->put_byte (0x00);
		pack->put_int (id);
		
		return pack;
	}
	
	packet*
	packet::make_login (int eid, const char *level_type, char game_mode,
		char dimension, char difficulty, unsigned char max_players)
	{
		packet *pack = new packet (12 + (std::strlen (level_type) * 2));
		
		pack->put_byte (0x01);
		pack->put_int (eid);
		pack->put_string (level_type);
		pack->put_byte (game_mode);
		pack->put_byte (dimension);
		pack->put_byte (difficulty);
		pack->put_byte (0); // unused
		pack->put_byte (max_players);
		
		return pack;
	}
	
	packet*
	packet::make_message (const char *msg)
	{
		packet *pack = new packet (3 + (std::strlen (msg) * 2));
		
		pack->put_byte (0x03);
		pack->put_string (msg);
		
		return pack;
	}
	
	packet*
	packet::make_spawn_pos (int x, int y, int z)
	{
		packet *pack = new packet (13);
		
		pack->put_byte (0x06);
		pack->put_int (x);
		pack->put_int (y);
		pack->put_int (z);
		
		return pack;
	}
	
	packet*
	packet::make_player_pos_and_look (double x, double y, double z,
		double stance, float r, float l, bool on_ground)
	{
		packet *pack = new packet (42);
		
		pack->put_byte (0x0D);
		pack->put_double (x);
		pack->put_double (stance);
		pack->put_double (y);
		pack->put_double (z);
		pack->put_float (r);
		pack->put_float (l);
		pack->put_byte (on_ground);
		
		return pack;
	}
	
	packet*
	packet::make_spawn_named_entity (int eid, const char *name, double x,
		double y, double z, float r, float l, short current_item,
		entity_metadata& meta)
	{
		packet* pack = new packet (22 + (std::strlen (name) * 2)
			+ entity_metadata_size (meta));
		
		pack->put_byte (0x14);
		pack->put_int (eid);
		pack->put_string (name);
		pack->put_int ((int)(x * 32.0));
		pack->put_int ((int)(y * 32.0));
		pack->put_int ((int)(z * 32.0));
		pack->put_byte ((unsigned char)(r / 360.0f * 255.0));
		pack->put_byte ((unsigned char)(l / 360.0f * 255.0));
		pack->put_short (current_item);
		encode_entity_metadata (pack, meta);
		
		return pack;
	}
	
	packet*
	packet::make_destroy_entity (int eid)
	{
		packet* pack = new packet (6);
		
		pack->put_byte (0x1D);
		pack->put_byte (1); // entity count
		pack->put_int (eid);
		
		return pack;
	}
	
	packet*
	packet::make_entity_relative_move (int eid, char dx, char dy, char dz)
	{
		packet *pack = new packet (8);
		
		pack->put_byte (0x1F);
		pack->put_int (eid);
		pack->put_byte (dx);
		pack->put_byte (dy);
		pack->put_byte (dz);
		
		return pack;
	}
	
	packet*
	packet::make_entity_look (int eid, float r, float l)
	{
		packet *pack = new packet (7);
		
		pack->put_byte (0x20);
		pack->put_int (eid);
		pack->put_byte ((unsigned char)(std::fmod (std::floor (r), 360.0f) / 360.0 * 256.0));
		pack->put_byte ((unsigned char)(std::fmod (std::floor (l), 360.0f) / 360.0 * 256.0));
		
		return pack;
	}
	
	packet*
	packet::make_entity_look_and_move (int eid, char dx, char dy, char dz,
		float r, float l)
	{
		packet* pack = new packet (10);
		
		pack->put_byte (0x21);
		pack->put_int (eid);
		pack->put_byte (dx);
		pack->put_byte (dy);
		pack->put_byte (dz);
		pack->put_byte ((unsigned char)(std::fmod (std::floor (r), 360.0f) / 360.0 * 256.0));
		pack->put_byte ((unsigned char)(std::fmod (std::floor (l), 360.0f) / 360.0 * 256.0));
		
		return pack;
	}
	
	packet*
	packet::make_entity_head_look (int eid, float yaw)
	{
		packet* pack = new packet (6);
		
		pack->put_byte (0x23);
		pack->put_int (eid);
		pack->put_byte ((unsigned char)(std::fmod (std::floor (yaw), 360.0f) / 360.0 * 256.0));
		
		return pack;
	}
	
	packet*
	packet::make_entity_teleport (int eid, int x, int y, int z, float r, float l)
	{
		packet* pack = new packet (19);
		
		pack->put_byte (0x22);
		pack->put_int (eid);
		pack->put_int (x);
		pack->put_int (y);
		pack->put_int (z);
		pack->put_byte ((unsigned char)(std::fmod (std::floor (r), 360.0f) / 360.0 * 256.0));
		pack->put_byte ((unsigned char)(std::fmod (std::floor (l), 360.0f) / 360.0 * 256.0));
		
		return pack;
	}
	
	packet*
	packet::make_chunk (int x, int z, chunk *ch)
	{
		int data_size = 0, n = 0, i;
		unsigned short primary_bitmap = 0, add_bitmap = 0;
		
		// create bitmaps and calculate the size of the uncompressed data array.
		data_size += 256; // biome array
		for (i = 0; i < 16; ++i)
			{
				subchunk *sub = ch->get_sub (i);
				if (sub && !sub->all_air ())
					{
						primary_bitmap |= (1 << i);
						data_size += 10240;
						
						if (sub->has_add ())
							{ add_bitmap |= (1 << i); data_size += 2048; }
					}
			}
		
		unsigned char *data = new unsigned char[data_size];
		
		// fill the array.
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->ids, 4096);
					n += 4096; }
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->meta, 2048);
					n += 2048; }
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->blight, 2048);
					n += 2048; }
		
		for (i = 0; i < 16; ++i)
			if (primary_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->slight, 2048);
					n += 2048; }
		
		for (i = 0; i < 16; ++i)
			if (add_bitmap & (1 << i))
				{ std::memcpy (data + n, ch->get_sub (i)->add, 2048);
					n += 2048; }
		
		std::memcpy (data + n, ch->get_biome_array (), 256);
		n += 256;
		
		// compress.
		unsigned long compressed_size = compressBound (data_size);
		unsigned char *compressed = new unsigned char[compressed_size];
		if (compress2 (compressed, &compressed_size, data, data_size,
			Z_BEST_COMPRESSION) != Z_OK)
			{
				delete[] compressed;
				delete[] data;
				return nullptr;
			}
		
		delete[] data;
		
		// and finally, create the packet.
		packet* pack = new packet (18 + compressed_size);
		
		pack->put_byte (0x33);
		pack->put_int (x);
		pack->put_int (z);
		pack->put_byte (1); // ground-up continuous
		pack->put_short (primary_bitmap);
		pack->put_short (add_bitmap);
		pack->put_int (compressed_size);
		pack->put_bytes (compressed, compressed_size);
		delete[] compressed;
		
		return pack;
	}
	
	packet*
	packet::make_empty_chunk (int x, int z)
	{
		static const unsigned char unload_sequence[] =
			{ 0x78, 0x9C, 0x63, 0x64, 0x1C, 0xD9, 0x00, 0x00, 0x81, 0x80, 0x01, 0x01 };
		
		packet *pack = new packet (18 + (sizeof unload_sequence));
		
		pack->put_byte (0x33);
		pack->put_int (x);
		pack->put_int (z);
		pack->put_byte (1); // ground-up continuous
		pack->put_short (0); // primary bitmap
		pack->put_short (0); // add bitmap
		pack->put_int (sizeof unload_sequence);
		pack->put_bytes (unload_sequence, sizeof unload_sequence);
		
		return pack;
	}
	
	packet*
	packet::make_block_change (int x, unsigned char y, int z,
		unsigned short id, unsigned char meta)
	{
		packet* pack = new packet (13);
		
		pack->put_byte (0x35);
		pack->put_int (x);
		pack->put_byte (y);
		pack->put_int (z);
		pack->put_short (id);
		pack->put_byte (meta);
		
		return pack;
	}
	
	packet*
	packet::make_kick (const char *str)
	{
		packet *pack = new packet (3 + (std::strlen (str) * 2));
		
		pack->put_byte (0xFF);
		pack->put_string (str);
		
		return pack;
	}
}

