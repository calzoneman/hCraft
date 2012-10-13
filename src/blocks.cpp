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

#include "blocks.hpp"
#include <cstring>
#include <vector>


namespace hCraft {
	
	block_info::block_info (unsigned short id, const char *name,
		float blast_resistance, char opacity, char luminance, char max_stack)
	{
		this->id = id;
		this->blast_resistance = blast_resistance;
		this->opacity = opacity;
		this->luminance = luminance;
		this->max_stack = max_stack;
		std::strcpy (this->name, name);
	}
	
	
//----
	
	static std::vector<block_info> block_list {
		{ 0x00, "air", 0.0f, 0, 0, 64 },
		{ 0x01, "stone", 30.0f, 15, 0, 64 },
		{ 0x02, "grass", 3.0f, 15, 0, 64 },
		{ 0x03, "dirt", 2.5f, 15, 0, 64 },
		{ 0x04, "cobble", 30.0f, 15, 0, 64 },
		{ 0x05, "wood", 15.0f, 15, 0, 64 },
		{ 0x06, "sapling", 0.0f, 0, 0, 64 },
		{ 0x07, "bedrock", 18000000.0f, 15, 0, 64 },
		{ 0x08, "water", 500.0f, 3, 0, 64 },
		{ 0x09, "still-water", 500.0f, 3, 0, 64 },
		{ 0x0A, "lava", 0.0f, 0, 0, 64 },
		{ 0x0B, "still-lava", 500.0f, 0, 0, 64 },
		{ 0x0C, "sand", 2.5f, 15, 0, 64 },
		{ 0x0D, "gravel", 3.0f, 15, 0, 64 },
		{ 0x0E, "gold-ore", 15.0f, 15, 0, 64 },
		{ 0x0F, "iron-ore", 15.0f, 15, 0, 64 },
		{ 0x10, "coal-ore", 15.0f, 15, 0, 64 },
		{ 0x11, "trunk", 10.0f, 15, 0, 64 },
		{ 0x12, "leaves", 1.0f, 0, 0, 64 },
		{ 0x13, "sponge", 3.0f, 15, 0, 64 },
		{ 0x14, "glass", 1.5f, 0, 0, 64 },
		{ 0x15, "lapis-ore", 15.0f, 15, 0, 64 },
		{ 0x16, "lapis-block", 15.0f, 15, 0, 64 },
		{ 0x17, "dispenser", 17.5f, 15, 0, 64 },
		{ 0x18, "sandstone", 4.0f, 15, 0, 64 },
		{ 0x19, "note-block", 4.0f, 15, 0, 64 },
		{ 0x1A, "bed", 1.0f, 0, 0, 64 },
		{ 0x1B, "powered-rail", 3.5f, 0, 0, 64 },
		{ 0x1C, "detector-rail", 3.5f, 0, 0, 64 },
		{ 0x1D, "sticky-piston", 2.5f, 0, 0, 64 },
		{ 0x1E, "cobweb", 20.0f, 0, 0, 64 },
		{ 0x1F, "tall-grass", 0.0f, 0, 0, 64 },
		{ 0x20, "dead-bush", 0.0f, 0, 0, 64 },
		{ 0x21, "piston", 2.5f, 0, 0, 64 },
		{ 0x22, "piston-extension", 2.5f, 0, 0, 64 },
		{ 0x23, "wool", 4.0f, 15, 0, 64 },
		{ 0x24, "piston-move", 0.0f, 0, 0, 64 },
		{ 0x25, "dandelion", 0.0f, 0, 0, 64 },
		{ 0x26, "rose", 0.0f, 0, 0, 64 },
		{ 0x27, "brown-mushroom", 0.0f, 0, 0, 64 },
		{ 0x28, "red-mushroom", 0.0f, 0, 0, 64 },
	};
	
	
	
	/* 
	 * Returns the block_info structure describing the block associated with the
	 * specified ID number.
	 */
	block_info*
	block_info::from_id (unsigned short id)
	{
		if (id >= block_list.size ())
			return nullptr;
		return &block_list[id];
	}
}

