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
#include "../worldprovider.hpp"
#include "../worldgenerator.hpp"


namespace hCraft {
	namespace commands {
		
		static bool
		add_to_autoload (server& srv, const std::string& world_name)
		{
			int count = srv.sql ().scalar_int (
				("SELECT count(*) FROM `autoloaded-worlds` WHERE `name`='"
				+ world_name + "';").c_str ());
			if (count != 0)
				return false;
			
			srv.sql ().execute (
				("INSERT INTO `autoloaded-worlds` (`name`) VALUES ('"
				+ world_name + "');").c_str ());
			return true;
		}
		
		
		/* 
		 * /wload -
		 * 
		 * Loads a world from disk onto the server's world list. Doing this enables
		 * players to go to that world using /w.
		 * 
		 * Permissions:
		 *   - command.world.wload
		 *       Needed to execute the command.
		 */
		void
		c_wload::execute (player *pl, command_reader& reader)
		{
			if (!pl->perm ("command.world.wload"))
				return;
			
			reader.add_option ("autoload", "a");
			if (!reader.parse_args (this, pl))
				return;
			
			if (reader.no_args () || reader.arg_count () > 1)
				{ this->show_usage (pl); return; }
			
			std::string& world_name = reader.arg (0);
			world *twr = pl->get_server ().find_world (world_name.c_str ());
			if (twr)
				{
					if (reader.opt ("autoload")->found ())
						{
							if (add_to_autoload (pl->get_server (), world_name))
								pl->message ("§eWorld §b" + world_name + " §ehas been added to the autoload list§f.");
							else
								pl->message ("§cWorld §7" + world_name + " §cis already autoloaded§7.");
						}
					else
						pl->message ("§c * §eWorld §b" + world_name + " §eis already loaded§f.");
					return;
				}
			
			std::string prov_name = world_provider::determine ("worlds", world_name.c_str ());
			if (prov_name.empty ())
				{
					pl->message ("§c * §eWorld §b" + world_name + " §edoes not exist§f.");
					return;
				}
			
			world_provider *prov = world_provider::create (prov_name.c_str (),
				"worlds", world_name.c_str ());
			if (!prov)
				{
					pl->message ("§c * ERROR§f: §eInvalid provider§f.");
					return;
				}
			
			const world_information& winf = prov->info ();
			world_generator *gen = world_generator::create (winf.generator.c_str (), winf.seed);
			if (!gen)
				{
					pl->message ("§c * ERROR§f: §eInvalid generator§f.");
					return;
				}
			
			pl->get_logger () () << "Loading world \"" << world_name << "\"" << std::endl;
			world *wr = new world (world_name.c_str (), gen, prov);
			wr->set_size (winf.width, winf.depth);
			wr->prepare_spawn (10);
			wr->start ();
			if (!pl->get_server ().add_world (wr))
				{
					pl->get_logger () (LT_ERROR) << "Failed to load world \"" << world_name << "\": Already loaded." << std::endl;
					pl->message ("§c * ERROR§f: §eFailed to load world§f.");
					delete wr;
					return;
				}
			
			// add to autoload list
			if (reader.opt ("autoload")->found ())
				{
					if (add_to_autoload (pl->get_server (), world_name))
						pl->message ("§eWorld §b" + world_name + " §ehas been added to the autoload list§f.");
					else
						pl->message ("§cWorld §7" + world_name + " §cis already autoloaded§7.");
				}
			
			pl->get_server ().get_players ().message (
				"§a * §6World §a" + world_name + " §6has been loaded§f.");
		}
	}
}

