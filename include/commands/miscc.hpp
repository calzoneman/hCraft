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

#ifndef _hCraft__COMMANDS__MISCELLANEOUS_H_
#define _hCraft__COMMANDS__MISCELLANEOUS_H_

#include "command.hpp"


namespace hCraft {
	namespace commands {
	
		/* 
		 * /ping -
		 * 
		 * Displays to the player how much time it takes for the server to both
		 * send and retreive a single keep alive (ping: 0x00) packet (in ms).
		 * 
		 * Permissions:
		 *   - command.misc.ping
		 *       Needed to execute the command.
		 */
		class c_ping: public command
		{
		public:
			const char* get_name () { return "ping"; }
		
			const char*
			get_summary ()
				{ return "Checks how much time it takes (in milliseconds) to ping and "
								 "get a response from a player."; }
		
			const char**
			get_usage ()
			{
				static const char *usage[] =
					{
						"/ping",
						"/ping <player>",
						"/ping [--help/--summary]",
						nullptr,
					};
				return usage;
			}
		
			const char**
			get_help ()
			{
				static const char *help[] =
					{
						"Displays the amount of time (in milliseconds) to both send and retreive"
						"a ping packet (keep alive) to/from the calling player.",
				
						"Measures ping time for <player> instead.",
				
						"Same as calling >/help< on >ping< (\"/help [-s] ping\")",
					};
				return help;
			}
		
			const char**
			get_examples ()
			{
				static const char *examples[] =
					{
						"/ping",
						"/ping user1234",
						nullptr,
					};
				return examples;
			}
		
			const char* get_exec_permission () { return "command.misc.ping"; }
		
		//----
			void execute (player *pl, command_reader& reader);
		};
	}
}

#endif

