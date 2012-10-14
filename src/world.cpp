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

#include "world.hpp"
#include "player.hpp"
#include "packet.hpp"
#include "utils.hpp"
#include <cstring>
#include <functional>
#include <chrono>


namespace hCraft {
	
	/* 
	 * Constructs a new empty world with the given name.
	 */
	world::world (const char *name, map_generator *gen, map_provider *prov)
		: mp (new map (gen, prov)), players (new playerlist ()),
			update_lock (), updates (),
			
			should_run (true),
			th (std::thread (std::bind (std::mem_fn (&hCraft::world::worker), this)))
	{
		std::strcpy (this->name, name);
	}
	
	/* 
	 * Class destructor.
	 */
	world::~world ()
	{
		this->should_run = false;
		if (this->th.joinable ())
			this->th.join ();
		
		delete this->players;
		delete this->mp;
	}
	
	
	
	/* 
	 * The function ran by the world's thread.
	 */
	void
	world::worker ()
	{
		while (this->should_run)
			{
				{
					std::lock_guard<std::mutex> guard {this->update_lock};
					if (!this->updates.empty ())
						{
							block_update &update = this->updates.front ();
							this->get_map ().set_id_and_meta (update.x, update.y, update.z,
								update.id, update.meta);
							
							chunk *ch = this->get_map ().get_chunk_at (update.x, update.z);
							if (ch)
								{
									ch->relight (utils::mod (update.x, 16), utils::mod (update.z, 16));
									this->get_players ().all (
										[&update] (player *pl)
											{
												pl->send (packet::make_block_change (update.x, update.y,
													update.z, update.id, update.meta));
											}, update.pl);
								}
							
							this->updates.pop ();
						}
				}
				
				std::this_thread::sleep_for (std::chrono::milliseconds (10));
			}
	}
	
	
	
	/* 
	 * Enqueues an update that should be made to a block in the world's
	 * underlying map instance and sent to nearby players.
	 */
	void
	world::queue_update (int x, int y, int z, unsigned short id,
		unsigned char meta, player *pl)
	{
		std::lock_guard<std::mutex> guard {this->update_lock};
		this->updates.emplace (x, y, z, id, meta, pl);
	}
}

