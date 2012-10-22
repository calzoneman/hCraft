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

#include "infoc.hpp"
#include "../server.hpp"
#include "../player.hpp"


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
		 *       To execute the command.
		 */
		void
		c_help::execute (player *pl, command_reader& reader)
		{
			if (!pl->perm ("command.info.help"))
				return;
			
			// we handle --help and --summary ourselves, instead of passing the work
			// to the command reader.
			reader.add_option ("help", "h");
			reader.add_option ("summary", "s");
			if (!reader.parse_args (this, pl, false))
				return;
			
			if (reader.opt ("summary")->found () && reader.no_args ())
				{ this->show_summary (pl); return; }
			else if (reader.opt ("help")->found ())
				{ this->show_help (pl); return; }
			
			if (reader.arg_count () > 1)
				{ this->show_usage (pl); return; }
			else if (reader.arg_count () == 1)
				{
					command *cmd = pl->get_server ().get_commands ().find (reader.arg (0).c_str ());
					if (!cmd || !pl->has (cmd->get_exec_permission ()))
						{
							pl->message ("§c * §eUnable to find help for§f: §c" + reader.arg (0));
							return;
						}
					
					if (reader.opt ("summary")->found ())
						cmd->show_summary (pl);
					else
						cmd->show_help (pl);
					return;
				}
		}
	}
}

