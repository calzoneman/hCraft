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

#ifndef _hCraft__UTILS_H_
#define _hCraft__UTILS_H_


namespace hCraft {
	
	/* 
	 * Utility functions and classes.
	 */
	namespace utils {
		
		inline int
		div (int x, int y)
		{
			if (-13 / 5 == -2 && (x < 0) != (y < 0) && x % y != 0)
	  		return x / y - 1;
			return x / y;
		}
		
		inline int
		mod (int x, int y)
		{
			if (-13 / 5 == -2 && (x < 0) != (y < 0) && x % y != 0)
	  		return x % y + y;
			return x % y;
		}
	}
}

#endif

