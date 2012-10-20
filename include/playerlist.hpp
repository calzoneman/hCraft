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

#ifndef _hCraft__PLAYERLIST_H_
#define _hCraft__PLAYERLIST_H_

#include "cistring.hpp"
#include "packet.hpp"
#include <unordered_map>
#include <mutex>
#include <functional>
#include <string>


namespace hCraft {
	
	class player;
	
	
	enum class player_find_method
	{
		case_sensitive,
		case_insensitive,
		name_completion,
	};
	
	
	/* 
	 * A thread-safe list of players that provides a multitude of methods to
	 * acquire a subset of that list according to certain criteria.
	 */
	class playerlist
	{
		std::unordered_map<cistring, player *> players;
		std::mutex lock;
		
	public:
		/* 
		 * Constructs a new empty player list.
		 */
		playerlist ();
		
		/* 
		 * Class copy constructor.
		 */
		playerlist (const playerlist& other);
		
		/* 
		 * Class destructor.
		 */
		~playerlist ();
		
		
		/* 
		 * Returns the number of players contained in this list.
		 */
		int count ();
		
		
		/* 
		 * Adds the specified player into the player list.
		 * Returns false if a player with the same name already exists in the list;
		 * true otherwise.
		 */
		bool add (player *pl);
		
		/* 
		 * Removes the player that has the specified name from this player list.
		 * NOTE: the search is done using case-INsensitive comparison.
		 */
		void remove (const char *name, bool delete_player = false);
		
		/* 
		 * Removes the specified player from this player list.
		 */
		void remove (player *pl, bool delete_player = false);
		
		/* 
		 * Removes all players from this player list.
		 */
		void clear (bool delete_players = false);
		
		/* 
		 * Searches the player list for a player that has the specified name.
		 * Uses the given method to determine if names match.
		 */
		player* find (const char *name,
			player_find_method method = player_find_method::case_insensitive);
		
		
		
		/* 
		 * Calls the function @{f} on all players in this list.
		 */
		void all (std::function<void (player *)> f, player* except = nullptr);
		
		/* 
		 * Calls the function @{f} on all players visible to player @{target} with
		 * exception to @{target} itself.
		 */
		void all_visible (std::function<void (player *)> f, player *target);
		
		/* 
		 * Iterates through the list, and passes all players to the specified
		 * predicate function. Players that produce a positive value are
		 * removed from the list, and can be optinally destroyed as well.
		 */
		void remove_if (std::function<bool (player *)> pred,
			bool delete_players = false);
		
		
		
		/* 
		 * Broadcasts the given message to all players in this list.
		 */
		void message (const char *msg, player *except = nullptr);
		void message (const std::string& msg, player *except = nullptr);
		
		/* 
		 * Sends the specified packet to all players in this list.
		 */
		void send_to_all (packet *pack, player *except = nullptr);
	};
}

#endif

