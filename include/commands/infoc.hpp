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
		 * /help -
		 * 
		 * When executed without any arguments, the command displays general tips,
		 * tricks and hints about what the player can do in the server. Otherwise,
		 * it displays detailed information about the supplied command.
		 * 
		 * Permissions:
		 *   - command.info.help
		 *       Needed to execute the command.
		 */
		class c_help: public command
		{
		public:
			const char* get_name () { return "help"; }
			
			const char*
			get_summary ()
				{ return "Displays general information about the server or about a "
								 "specific command."; }
			
			const char**
			get_usage ()
			{
				static const char *usage[] =
					{
						"/help",
						"/help [--summary/-s] <command>",
						"/help [--help/--summary]",
						nullptr,
					};
				return usage;
			}
			
			const char**
			get_help ()
			{
				static const char *help[] =
					{
						"Displays general information, tips, tricks and hints about "
						"the server and/or the software that is running it.",
					
						"Shows a detailed description, usage patterns, and examples (if "
						"available) about the specified command (<command>) if --summary is "
						"not specified. Otherwise, only a brief summary about the command "
						"is shown.",
					
						"Same as calling >/help< on >help< (\"/help [-s] help\")",
						nullptr,
					};
				return help;
			}
			
			const char**
			get_examples ()
			{
				static const char *examples[] =
					{
						"/help",
						"/help cuboid",
						"/help -s ping",
						"/help --help",
						nullptr,
					};
				return examples;
			}
			
			const char* get_exec_permission () { return "command.info.help"; }
			
		//----
			void execute (player *pl, command_reader& reader);
		};
	}
}

#endif

