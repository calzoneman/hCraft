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

#ifndef _hCraft__COMMANDS__INFORMATION_H_
#define _hCraft__COMMANDS__INFORMATION_H_

#include "command.hpp"


namespace hCraft {
	namespace commands {
		
		/* 
		 * When executed without any arguments, the command displays general tips,
		 * tricks and hints about what the player can do in the server. Otherwise,
		 * it displays detailed information about the supplied command.
		 */
		class c_help: public command
		{
		public:
			virtual const char* get_name ();
			virtual const char* get_summary ();
			virtual int get_usage_count ();
			virtual const char* get_usage (int n);
			virtual const char* get_usage_help (int n);
			virtual const char** get_examples ();
			virtual const char** get_permissions ();
			virtual void execute (player *pl, command_reader& reader);
		};
	}
}

#endif

