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

#ifndef _hCraft__COMMANDS__CHAT_H_
#define _hCraft__COMMANDS__CHAT_H_

#include "command.hpp"


namespace hCraft {
	namespace commands {
	
		/* 
		 * /me -
		 * 
		 * Broadcasts an IRC-style action message (in the form of: * user1234
		 * <message>).
		 * 
		 * Permissions:
		 *   - command.chat.me
		 *       Needed to execute the command.
		 */
		class c_me: public command
		{
		public:
			const char* get_name () { return "me"; }
			
			const char*
			get_summary ()
				{ return "Broadcasts an IRC-style action message to all players in the "
								"same world."; }
			
			const char**
			get_usage ()
			{
				static const char *usage[] =
					{
						"/me <action>",
						"/me [--help/--summary]",
						nullptr,
					};
				return usage;
			}
			
			const char**
			get_help ()
			{
				static const char *help[] =
					{
						"Broadcasts an IRC-style action message (in the form of: \"* player action\" "
						"to all players in the same world).",
					
						"Same as calling >/help< on >me< (\"/help [-s] me\")",
						nullptr,
					};
				return help;
			}
			
			const char**
			get_examples ()
			{
				static const char *examples[] =
					{
						"/me wants cookies",
						"/me shrugs",
						nullptr,
					};
				return examples;
			}
			
			const char* get_exec_permission () { return "command.chat.me"; }
			
		//----
			void execute (player *pl, command_reader& reader);
		};
	}
}

#endif

