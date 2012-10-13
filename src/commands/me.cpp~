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

#include "chat.hpp"
#include "../server.hpp"
#include "../player.hpp"
#include <sstream>


namespace hCraft {
	namespace commands {
		
		const char*
		c_me::get_name ()
			{ return "me"; }
		
		const char*
		c_me::get_summary ()
			{ return "Broadcasts an IRC-style action message to all players in the same world."; }
		
		int
		c_me::get_usage_count ()
			{ return 2; }
		
		const char*
		c_me::get_usage (int n)
		{
			static const char *usage[2] =
				{
					"/me <action>",
					"/me [--help/--summary]",
				};
			
			return (n >= this->get_usage_count ()) ? "" : usage[n];
		}
		
		const char*
		c_me::get_usage_help (int n)
		{
			static const char *help[2] =
				{
					"Broadcasts an IRC-style action message (in the form of: \"* player action\" "
					"to all players in the same world).",
					
					"Same as calling >/help< on >me< (\"/help [-s] me\")",
				};
			
			return (n >= this->get_usage_count ()) ? "" : help[n];
		}
		
		const char**
		c_me::get_examples ()
		{
			static const char *examples[] =
				{
					"/me wants cookies",
					"/me shrugs",
					nullptr,
				};
			
			return examples;
		}
		
		const char*
		c_me::get_permissions ()
			{ return "command.chat.me"; }
		
		
		
	//----
		
		void
		c_me::execute (player *pl, command_reader& reader)
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
				{ this->show_usage (pl); return; }
			
			std::ostringstream ss;
			ss << "§9* " << pl->get_username () << " " << reader.get_arg_string ();
			std::string out = ss.str ();
			
			pl->get_world ()->get_players ().all (
				[&out] (player *pl)
					{
						pl->message (out);
					});
		}
	}
}

