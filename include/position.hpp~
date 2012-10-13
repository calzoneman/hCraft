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

#ifndef _hCraft__POSITION_H_
#define _hCraft__POSITION_H_

#include <functional>


namespace hCraft {
	
	class entity_pos;
	class block_pos;
	class chunk_pos;
	
	
	
	/* 
	 * Represents the position of an entity within a world.
	 */
	struct entity_pos
	{
		double x;
		double y;
		double z;
		
		float r; // rotation (yaw)
		float l; // look (pitch)
		
		bool on_ground;
		
	//----
		
		entity_pos (double nx = 0.0, double ny = 0.0, double nz = 0.0,
			float nr = 0.0f, float nl = 0.0f, bool ng = true)
			: x (nx), y (ny), z (nz), r (nr), l (nl), on_ground (ng)
			{ }
		
		// conversion constructors:
		entity_pos (const block_pos &other);
		entity_pos (const chunk_pos &other);
		
		// assignment conversions:
		entity_pos& operator= (const block_pos &other);
		entity_pos& operator= (const chunk_pos &other);
		
	//----
		
		entity_pos&
		set_pos (double nx, double ny, double nz)
		{
			this->x = nx;
			this->y = ny;
			this->z = nz;
			return *this;
		}
		
		entity_pos&
		set_rot (float nr, float nl)
		{
			this->r = nr;
			this->l = nl;
			return *this;
		}
		
		entity_pos&
		set (double nx, double ny, double nz, float nr, float nl, bool ng)
		{
			this->x = nx;
			this->y = ny;
			this->z = nz;
			this->r = nr;
			this->l = nl;
			this->on_ground = ng;
			return *this;
		}
		
	//----
		
		bool
		operator== (const entity_pos& other)
			{
				return (this->x == other.x) && (this->y == other.y) &&
							 (this->z == other.z) && (this->r == other.r) && 
							 (this->l == other.l) && (this->on_ground == other.on_ground); 
			}
		
		bool operator!= (const entity_pos& other) { return !(this->operator== (other)); }
	};
	
	
	/* 
	 * The position of a block.
	 */
	struct block_pos
	{
		int x;
		int y;
		int z;
		
	//----
		
		block_pos (int nx = 0, int ny = 0, int nz = 0)
			: x (nx), y (ny), z (nz)
			{ }
		
		// conversion constructors:
		block_pos (const entity_pos &other);
		block_pos (const chunk_pos &other);
		
		// assignment conversions:
		block_pos& operator= (const entity_pos &other);
		block_pos& operator= (const chunk_pos &other);
		
	//----
		
		block_pos&
		set (int nx, int ny, int nz)
		{
			this->x = nx;
			this->y = ny;
			this->z = nz;
			return *this;
		}
	};
	
	
	/* 
	 * The position of a chunk.
	 */
	struct chunk_pos
	{
		int x;
		int z;
		
	//----
		
		chunk_pos (int nx = 0, int nz = 0)
			: x (nx), z (nz)
			{ }
		
		// conversion constructors:
		chunk_pos (const entity_pos &other);
		chunk_pos (const block_pos &other);
		
		// assignment conversions:
		chunk_pos& operator= (const entity_pos &other);
		chunk_pos& operator= (const block_pos &other);
		
		bool
		operator== (const chunk_pos other) const
			{ return (this->x == other.x) && (this->z == other.z); }
		
	//----
		
		chunk_pos&
		set (int nx, int nz)
		{
			this->x = nx;
			this->z = nz;
			return *this;
		}
	};
	
	
	
//----
	
	class chunk_pos_hash
	{
		std::hash<int> int_hash;
		
	public:
		std::size_t
		operator() (const chunk_pos& cpos) const
		{
			return int_hash (cpos.x) ^ (int_hash (cpos.z) << 5);
		}
	};
}

#endif

