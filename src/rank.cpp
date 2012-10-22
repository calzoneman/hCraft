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

#include "rank.hpp"
#include <cstring>
#include <sstream>
#include <string>
#include <stdexcept>


namespace hCraft {
	
	/* 
	 * Class constructor.
	 */
	group::group (permission_manager& perm_man, int id, int power, const char *name)
		: perm_man (perm_man)
	{
		this->id = id;
		this->power = power;
		std::strcpy (this->name, name);
		this->col = 'f';
		this->prefix[0] = this->mprefix[0] = '\0';
		this->suffix[0] = this->msuffix[0] = '\0';
		this->chat = true;
		this->build = true;
		this->move = true;
	}
	
	
	
	void
	group::set_prefix (const char *val)
	{
		int len = std::strlen (val);
		if (len > 32)
			{
				std::memcpy (this->prefix, val, 32);
				this->prefix[32] = '\0';
			}
		else
			std::strcpy (this->prefix, val);
	}
	
	void
	group::set_mprefix (const char *val)
	{
		int len = std::strlen (val);
		if (len > 32)
			{
				std::memcpy (this->mprefix, val, 32);
				this->mprefix[32] = '\0';
			}
		else
			std::strcpy (this->mprefix, val);
	}
	
	void
	group::set_suffix (const char *val)
	{
		int len = std::strlen (val);
		if (len > 32)
			{
				std::memcpy (this->suffix, val, 32);
				this->suffix[32] = '\0';
			}
		else
			std::strcpy (this->suffix, val);
	}
	
	void
	group::set_msuffix (const char *val)
	{
		int len = std::strlen (val);
		if (len > 32)
			{
				std::memcpy (this->msuffix, val, 32);
				this->msuffix[32] = '\0';
			}
		else
			std::strcpy (this->msuffix, val);
	}
	
	
	
	/* 
	 * Adds the specified permission node into the group's permissions list.
	 */
	void
	group::add (permission perm)
	{
		// TODO: do not insert duplicates.
		this->perms.insert (perm);
	}
	
	void
	group::add (const char *perm)
	{
		permission res = this->perm_man.add (perm);
		this->add (res);
	}
	
	
	/* 
	 * Adds all permission nodes contained within the specified null-terminated
	 * array into the group's permissions list.
	 */
	void
	group::add (const char **perms)
	{
		const char** ptr = perms;
		while (*ptr)
			this->add (*ptr++);
	}
	
	
	
	/* 
	 * Inherits all permissions from the specified group.
	 */
	void
	group::inherit (group *grp)
	{
		if (!grp || grp == this)
			return;
		this->parents.push_back (grp);
	}
	
	
	
	/* 
	 * Checks whether this group has the given permission node.
	 */
	bool
	group::has (permission perm) const
	{
		auto itr = this->perms.find (perm);
		if (itr != this->perms.end ())
			return true;
		
		for (permission p : this->perms)
			{
				int node;
				bool match = true;
				for (int i = 0; i < 5; ++i)
					{
						node = p.nodes[i];
						if (node == -1)
							break;
						
						if (node == PERM_FLAG_ALL)
							{ match = true; break; }
						else if (node == PERM_FLAG_NONE)
							{ match = false; break; }
						
						if (node != perm.nodes[i])
							{ match = false; break; }
					}
				
				if (match)
					return true;
			}
		
		for (group *grp : this->parents)
			if (grp->has (perm))
				return true;
		
		return false;
	}
	
	bool
	group::has (const char *str) const
	{
		permission res = this->perm_man.get (str);
		if (!res.valid ())
			return false;
		return this->has (res);
	}
	
	
	
//----
	
	/* 
	 * Class constructor.
	 */
	group_manager::group_manager (permission_manager& perm_man)
		: perm_man (perm_man)
	{
		this->id_counter = 0;
	}
	
	/* 
	 * Class destructor.
	 */
	group_manager::~group_manager ()
	{
		this->clear ();
	}
	
	
	
	/* 
	 * Creates and inserts a new group into the given
	 */
	group*
	group_manager::add (int power, const char *name)
	{
		std::string nm {name};
		auto itr = this->groups.find (nm);
		if (itr != this->groups.end ())
			{ return nullptr; }
		
		group* grp = new group (this->perm_man, (this->id_counter)++, power, name);
		this->groups[std::move (nm)] = grp;
		return grp;
	}
	
