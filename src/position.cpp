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

#include "position.hpp"


namespace hCraft {
	
	inline int
	fast_floor (double x)
		{ return (x >= 0.0) ? (int)x : ((int)x - 1); }
	
	
	entity_pos::entity_pos (const block_pos &other)
	{
		this->operator= (other);
	}
	
	entity_pos::entity_pos (const chunk_pos &other)
	{
		this->operator= (other);
	}
	
	
	entity_pos&
	entity_pos::operator= (const block_pos &other)
	{
		this->x = other.x;
		this->y = other.y;
		this->z = other.z;
		this->r = 0.0f;
		this->l = 0.0f;
		this->on_ground = true;
	}
	
	entity_pos&
	entity_pos::operator= (const chunk_pos &other)
	{
		this->x = other.x * 16;
		this->y = 0.0;
		this->z = other.z * 16;
		this->r = 0.0f;
		this->l = 0.0f;
		this->on_ground = true;
	}
	
	
	
//----
	
	block_pos::block_pos (const entity_pos &other)
	{
		this->operator= (other);
	}
	
	block_pos::block_pos (const chunk_pos &other)
	{
		this->operator= (other);
	}
	
	
	block_pos&
	block_pos::operator= (const entity_pos &other)
	{
		this->x = fast_floor (other.x);
		this->y = fast_floor (other.y);
		this->z = fast_floor (other.z);
		
		//if (this->y <   0) this->y = 0;
		//if (this->y > 255) this->y = 255;
	}
	
	block_pos&
	block_pos::operator= (const chunk_pos &other)
	{
		this->x = other.x * 16;
		this->y = 0;
		this->z = other.z * 16;
	}
	
	
	
//----
	
	chunk_pos::chunk_pos (const entity_pos &other)
	{
		this->operator= (other);
	}
	
	chunk_pos::chunk_pos (const block_pos &other)
	{
		this->operator= (other);
	}
	
	
	chunk_pos&
	chunk_pos::operator= (const entity_pos &other)
	{
		this->x = fast_floor (other.x) / 16;
		this->z = fast_floor (other.z) / 16;
	}
	
	chunk_pos&
	chunk_pos::operator= (const block_pos &other)
	{
		this->x = other.x / 16;
		this->z = other.z / 16;
	}
}

