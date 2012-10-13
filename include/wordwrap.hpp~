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

#ifndef _hCraft__WORDWRAP_H_
#define _hCraft__WORDWRAP_H_

#include <vector>
#include <string>


namespace hCraft {
	
	/* 
	 * Collection of static methods that provide word-wrapping.
	 */
	class wordwrap
	{
	public:
		/* 
		 * Performs simple word wrapping on the given string, without doing any
		 * further processing\formatting (except color wrapping, which is done).
		 * The resulting lines are stored in the vector @{out} with lines that
		 * are at most @{max_line} characters long.
		 */
		static void wrap_simple (std::vector<std::string>& out, const char *in,
			int max_line);
		
		/* 
		 * Same as wrap_simple (), but appends the string @{prefix} to all lines
		 * except for the first (unless @{first_line} is true).
		 */
		static void wrap_prefix (std::vector<std::string>& out, const char *in,
			int max_line, const char *prefix, bool first_line = false);
		
		/* 
		 * Counts the number of spaces at the beginning of the string before word-
		 * wrapping. The exact amount is then inserted to the beginning of every
		 * line except the first. If @{remove_from_first} is true, then the leading
		 * spaces are removed from the first line.
		 */
		static void wrap_spaced (std::vector<std::string>& out, const char *in,
			int max_line, bool remove_from_first = false);
		
		/* 
		 * Performs color wrapping on the given line vector.
		 */
		static void wrap_colors (std::vector<std::string>& lines);
	};
}

#endif