	/* 
	 * Searches for the group that has the specified name (case-sensitive).
	 */
	group*
	group_manager::find (const char *name)
	{
		auto itr = this->groups.find (name);
		if (itr != this->groups.end ())
			return itr->second;
		return nullptr;
	}
	
	
	
	/* 
	 * Removes all groups from this manager.
	 */
	void
	group_manager::clear ()
	{
		for (auto itr = this->groups.begin (); itr != this->groups.end (); ++itr)
			{
				group *grp = itr->second;
				delete grp;
			}
		this->groups.clear ();
	}
	
	
	
//----
	
	/* 
	 * Constructs an empty rank, that does not hold any groups.
	 */
	rank::rank ()
		{ this->main_group = nullptr; }
	
	/* 
	 * Constructs a new rank from the given group string (in the form of
	 * <group1>;<group2>; ... ;<groupN>).
	 */
	rank::rank (const char *group_str, group_manager& groups)
	{
		this->set (group_str, groups);
	}
	
	/* 
	 * Copy constructor.
	 */
	rank::rank (const rank& other)
	{
		this->set (other);
	}
	
	
	
	/* 
	 * Returns a group string representation of this rank object (groups names
	 * seperated by semicolons).
	 */
	void
	rank::get_string (std::string& out)
	{
		out.clear ();
		for (group *grp : this->groups)
			{
				if (!out.empty ())
					out.push_back (';');
				if (grp == this->main_group)
					out.push_back ('@');
				out.append (grp->get_name ());
			}
	}
	
	
	
	/* 
	 * Inserts all the groups contained within the given group string
	 * (in the form of: <group1>;<group2>;...<groupN>). And previous
	 * groups that were held by the rank are removed.
	 */
	void
	rank::set (const char *group_str, group_manager& groups)
	{
		this->groups.clear ();
		this->main_group = nullptr;
		
		std::stringstream ss {group_str};
		std::string str;
		while (std::getline (ss, str, ';'))
			{
				bool is_main = false;
				if (str[0] == '@')
					{
						str.erase (0, 1);
						is_main = true;
					}
				
				group *grp = groups.find (str.c_str ());
				if (!grp)
					throw std::runtime_error ("group \"" + str + "\" does not exist");
				this->groups.push_back (grp);
				if (is_main)
					this->main_group = grp;
			}
		
		if (!this->main_group && !this->groups.empty ())
			this->main_group = this->groups[0];
	}
	
	void
	rank::set (const rank& other)
	{
		if (this == &other)
			return;
		
		this->groups = other.groups;
		this->main_group = other.main_group;
	}
	
	void
	rank::operator= (const rank& other)
	{
		this->set (other);
	}
	
	
	
	/* 
	 * Searches through the rank's group list for the highest power field.
	 */
	int
	rank::power () const
	{
		group *grp = this->highest ();
		return grp ? grp->get_power () : 0;
	}
	
	/* 
	 * Returns the group that has the highest power field.
	 */
	group*
	rank::highest () const
	{
		if (this->groups.size () == 0)
			return nullptr;
		
		group *max = this->groups[0];
		for (auto itr = this->groups.begin () + 1; itr != this->groups.end (); ++itr)
			{
				group *grp = *itr;
				if (grp->get_power () > max->get_power ())
					max = grp;
			}
		
		return max;
	}
	
	
	/* 
	 * Checks whether one or more of the groups contained in this rank have
	 * the given permission node registered.
	 */
	bool
	rank::has (const char *perm) const
	{
		if (this->groups.empty ())
			return false;
		
		const permission_manager& perm_man = this->groups[0]->get_perm_man ();
		permission perm_struct = perm_man.get (perm);
		if (!perm_struct.valid ())
			return false;
		
		for (group *grp : this->groups)
			{
				if (grp->has (perm_struct))
					return true;
			}
		
		return false;
	}
	
	
	
	/* 
	 * Comparison between rank objects:
	 */
	
	bool
	rank::operator== (const rank& other) const
	{
		return (this->groups == other.groups);
	}
	
	bool
	rank::operator!= (const rank& other) const
	{
		return (this->groups != other.groups);
	}
	
	bool
	rank::operator< (const rank& other) const
	{
		return (this->power () < other.power ());
	}
	
	bool
	rank::operator> (const rank& other) const
	{
		return (this->power () > other.power ());
	}
}

