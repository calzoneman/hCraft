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
#include <string>
#include <sstream>


namespace hCraft {
	namespace commands {
		
		const char*
		c_world::get_name ()
			{ return "world"; }
		
		const char**
		c_world::get_aliases ()
		{
			static const char *aliases[] =
				{
					"w",
					nullptr,
				};
			return aliases;
		}
		
		const char*
		c_world::get_summary ()
			{ return "Teleports the player to a different world."; }
		
		int
		c_world::get_usage_count ()
			{ return 3; }
		
		const char*
		c_world::get_usage (int n)
		{
			static const char *usage[3] =
				{
					"/world",
					"/world <world>",
					"/world [--help/--summary]",
				};
			
			return (n >= this->get_usage_count ()) ? "" : usage[n];
		}
		
		const char*
		c_world::get_usage_help (int n)
		{
			static const char *help[3] =
				{
					"Displays the name of the world that the player is currently in.",
					
					"Teleports the player to world <world>.",
					
					"Same as calling >/help< on >world< (\"/help [-s] world\")",
				};
			
			return (n >= this->get_usage_count ()) ? "" : help[n];
		}
		
		const char**
		c_world::get_examples ()
		{
			static const char *examples[] =
				{
					"/world",
					"/world main",
					"/world freebuild-2",
					nullptr,
				};
			
			return examples;
		}
		
		const char**
		c_world::get_permissions ()
		{
			static const char *perms[] =
				{
					"command.world.world",
					nullptr,
				};
			
			return perms;
		}
		
		
		
	//----
		
		void
		c_world::execute (player *pl, command_reader& reader)
		{
			reader.add_option ("help", "h", false, false, false);
			reader.add_option ("summary", "s", false, false, false);
			if (!reader.parse_args (pl))
				return;
			
			if (reader.get_option ("help")->found)
				{ this->show_help (pl); return; }
			else if (reader.get_option ("summary")->found)
				{ this->show_summary (pl); return; }
			
			if (reader.no_args ())
				{
					pl->message ("§eYou are currently in§f: §b" + std::string (pl->get_world ()->get_name ()));
					return;
				}
			else if (reader.arg_count () > 1)
				{ this->show_usage (pl); return; }
			
			std::string& world_name = reader.arg (0);
			world *wr = pl->get_server ().find_world (world_name.c_str ());
			if (!wr)
				{
					pl->message ("§c * §eCannot find world§f: §c" + world_name);
					return;
				}
			
			world *prev_world = pl->get_world ();
			if (wr == prev_world)
				{
					pl->message_nowrap ("§eAlready there§f." );
					return;
				}
			
			pl->join_world (wr);
			
			std::ostringstream leave_ss;
			leave_ss << "§c - §" << pl->rnk.main_group->get_color () <<
				pl->get_username () << " §ehas departed to §9" << prev_world->get_name ()
				<< std::endl;
			std::string leave_msg = leave_ss.str ();
			prev_world->get_players ().all (
				[&leave_msg] (player *pl)
					{
						pl->message (leave_msg);
					});
			
			std::ostringstream enter_ss;
			enter_ss << "§a + §" << pl->rnk.main_group->get_color () <<
				pl->get_username () << " §ehas entered the world" << std::endl;
			std::string enter_msg = enter_ss.str ();
			pl->get_world ()->get_players ().all (
				[&enter_msg] (player *pl)
					{
						pl->message (enter_msg);
					});
		}
	}
}
