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

#include "chatc.hpp"
#include "../player.hpp"
#include "../server.hpp"
#include "../sql.hpp"
#include "../rank.hpp"
#include <cstring>
#include <sstream>


namespace hCraft {
	namespace commands {
		
		/* 
		 * /nick -
		 * 
		 * Changes the nickname of a player to the one requested.
		 * 
		 * Permissions:
		 *   - command.chat.nick
		 *       Needed to execute the command.
		 */
		void
		c_nick::execute (player *pl, command_reader& reader)
		{
			if (!pl->perm ("command.chat.nick") || !reader.parse_args (this, pl))
				return;
			
			if (reader.no_args () || reader.arg_count () > 2)
				{ this->show_usage (pl); return; }
			
			std::string target_name = reader.arg (0);
			std::string nickname, prev_nickname;
			
			player *target = pl->get_server ().get_players ().find (target_name.c_str ());
			if (target)
				target_name.assign (target->get_username ());
			
			sql::statement stmt = pl->get_server ().sql ().create (
				"SELECT * FROM `players` WHERE `name` LIKE ?;");
			stmt.bind_text (1, target_name.c_str (), -1, sql::dctor_transient);
			if (stmt.step () != sql::row)
				{
					pl->message ("§c * §eUnable to find player§f: §c" + target_name);
					return;
				}
			
			rank rnk ((const char *)stmt.column_text (2), pl->get_server ().get_groups ());
			prev_nickname.assign ((const char *)stmt.column_text (3));
			
			if (reader.arg_count () == 2)
				nickname.assign (reader.arg (1));
			else
				nickname.assign ((const char *)stmt.column_text (1));
			if (nickname.empty () || nickname.length () > 36)
				{
					pl->message ("§c * §eThe nickname cannot have more than §a36 "
											 "§echaracters§f, and must have at least one§f.");
					return;
				}
			else if (std::strcmp (nickname.c_str (), (const char *)stmt.column_text (3)) == 0)
				{
					pl->message ("§ePlayer §a" + target_name + " §ealready has that nickname§f.");
					return;
				}
			
			{
				std::ostringstream ss;
				if (target)
					{
						pl->set_nickname (nickname.c_str ());
					}
				else
					{
						ss << "UPDATE `players` SET `nick`='"
							 << nickname << "' WHERE `name`='"
							 << target_name << "';";
						pl->get_server ().sql ().execute (ss.str ().c_str ());
					}
				
				ss.clear (); ss.str (std::string ());
				ss << "§" << rnk.main ()->get_color () << prev_nickname
					 << " §eis now known as§f: §" << rnk.main ()->get_color ()
					 << nickname << "§f!";
				pl->get_server ().get_players ().message (ss.str ());
			}
		}
	}
}

