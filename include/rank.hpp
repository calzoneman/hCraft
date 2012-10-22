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

#ifndef _hCraft__RANK_H_
#define _hCraft__RANK_H_

#include "permissions.hpp"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>


namespace hCraft {
	
	/* 
	 * 
	 */
	class group
	{
		int  id;         // unique id
		int  power;      // groups are sorted using this field (higher power = higher-ranked group).
		char name[25];   // 24 chars max
		char col;        // name color.
		
		// 32 chars max
		char prefix[33];
		char suffix[33];
		char mprefix[33]; // used only if the group is the main group.
		char msuffix[33]; // used only if the group is the main group.
		
		bool chat;       // if players of this group can send chat messages.
		bool build;      // whether players of this group can modify blocks.
		bool move;       // whether the players can move.
		
		permission_manager& perm_man;
		std::unordered_set<permission> perms;
		std::vector<group *> parents;
		
	public:
		inline const permission_manager& get_perm_man () const
			{ return this->perm_man; }
		inline std::unordered_set<permission>& get_perms ()
			{ return this->perms; }
		inline const std::vector<group *>& get_parents () const
			{ return this->parents; }
		
	public:
		/* 
		 * Class constructor.
		 */
		group (permission_manager& perm_man, int id, int power, const char *name);
		
		
		/* 
		 * Adds the specified permission node into the group's permissions list.
		 */
		void add (permission perm);
		void add (const char *perm);
		
		/* 
		 * Adds all permission nodes contained within the specified null-terminated
		 * array into the group's permissions list.
		 */
		void add (const char **perms);
		
		
		/* 
		 * Checks whether this group has the given permission node.
		 */
		bool has (permission perm) const;
		bool has (const char *str) const;
		
		
		/* 
		 * Inherits all permissions from the specified group.
		 */
		void inherit (group *grp);
		
		
		/* 
		 * Getter\Setters:
		 */
		
		inline int get_power () const { return this->power; }
		inline const char* get_name () { return this->name; }
		
		inline char get_color () const { return this->col; }
		inline void set_color (char col) { this->col = col ; }
		
		inline const char* get_prefix () const { return this->prefix; }
		void set_prefix (const char *val);
		inline const char* get_mprefix () const { return this->mprefix; }
		void set_mprefix (const char *val);
		
		inline const char* get_suffix () const { return this->suffix; }
		void set_suffix (const char *val);
		inline const char* get_msuffix () const { return this->msuffix; }
		void set_msuffix (const char *val);
		
		inline bool can_chat () const { return this->chat; }
		inline void can_chat (bool val) { this->chat = val; }
		inline bool can_build () const { return this->build; }
		inline void can_build (bool val) { this->build = val; }
		inline bool can_move () const { return this->move; }
		inline void can_move (bool val) { this->move = val; }
		
	//----
		
		inline bool
		operator< (const group& other) const
			{ return this->power < other.power; }
		
		inline bool
		operator> (const group& other) const
			{ return this->power > other.power; }
		
		inline bool
		operator== (const group& other) const
			{ return this->id == other.id; }
		
		inline bool
		operator!= (const group& other) const
			{ return this->id != other.id; }
	};
	
	
	
	class group_manager; // forward def
	
	/* 
	 * 
	 */
	class rank
	{
		std::vector<group *> groups;
		
	public:
		group *main_group;
		
	public:
		inline const std::vector<group *>& get_groups () const
			{ return this->groups; }
		
	public:
		/* 
		 * Constructs an empty rank, that does not hold any groups.
		 */
		rank ();
		
		/* 
		 * Constructs a new rank from the given group string (in the form of
		 * <group1>;<group2>; ... ;<groupN>).
		 */
		rank (const char *group_str, group_manager& groups);
		
		/* 
		 * Copy constructor.
		 */
		rank (const rank& other);
		
		
		/* 
		 * Returns a group string representation of this rank object (groups names
		 * seperated by semicolons).
		 */
		void get_string (std::string& out);
		
		
		/* 
		 * Inserts all the groups contained within the given group string
		 * (in the form of: <group1>;<group2>;...<groupN>). And previous
		 * groups that were held by the rank are removed.
		 */
		void set (const char *group_str, group_manager& groups);
		
		void set (const rank& other);
		void operator= (const rank& other);
		
		
		/* 
		 * Searches through the rank's group list for the highest power field.
		 */
		int power () const;
		
		/* 
		 * Returns the group that has the highest power field.
		 */
		group* highest () const;
		group* main () const
			{ return this->main_group; }
		
		/* 
		 * Checks whether one or more of the groups contained in this rank have
		 * the given permission node registered.
		 */
		bool has (const char *perm) const;
		
	//---
		/* 
		 * Comparison between rank objects:
		 */
		
		bool operator== (const rank& other) const;
		bool operator!= (const rank& other) const;
		bool operator< (const rank& other) const;
		bool operator> (const rank& other) const;
	};
	
	
	
	/* 
	 * Manages a collection of groups.
	 */
	class group_manager
	{
		std::unordered_map<std::string, group *> groups;
		permission_manager& perm_man;
		int id_counter;
		
	public:
		rank default_rank;
		
	public:
		inline std::unordered_map<std::string, group *>::iterator
		begin()
			{ return this->groups.begin (); }
		
		inline std::unordered_map<std::string, group *>::iterator
		end()
			{ return this->groups.end (); }
		
		inline int size () const { return this->groups.size (); }
		
		
		inline permission_manager&
		get_permission_manager ()
			{ return this->perm_man; }
		
	public:
		/* 
		 * Class constructor.
		 */
		group_manager (permission_manager& perm_man);
		
		/* 
		 * Class destructor.
		 */
		~group_manager ();
		
		
		
		/* 
		 * Creates and inserts a new group into the given
		 */
		group* add (int power, const char *name);
		
		/* 
		 * Searches for the group that has the specified name (case-sensitive).
		 */
		group* find (const char *name);
		
		
		/* 
		 * Removes all groups from this manager.
		 */
		void clear ();
	};
}

#endif

