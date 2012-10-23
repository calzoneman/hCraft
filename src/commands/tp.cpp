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
#include "../player.hpp"
#include "../server.hpp"
#include <sstream>


namespace hCraft {
	namespace commands {
		
		/* 
		 * /tp -
		 * 
		 * Teleports the player to a requested location.
		 * 
		 * Permissions:
		 *   - command.world.tp
		 *       Needed to execute the command.
		 *   - command.world.tp.others
		 *       Allows the user to teleport players other than themselves.
		 */
		void
		c_tp::execute (player *pl, command_reader& reader)
		{
			if (!pl->perm ("command.world.tp"))
				return;
			
			if (!reader.parse_args (this, pl))
				return;
			
			entity_pos curr_pos = pl->get_pos ();
			
			if (reader.no_args () || reader.arg_count () > 3)
				{ this->show_usage (pl); return; }
			else if (reader.arg_count () == 1)
				{
					/* 
					 * Teleport to a player.
					 *   /tp <player>
					 */
					
					const std::string& target_name = reader.arg (0);
					player *target = pl->get_server ().get_players ().find (
						target_name.c_str ());
					if (!target)
						{
							pl->message ("§c * §ePlayer §b" + target_name + " §eis not online§f.");
							return;
						}
					else if (target == pl)
						{
							pl->message ("§eAlready there§f.");
							return;
						}
					
					// Teleport to the target player's world (if possible).
					if (target->get_world () != pl->get_world ())
						{
							if (!pl->has ("command.world.world"))
								{
									pl->message ("§c * §ePlayer " + std::string (
										target->get_colored_username ()) + " §eis in another world§f.");
									return;
								}
							
							pl->join_world_at (target->get_world (), target->get_pos ());
							return;
						}
					else
						{
							pl->teleport_to (target->get_pos ());
						}
					
					pl->message ("§eTeleported to " + std::string (target->get_colored_username ()));
				}
			else if (reader.arg_count () == 3)
				{
					/* 
					 * Teleport to exact (integer) coordinates.
					 *   /tp <x> <y> <z>
					 */
					int x, y, z;
					
					if (!reader.arg_is_int (0) ||
							!reader.arg_is_int (1) ||
							!reader.arg_is_int (2))
						{
							pl->message ("§eInvalid coordinates §f(§emust be integers§f).");
							return;
						}
					
					x = reader.arg_as_int (0);
					y = reader.arg_as_int (1);
					z = reader.arg_as_int (2);
					
					std::ostringstream ss;
					ss << "§eTeleporting to (§9"
						 << x << "§f, §9"
						 << y << "§f, §9"
						 << z << "§e)";
					pl->message_nowrap (ss.str ());
					
					pl->teleport_to (entity_pos (block_pos (x, y, z))
						.set_rot (curr_pos.r, curr_pos.l));
				}
		}
	}
}

