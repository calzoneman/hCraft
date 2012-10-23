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

#include "worldc.hpp"
#include "../server.hpp"
#include "../player.hpp"
#include "../world.hpp"
#include <vector>


namespace hCraft {
	namespace commands {
		
		static bool
		remove_from_autoload (server& srv, const std::string& world_name)
		{
			int count = srv.sql ().scalar_int ((
				"SELECT count(*) FROM `autoloaded-worlds` WHERE `name`='"
				+ world_name + "'").c_str ());
			if (count == 0)
				return false;
			
			srv.sql ().execute ((
				"DELETE FROM `autoloaded-worlds` WHERE `name`='"
				+ world_name + "'").c_str ());
			return true;
		}
		
		
		/* 
		 * /wunload -
		 * 
		 * Saves and removes a requested world from the server's online world world
		 * list, optionally removing it from the autoload list as well.
		 * 
		 * Permissions:
		 *   - command.world.wunload
		 *       Needed to execute the command.
		 */
		void
		c_wunload::execute (player *pl, command_reader& reader)
		{
			if (!pl->perm ("command.world.wunload"))
				return;
			
			reader.add_option ("autoload", "a");
			if (!reader.parse_args (this, pl))
				return;
			
			if (reader.no_args () || reader.arg_count () > 1)
				{ this->show_usage (pl); return; }
			
			std::string world_name = reader.arg (0);
			world *wr = pl->get_server ().find_world (world_name.c_str ());
			if (!wr)
				{
					if (reader.opt ("autoload")->found ())
						{
							if (remove_from_autoload (pl->get_server (), world_name))
								pl->message ("§eWorld §b" + world_name + " §ehas been removed from the autoload list§f.");
							else
								pl->message ("§cWorld §7" + world_name + " §cis not in the autoload list§7.");
						}
					else
						pl->message ("§c * §eWorld §b" + world_name + " §eis not loaded§f.");
					return;
				}
			else if (wr == pl->get_server ().get_main_world ())
				{
					pl->message ("§c * §eYou can not unload the main world§f!");
					return;
				}
			
			world_name.assign (wr->get_name ());
			
			// transfer all players to the server's main world.
			std::vector<player *> to_transfer;
			wr->get_players ().populate (to_transfer);
			for (player *pl : to_transfer)
				pl->join_world (pl->get_server ().get_main_world ());
			pl->get_server ().remove_world (wr);
			
			if (reader.opt ("autoload")->found ())
				{
					if (remove_from_autoload (pl->get_server (), world_name))
						pl->message ("§eWorld §b" + world_name + " §ehas been removed from the autoload list§f.");
					else
						pl->message ("§cWorld §7" + world_name + " §cis not in the autoload list§7.");
				}
			pl->get_server ().get_players ().message (
				"§a * §6World §a" + world_name + " §6has been unloaded§e.");
		}
	}
}
