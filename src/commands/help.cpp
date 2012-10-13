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

#include "info.hpp"
#include "../server.hpp"
#include "../player.hpp"


namespace hCraft {
	namespace commands {
		
		const char*
		c_help::get_name ()
			{ return "help"; }
		
		const char*
		c_help::get_summary ()
			{ return "Displays general information about the server or about a specific command."; }
		
		int
		c_help::get_usage_count ()
			{ return 3; }
		
		const char*
		c_help::get_usage (int n)
		{
			static const char *usage[3] =
				{
					"/help",
					"/help [--summary/-s] <command>",
					"/help [--help/--summary]",
				};
			
			return (n >= this->get_usage_count ()) ? "" : usage[n];
		}
		
		const char*
		c_help::get_usage_help (int n)
		{
			static const char *help[3] =
				{
					"Displays general information, tips, tricks and hints about "
					"the server and/or the software that is running it.",
					
					"Shows a detailed description, usage patterns, and examples (if "
					"available) about the specified command (<command>) if --summary is "
					"not specified. Otherwise, only a brief summary about the command "
					"is shown.",
					
					"Same as calling >/help< on >help< (\"/help [-s] help\")",
				};
			
			return (n >= this->get_usage_count ()) ? "" : help[n];
		}
		
		const char**
		c_help::get_examples ()
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
		
		const char*
		c_help::get_permissions ()
			{ return "command.info.help"; }
		
		
		
	//----
		
		void
		c_help::execute (player *pl, command_reader& reader)
		{
			reader.add_option ("help", "h", false, false, false);
			reader.add_option ("summary", "s", false, false, false);
			if (!reader.parse_args (pl))
				return;
			
			if (reader.get_option ("help")->found)
				{ this->show_help (pl); return; }
			else if (reader.get_option ("summary")->found && reader.no_args ())
				{ this->show_summary (pl); return; }
			
			if (reader.arg_count () > 1)
				{ this->show_usage (pl); return; }
			
			if (reader.arg_count () == 1)
				{
					command *cmd = pl->get_server ().get_commands ().find (reader.arg (0).c_str ());
					if (!cmd)
						{
							pl->message ("§c * §eUnable to find help for§f: §c" + reader.arg (0));
							return;
						}
					
					if (reader.get_option ("summary")->found)
						cmd->show_summary (pl);
					else
						cmd->show_help (pl);
					return;
				}
		}
	}
}

