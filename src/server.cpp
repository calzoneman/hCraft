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

#include "server.hpp"
#include <memory>
#include <fstream>
#include <cstring>
#include <yaml-cpp/yaml.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <algorithm>


namespace hCraft {
	
	// constructor.
	server::initializer::initializer (std::function<void ()>&& init,
		std::function<void ()>&& destroy)
		: init (std::move (init)), destroy (std::move (destroy))
		{ this->initialized = false; }
	
	// move constructor.
	server::initializer::initializer (initializer&& other)
		: init (std::move (other.init)), destroy (std::move (other.destroy)),
			initialized (other.initialized)
		{ }
	
	server::initializer::initializer (const initializer &other)
		: init (other.init), destroy (other.destroy), initialized (other.initialized)
		{ }
	
	
	
	// constructor.
	server::worker::worker (struct event_base *base, std::thread&& th)
		: evbase (base), th (std::move (th)), event_count (0)
		{ }
	
	// move constructor.
	server::worker::worker (worker&& w)
		: evbase (w.evbase), th (std::move (w.th)),
			event_count (w.event_count.load ())
		{ }
	
	
	
	/* 
	 * Constructs a new server.
	 */
	server::server (logger &log)
		: log (log), perms (), groups (perms)
	{
		// add <init, destory> pairs
		
		this->inits.push_back (initializer (
			std::bind (std::mem_fn (&hCraft::server::init_config), this),
			std::bind (std::mem_fn (&hCraft::server::destroy_config), this)));
		
		this->inits.push_back (initializer (
			std::bind (std::mem_fn (&hCraft::server::init_sql), this),
			std::bind (std::mem_fn (&hCraft::server::destroy_sql), this)));
		
		this->inits.push_back (initializer (
			std::bind (std::mem_fn (&hCraft::server::init_core), this),
			std::bind (std::mem_fn (&hCraft::server::destroy_core), this)));
		
		this->inits.push_back (initializer (
			std::bind (std::mem_fn (&hCraft::server::init_commands), this),
			std::bind (std::mem_fn (&hCraft::server::destroy_commands), this)));
		
		this->inits.push_back (initializer (
			std::bind (std::mem_fn (&hCraft::server::init_ranks), this),
			std::bind (std::mem_fn (&hCraft::server::destroy_ranks), this)));
		
		this->inits.push_back (initializer (
			std::bind (std::mem_fn (&hCraft::server::init_worlds), this),
			std::bind (std::mem_fn (&hCraft::server::destroy_worlds), this)));
		
		this->inits.push_back (initializer (
			std::bind (std::mem_fn (&hCraft::server::init_workers), this),
			std::bind (std::mem_fn (&hCraft::server::destroy_workers), this)));
		
		this->inits.push_back (initializer (
			std::bind (std::mem_fn (&hCraft::server::init_listener), this),
			std::bind (std::mem_fn (&hCraft::server::destroy_listener), this)));
		
		this->running = false;
	}
	
	/* 
	 * Class destructor.
	 * 
	 * Calls `stop ()' if the server is still running.
	 */
	server::~server ()
	{
		this->stop ();
	}
	
	
	
	/* 
	 * The function executed by worker threads.
	 * Waits for incoming connections.
	 */
	void
	server::work ()
	{
		while (!this->workers_ready)
			{
				if (this->workers_stop)
					return;
				
				std::this_thread::sleep_for (std::chrono::milliseconds (10));
			}
		
		// find the worker that spawned this thread.
		std::thread::id this_id = std::this_thread::get_id ();
		worker *w = nullptr;
		for (auto itr = this->workers.begin (); itr != this->workers.end (); ++itr)
			{
				worker &t = *itr;
				if (t.th.get_id () == this_id)
					w = &t;
			}
		
		while (!this->workers_stop)
			{
				event_base_loop (w->evbase, EVLOOP_NONBLOCK);
				std::this_thread::sleep_for (std::chrono::milliseconds (1));
			}
	}
	
	/* 
	 * Returns the worker that has the least amount of events associated with
	 * it.
	 */
	server::worker&
	server::get_min_worker ()
	{
		auto min_itr = this->workers.begin ();
		for (auto itr = (min_itr + 1); itr != this->workers.end (); ++itr)
			{
				if (itr->event_count.load () < min_itr->event_count.load ())
					min_itr = itr;
			}
		
		return *min_itr;
	}
	
	/* 
	 * Wraps the accepted connection around a player object and associates it
	 * with a server worker.
	 */
	void
	server::handle_accept (struct evconnlistener *listener, evutil_socket_t sock,
		struct sockaddr *addr, int len, void *ptr)
	{
		server &srv = *static_cast<server *> (ptr);
		
		// get IP address
		char ip[16];
		if (!inet_ntop (AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), ip,
			sizeof ip))
			{
				srv.log (LT_WARNING) << "Received a connection from an invalid IP address." << std::endl;
				evutil_closesocket (sock);
				return;
			}
		
		worker &w = srv.get_min_worker ();
		player *pl = new player (srv, w.evbase, sock, ip);
		
		{
			std::lock_guard<std::mutex> guard {srv.connecting_lock};
			srv.connecting.push_back (pl);
		}
	}
	
	
	
	/* 
	 * Removes and destroys disconnected players.
	 */
	void
	server::cleanup_players (scheduler_task& task)
	{
		server &srv = *(static_cast<server *> (task.get_context ()));
		
		// check list of logged-in players.
		srv.get_players ().remove_if (
			[] (player *pl) -> bool
				{
					if (pl->bad ())		
						return true;
					
					return false;
				}, true);
		
		// and the list of players that haven't fully logged-in yet.
		{
			std::lock_guard<std::mutex> guard {srv.connecting_lock};
			for (auto itr = srv.connecting.begin (); itr != srv.connecting.end (); )
				{
					player *pl = *itr;
					if (pl->bad ())
						{
							itr = srv.connecting.erase (itr);
							delete pl;
						}
					else
						++ itr;
				}
		}
	}
	
	
	
	/* 
	 * Attempts to start the server up.
	 * Throws `server_error' on failure.
	 */
	void
	server::start ()
	{
		if (this->running)
			throw server_error ("server already running");
		
		try
			{
				for (auto itr = this->inits.begin (); itr != this->inits.end (); ++itr)
					{
						initializer& init = *itr;
						init.init ();
						init.initialized = true;
					}
			}
		catch (const std::exception& ex)
			{
				for (auto itr = this->inits.rbegin (); itr != this->inits.rend (); ++itr)
					{
						initializer &init = *itr;
						if (init.initialized)
							{
								init.destroy ();
								init.initialized = false;
							}
					}
				
				// wrap the error in a server_error object.
				throw server_error (ex.what ());
			}
		
		this->running = true;
	}
	
	/* 
	 * Stops the server, kicking all connected players and freeing resources
	 * previously allocated by the start () member function.
	 */
	void
	server::stop ()
	{
		if (!this->running)
			return;
		
		for (auto itr = this->inits.rbegin (); itr != this->inits.rend (); ++itr)
			{
				initializer &init = *itr;
				init.destroy ();
				init.initialized = false;
			}
		
		this->running = false;
	}
	
	
	
	/* 
	 * Attempts to insert the specifed world into the server's world list.
	 * Returns true on success, and false on failure (due to a name collision).
	 */
	bool
	server::add_world (world *w)
	{
		std::lock_guard<std::mutex> guard {this->world_lock};
		
		cistring name {w->get_name ()};
		auto itr = this->worlds.find (name);
		if (itr != this->worlds.end ())
			return false;
		
		this->worlds[std::move (name)] = w;
		return true;
	}
	
	/* 
	 * Removes the specified world from the server's world list.
	 */
	void
	server::remove_world (world *w)
	{
		std::lock_guard<std::mutex> guard {this->world_lock};
		
		for (auto itr = this->worlds.begin (); itr != this->worlds.end (); ++itr)
			{
				world *other = itr->second;
				if (other == w)
					{
						this->worlds.erase (itr);
						delete other;
						break;
					}
			}
	}
	
	void
	server::remove_world (const char *name)
	{
		std::lock_guard<std::mutex> guard {this->world_lock};
		
		auto itr = this->worlds.find (name);
		if (itr != this->worlds.end ())
			this->worlds.erase (itr);
	}
	
	/* 
	 * Searches the server's world list for a world that has the specified name.
	 */
	world*
	server::find_world (const char *name)
	{
		std::lock_guard<std::mutex> guard {this->world_lock};
		
		auto itr = this->worlds.find (name);
		if (itr != this->worlds.end ())
			return itr->second;
		return nullptr;
	}
	
	
	
	/* 
	 * Returns a unique number that can be used for entity identification.
	 */
	int
	server::next_entity_id ()
	{
		std::lock_guard<std::mutex> guard {this->id_lock};
		int id = this->id_counter ++;
		if (this->id_counter < 0)
			this->id_counter = 0; // overflow.
		return id;
	}
	
	/* 
	 * Removes the specified player from the "connecting" list, and then inserts
	 * that player into the global player list.
	 * 
	 * If the server is full, or if the same player connected twice, the function
	 * returns false and the player is kicked with an appropriate message.
	 */
	bool
	server::done_connecting (player *pl)
	{
		bool can_stay = true;
		
		if (this->players->count () == this->get_config ().max_players)
			{ pl->kick ("§bThe server is full", "server full");
				can_stay = false; }
		else if (this->players->add (pl) == false)
			{ pl->kick ("§4You're already logged in", "already logged in");
				can_stay = false; }
		
		{
			std::lock_guard<std::mutex> guard {this->connecting_lock};
			for (auto itr = this->connecting.begin (); itr != this->connecting.end (); ++itr)
				{
					player *other = *itr;
					if (other == pl)
						{
							this->connecting.erase (itr);
							break;
						}
				}
		}
		
		return can_stay;
	}
	
	
	
/*******************************************************************************
		
		<init, destroy> pairs:
		
********************************************************************************/
	
//----
	// init_config (), destroy_config ():
	/* 
	 * Loads settings from the configuration file ("config.yaml",
	 * in YAML form) into the server's `cfg' structure. If "config.yaml"
	 * does not exist, it will get created with default settings.
	 */
	
	static void
	default_config (server_config& out)
	{
		std::strcpy (out.srv_name, "hCraft server");
		std::strcpy (out.srv_motd, "Welcome to my server!");
		out.max_players = 12;
		std::strcpy (out.main_world, "main");
		
		std::strcpy (out.ip, "0.0.0.0");
		out.port = 25565;
	}
	
	static void
	write_config (std::ofstream& strm, const server_config& in)
	{
		YAML::Emitter out;
		
		out << YAML::BeginMap;
		
		out << YAML::Key << "server";
		out << YAML::Value << YAML::BeginMap;
		
		out << YAML::Key << "general" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "server-name" << YAML::Value << in.srv_name;
		out << YAML::Key << "server-motd" << YAML::Value << in.srv_motd;
		out << YAML::Key << "max-players" << YAML::Value << in.max_players;
		out << YAML::Key << "main-world" << YAML::Value << in.main_world;
		out << YAML::EndMap;
		
		out << YAML::Key << "network" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "ip-address" << YAML::Value << in.ip;
		out << YAML::Key << "port" << YAML::Value << in.port;
		out << YAML::EndMap;
		
		out << YAML::EndMap;
		out << YAML::EndMap;
		
		strm << out.c_str () << std::flush;
	}
	
	
	
	static void
	_cfg_read_general_map (logger& log, const YAML::Node *general_map, server_config& out)
	{
		const YAML::Node *node;
		std::string str;
		bool error = false;
		
		// server name
		node = general_map->FindValue ("server-name");
		if (node && node->Type () == YAML::NodeType::Scalar)
			{
				*node >> str;
				if (str.size () > 0 && str.size() <= 80)
					std::strcpy (out.srv_name, str.c_str ());
				else
					{
						if (!error)
							log (LT_ERROR) << "Config: at map \"server.general\":" << std::endl;
						log (LT_INFO) << " - Scalar \"server-name\" must contain at "
														 "least one character and no more than 80." << std::endl;
						error = true;
					}
			}
		
		// server motd
		node = general_map->FindValue ("server-motd");
		if (node && node->Type () == YAML::NodeType::Scalar)
			{
				*node >> str;
				if (str.size() <= 80)
					std::strcpy (out.srv_motd, str.c_str ());
				else
					{
						if (!error)
							log (LT_ERROR) << "Config: at map \"server.general\":" << std::endl;
						log (LT_INFO) << " - Scalar \"server-motd\" must contain no more than 80 characters." << std::endl;
						error = true;
					}
			}
		
		// max players
		node = general_map->FindValue ("max-players");
		if (node && node->Type () == YAML::NodeType::Scalar)
			{
				int num;
				*node >> num;
				if (num > 0 && num <= 1024)
					out.max_players = num;
				else
					{
						if (!error)
							log (LT_ERROR) << "Config: at map \"server.general\":" << std::endl;
						log (LT_INFO) << " - Scalar \"max_players\" must be in the range of 1-1024." << std::endl;
						error = true;
					}
			}
		
		// main world
		node = general_map->FindValue ("main-world");
		if (node && node->Type () == YAML::NodeType::Scalar)
			{
				*node >> str;
				if (world::is_valid_name (str.c_str ()))
					std::strcpy (out.main_world, str.c_str ());
				else
					{
						if (!error)
							log (LT_ERROR) << "Config: at map \"server.general\":" << std::endl;
						log (LT_INFO) << " - Scalar \"main-world\" is not a valid world name." << std::endl;
						error = true;
					}
			}
	}
	
	static void
	_cfg_read_network_map (logger& log, const YAML::Node *network_map, server_config& out)
	{
		const YAML::Node *node;
		std::string str;
		bool error = false;
		
		// ip address
		node = network_map->FindValue ("ip-address");
		if (node && node->Type () == YAML::NodeType::Scalar)
			{
				*node >> str;
				if (str.size () == 0)
					std::strcpy (out.ip, "0.0.0.0");
				else
					{
						struct in_addr addr;
						if (inet_pton (AF_INET, str.c_str (), &addr) == 1)
							std::strcpy (out.ip, str.c_str ());
						else
							{
								if (!error)
									log (LT_ERROR) << "Config: at map \"server.network\":" << std::endl;
								log (LT_INFO) << " - Scalar \"ip-address\" is invalid." << std::endl;
								error = true;
							}
					}
			}
		
		// port
		node = network_map->FindValue ("port");
		if (node && node->Type () == YAML::NodeType::Scalar)
			{
				int num;
				*node >> num;
				if (num >= 0 && num <= 65535)
					out.port = num;
				else
					{
						if (!error)
							log (LT_ERROR) << "Config: at map \"server.network\":" << std::endl;
						log (LT_INFO) << " - Scalar \"port\" must be in the range of 0-65535." << std::endl;
						error = true;
					}
			}
	}
	
	static void
	_cfg_read_server_map (logger& log, const YAML::Node *server_map, server_config& out)
	{
		const YAML::Node *general_map = server_map->FindValue ("general");
		if (general_map && general_map->Type () == YAML::NodeType::Map)
			_cfg_read_general_map (log, general_map, out);
		
		const YAML::Node *network_map = server_map->FindValue ("network");
		if (network_map && network_map->Type () == YAML::NodeType::Map)
			_cfg_read_network_map (log, network_map, out);
	}
	
	static void
	read_config (logger& log, std::ifstream& strm, server_config& out)
	{
		YAML::Parser parser (strm);
		
		YAML::Node doc;
		if (!parser.GetNextDocument (doc))
			return;
		
		const YAML::Node *server_map = doc.FindValue ("server");
		if (server_map && server_map->Type () == YAML::NodeType::Map)
			_cfg_read_server_map (log, server_map, out);
	}
	
	
	
	void
	server::init_config ()
	{
		default_config (this->cfg);
		
		log () << "Loading configuration from \"config.yaml\"" << std::endl;
		
		std::ifstream strm ("config.yaml");
		if (strm.is_open ())
			{
				read_config (this->log, strm, this->cfg);
				strm.close ();
				return;
			}
		
		log () << "Configuration file does not exist, creating one with default settings." << std::endl;
		std::ofstream ostrm ("config.yaml");
		if (!ostrm.is_open ())
			{
				log (LT_ERROR) << "Failed to open \"server.cfg\" for writing." << std::endl;
				return;
			}
		
		write_config (ostrm, this->cfg);
	}
	
	void
	server::destroy_config ()
		{ }
	
	
	
//---
	// init_sql (), destroy_sql ():
	/* 
	 * Loads the server's database.
	 */
	void
	server::init_sql ()
	{
		int err;
		char* errmsg;
		
		log () << "Opening SQL database (at \"database.db\")" << std::endl;
		this->db.open ("database.db");
		
		// 
		// Create tables.
		// 
		this->db.execute (
			"CREATE TABLE IF NOT EXISTS `players` (`id` INTEGER PRIMARY KEY "
			"AUTOINCREMENT, `name` TEXT, `groups` TEXT, `nick` TEXT);"
			
			"CREATE TABLE IF NOT EXISTS `autoloaded-worlds` (`name` TEXT);"
			);
	}
	
	void
	server::destroy_sql ()
	{
		this->db.close ();
	}
	
	
	
//---
	// init_core (), destroy_core ():
	/* 
	 * Initializes various data structures and variables needed by the server.
	 */
	
	void
	server::init_core ()
	{
		this->sched.start ();
		
		this->players = new playerlist ();
		this->id_counter = 0;
		
		this->get_scheduler ().new_task (hCraft::server::cleanup_players, this)
			.run_forever (250);
		this->tpool.start (6); // 6 pooled threads
	}
	
	void
	server::destroy_core ()
	{
		this->tpool.stop ();
		this->sched.stop ();
		
		{
			std::lock_guard<std::mutex> guard {this->connecting_lock};
			for (auto itr = this->connecting.begin (); itr != this->connecting.end (); ++itr)
				{
					player *pl = *itr;
					delete pl;
				}
			this->connecting.clear ();
		}
		
		this->players->clear (true);
		delete this->players;
	}
	
	
	
//---
	// init_commands (), destroy_commands ():
	/* 
	 * Loads up commands.
	 */
	
	static void _add_command (permission_manager& perm_man, command_list *dest,
		const char *name)
	{
		command *cmd = command::create (name);
		if (cmd)
			{
				dest->add (cmd);
				/*
				// register permissions
				const char **perms = cmd->get_permissions ();
				while (*perms != nullptr)
					perm_man.add (*perms++);*/
			}
	}
	
	void
	server::init_commands ()
	{
		this->commands = new command_list ();
		
		_add_command (this->perms, this->commands, "help");
		_add_command (this->perms, this->commands, "me");
		_add_command (this->perms, this->commands, "ping");
		_add_command (this->perms, this->commands, "wcreate");
		_add_command (this->perms, this->commands, "wload");
		_add_command (this->perms, this->commands, "world");
		_add_command (this->perms, this->commands, "tp");
		_add_command (this->perms, this->commands, "nick");
		_add_command (this->perms, this->commands, "wunload");
	}
	
	void
	server::destroy_commands ()
	{
		delete this->commands;
	}
	
	
	
//---
	// init_ranks (), destroy_ranks ():
	/* 
	 * Reads reads and their associated permissions from "ranks.yaml".
	 * If the file does not exist, it will get created with default settings.
	 */
	
	static void
	create_default_ranks (group_manager& groups)
	{
		group* grp_guest = groups.add (1, "guest");
		grp_guest->set_color ('7');
		grp_guest->add ("command.info.help");
		
		group* grp_member = groups.add (2, "member");
		grp_member->set_color ('a');
		grp_member->inherit (grp_guest);
		grp_member->add ("command.chat.me");
		
		group* grp_builder = groups.add (3, "builder");
		grp_builder->set_color ('2');
		grp_builder->inherit (grp_member);
		grp_builder->add ("command.world.world");
		grp_builder->add ("command.world.tp");
		
		group* grp_designer = groups.add (4, "designer");
		grp_designer->set_color ('b');
		grp_designer->inherit (grp_builder);
		
		group* grp_architect = groups.add (5, "architect");
		grp_architect->set_color ('3');
		grp_architect->inherit (grp_designer);
		
		group* grp_moderator = groups.add (6, "moderator");
		grp_moderator->set_color ('c');
		grp_moderator->inherit (grp_designer);
		grp_moderator->add ("command.misc.ping");
		
		group* grp_admin = groups.add (7, "admin");
		grp_admin->set_color ('4');
		grp_admin->inherit (grp_architect);
		grp_admin->inherit (grp_moderator);
		grp_builder->add ("command.world.tp.others");
		
		group* grp_executive = groups.add (8, "executive");
		grp_executive->set_color ('e');
		grp_executive->inherit (grp_admin);
		grp_executive->add ("command.world.wcreate");
		grp_executive->add ("command.world.wload");
		grp_executive->add ("command.world.wunload");
		grp_executive->add ("command.chat.nick");
		
		group* grp_owner = groups.add (9, "owner");
		grp_owner->set_color ('6');
		grp_owner->add ("*");
	}
	
	static void
	write_ranks (std::ostream& strm, group_manager& groups)
	{
		YAML::Emitter emit;
		
		std::vector<group *> sorted_groups;
		for (auto itr = groups.begin (); itr != groups.end (); ++itr)
			sorted_groups.push_back (itr->second);
		std::sort (sorted_groups.begin (), sorted_groups.end (),
			[] (const group* a, const group* b) -> bool
				{ return (*a) < (*b); });
		
		emit << YAML::BeginMap;
		
		emit << YAML::Key << "default-rank" << YAML::Value << "guest";
		
		emit << YAML::Key << "groups";
		emit << YAML::Value << YAML::BeginMap;
		for (group* grp : sorted_groups)
			{
				emit << YAML::Key << grp->get_name ();
				emit << YAML::Value << YAML::BeginMap;
				
				if (grp->get_parents ().size () > 0)
					{
						emit << YAML::Key << "inheritance"
							   << YAML::Value << YAML::BeginSeq;
						for (group *parent : grp->get_parents ())
							emit << parent->get_name ();
						emit << YAML::EndSeq;
					}
				
				emit << YAML::Key << "power" << YAML::Value << grp->get_power ();
				emit << YAML::Key << "color" << YAML::Value << grp->get_color ();
				emit << YAML::Key << "prefix" << YAML::Value << grp->get_prefix ();
				emit << YAML::Key << "suffix" << YAML::Value << grp->get_suffix ();
				emit << YAML::Key << "mprefix" << YAML::Value << grp->get_mprefix ();
				emit << YAML::Key << "msuffix" << YAML::Value << grp->get_msuffix ();
				emit << YAML::Key << "can-chat" << YAML::Value << grp->can_chat ();
				emit << YAML::Key << "can-build" << YAML::Value << grp->can_build ();
				emit << YAML::Key << "can-move" << YAML::Value << grp->can_move ();
				
				emit << YAML::Key << "permissions";
				emit << YAML::Value << YAML::BeginSeq;
				for (permission perm : grp->get_perms ())
					{
						emit << groups.get_permission_manager ().to_string (perm);
					}
				emit << YAML::EndSeq;
				
				emit << YAML::EndMap;
			}
		
		emit << YAML::EndMap << YAML::EndMap;
		
		strm << emit.c_str () << std::flush;
	}
	
	
	
	using group_inheritance_map
		= std::unordered_map<std::string, std::vector<std::string>>;
	
	static void
	_ranks_read_group (logger& log, const YAML::Node &group_node,
		const std::string& group_name, group_manager& groups,
		group_inheritance_map& ihmap)
	{
		const YAML::Node *node;
		
		int grp_power;
		char grp_color;
		std::string grp_prefix;
		std::string grp_mprefix;
		std::string grp_suffix;
		std::string grp_msuffix;
		bool grp_can_build;
		bool grp_can_move;
		bool grp_can_chat;
		std::vector<std::string> perms;
		
		// inheritance
		node = group_node.FindValue ("inheritance");
		if (node && node->Type () == YAML::NodeType::Sequence)
			{
				std::string parent;
				for (int i = 0; i < node->size (); ++i)
					{
						(*node)[i] >> parent;
						
						auto itr = ihmap.find (group_name);
						std::vector<std::string>* seq;
						if (itr == ihmap.end ())
							{
								ihmap[group_name] = std::vector<std::string> ();
								seq = &ihmap[group_name];
							}
						else
							seq = &itr->second;
						
						seq->push_back (std::move (parent));
					}
			}
		
		// power
		node = group_node.FindValue ("power");
		if (!node)
			throw server_error ("in \"ranks.yaml\": group \"power\" field not found.");
		*node >> grp_power;
		
		// color
		node = group_node.FindValue ("color");
		if (!node)
			grp_color = 'f';
		else
			*node >> grp_color;
		
		// prefix
		node = group_node.FindValue ("prefix");
		if (node)
			{
				*node >> grp_prefix;
				if (grp_prefix.size () > 32)
					grp_prefix.resize (32);
			}
		
		// mprefix
		node = group_node.FindValue ("mprefix");
		if (node)
			{
				*node >> grp_mprefix;
				if (grp_mprefix.size () > 32)
					grp_mprefix.resize (32);
			}
		
		// suffix
		node = group_node.FindValue ("suffix");
		if (node)
			{
				*node >> grp_suffix;
				if (grp_suffix.size () > 32)
					grp_suffix.resize (32);
			}
		
		// msuffix
		node = group_node.FindValue ("msuffix");
		if (node)
			{
				*node >> grp_msuffix;
				if (grp_msuffix.size () > 32)
					grp_msuffix.resize (32);
			}
		
		// can-build
		node = group_node.FindValue ("can-build");
		if (node)
			*node >> grp_can_build;
		else
			grp_can_build = true;
		
		// can-move
		node = group_node.FindValue ("can-move");
		if (node)
			*node >> grp_can_move;
		else
			grp_can_move = true;
		
		// can-chat
		node = group_node.FindValue ("can-chat");
		if (node)
			*node >> grp_can_chat;
		else
			grp_can_chat = true;
		
		// permissions
		node = group_node.FindValue ("permissions");
		if (node && node->Type () == YAML::NodeType::Sequence)
			{
				std::string perm;
				for (int i = 0; i < node->size (); ++i)
					{
						(*node)[i] >> perm;
						perms.push_back (std::move (perm));
					}
			}
		
		// create the group and add it to the list.
		group *grp = groups.add (grp_power, group_name.c_str ());
		grp->set_color (grp_color);
		grp->set_prefix (grp_prefix.c_str ());
		grp->set_mprefix (grp_mprefix.c_str ());
		grp->set_suffix (grp_suffix.c_str ());
		grp->set_msuffix (grp_msuffix.c_str ());
		grp->can_build (grp_can_build);
		grp->can_move (grp_can_move);
		grp->can_chat (grp_can_chat);
		for (auto& perm : perms)
			grp->add (perm.c_str ());
	}
	
	static void
	_ranks_read_groups_map (logger& log, const YAML::Node *groups_map,
		group_manager& groups)
	{
		group_inheritance_map ihmap;
		for (auto itr = groups_map->begin (); itr != groups_map->end (); ++itr)
			{
				std::string group_name;
				itr.first () >> group_name;
				_ranks_read_group (log, itr.second (), group_name, groups, ihmap);
			}
		
		// resolve inheritance
		for (auto itr = ihmap.begin (); itr != ihmap.end (); ++itr)
			{
				std::string name = itr->first;
				std::vector<std::string>& parents = itr->second;
				
				group *child = groups.find (name.c_str ());
				for (auto& parent : parents)
					{
						group *grp = groups.find (parent.c_str ());
						if (grp)
							{
								child->inherit (grp);
							}
					}
			}
	}
	
	static void
	read_ranks (logger& log, std::istream& strm, group_manager& groups)
	{
		YAML::Parser parser (strm);
		
		YAML::Node doc;
		if (!parser.GetNextDocument (doc))
			return;
		
		const YAML::Node *def_rank = doc.FindValue ("default-rank");
		if (!def_rank || def_rank->Type () != YAML::NodeType::Scalar)
			throw server_error ("in \"ranks.yaml\": \"default-rank\" field not found or invalid");
		std::string def_rank_name;
		(*def_rank) >> def_rank_name;
		
		const YAML::Node *groups_map = doc.FindValue ("groups");
		if (groups_map && groups_map->Type () == YAML::NodeType::Map)
			_ranks_read_groups_map (log, groups_map, groups);
		
		groups.default_rank.set (def_rank_name.c_str (), groups);
	}
	
	
	
	void
	server::init_ranks ()
	{
		std::ifstream istrm ("ranks.yaml");
		if (istrm)
			{
				log () << "Loading ranks from \"ranks.yaml\"" << std::endl;
				read_ranks (this->log, istrm, this->groups);
				log (LT_INFO) << " - Loaded " << this->groups.size () << " groups." << std::endl;
				istrm.close ();
				return;
			}
		
		create_default_ranks (this->groups);
		
		log () << "\"ranks.yaml\" does not exist... Instantiating one with default settings." << std::endl;
		std::ofstream ostrm ("ranks.yaml");
		if (!ostrm)
			{
				log (LT_ERROR) << "Failed to open \"ranks.yaml\" for writing." << std::endl;
				return;
			}
		
		write_ranks (ostrm, this->groups);
	}
	
	void
	server::destroy_ranks ()
	{
		this->groups.clear ();
	}
	
	
	
//---
	// init_worlds (), destroy_worlds ():
	/* 
	 * Loads up and initializes worlds.
	 */
	
	void
	server::init_worlds ()
	{
		mkdir ("worlds", 0744);
		std::vector<std::string> to_load;
		
		world *main_world;
		std::string prov_name;
		
		log () << "Loading worlds:" << std::endl;
		
		// load main world
		prov_name = world_provider::determine ("worlds",
			this->get_config ().main_world);
		if (prov_name.empty ())
			{
				// main world does not exist
				log () << " - Main world does not exist, creating..." << std::endl;
				main_world = new world (this->get_config ().main_world,
					world_generator::create ("flatgrass"),
					world_provider::create ("hw", "worlds", this->get_config ().main_world));
				main_world->set_size (32, 32);
			}
		else
			{
				log () << " - Loading \"" << this->get_config ().main_world << "\"" << std::endl;
				world_provider *prov = world_provider::create (prov_name.c_str (),
					"worlds", this->get_config ().main_world);
				if (!prov)
					throw server_error ("failed to load main world (invalid provider)");
				
				const world_information& winf = prov->info ();
				world_generator *gen = world_generator::create (winf.generator.c_str (), winf.seed);
				if (!gen)
					{
						delete prov;
						throw server_error ("failed to load main world (invalid generator)");
					}
				
				main_world = new world (this->get_config ().main_world, gen, prov);
				main_world->set_size (winf.width, winf.depth);
			}
		main_world->prepare_spawn (10);
		main_world->start ();
		this->add_world (main_world);
		this->main_world = main_world;
		
		// load worlds from the autoload list.
		{
			sql::statement stmt = this->sql ().create (
				"SELECT * FROM `autoloaded-worlds`");
			while (stmt.step () == sql::row)
				{
					const char *world_name = (const char *)stmt.column_text (0);
					to_load.push_back (world_name);
				}
		}
		for (std::string& wname : to_load)
			{
				std::string prov_name = world_provider::determine ("worlds", wname.c_str ());
				if (prov_name.empty ())
					{
						log (LT_WARNING) << " - World \"" << wname << "\" does not exist." << std::endl;
						continue;
					}
				
				world_provider *prov = world_provider::create (prov_name.c_str (),
					"worlds", wname.c_str ());
				if (!prov)
					{
						log (LT_ERROR) << "Failed to load world \"" << wname
							<< "\": Invalid provider." << std::endl;
						continue;
					}
				
				const world_information& winf = prov->info ();
				world_generator *gen = world_generator::create (winf.generator.c_str (), winf.seed);
				if (!gen)
					{
						delete prov;
						log (LT_ERROR) << "Failed to load world \"" << wname
							<< "\": Invalid generator." << std::endl;
						continue;
					}
				
				log () << " - Loading \"" << wname << std::endl;
				world *wr = new world (wname.c_str (), gen, prov);
				wr->set_size (winf.width, winf.depth);
				wr->prepare_spawn (10);
				wr->start ();
				if (!this->add_world (wr))
					{
						log (LT_ERROR) << "Failed to load world \"" << wname << "\": Already loaded." << std::endl;
						delete wr;
						continue;
					}
			}
	}
	
	void
	server::destroy_worlds ()
	{
		
		// clear worlds
		{
			std::lock_guard<std::mutex> guard {this->world_lock};
			for (auto itr = this->worlds.begin (); itr != this->worlds.end (); ++itr)
				{
					world *w = itr->second;
					w->stop ();
					w->save_all ();
					delete w;
				}
			this->worlds.clear ();
		}
	}
	
	
	
//----
	// init_workers (), destroy_workers ():
	/* 
	 * Creates and starts server workers.
	 * The total number of workers created depends on how many cores the user
	 * has installed on their system, which means that on a multi-core system,
	 * the work will be parallelized between all cores.
	 */
	
	void
	server::init_workers ()
	{
		this->worker_count = std::thread::hardware_concurrency ();
		if (this->worker_count == 0)
			this->worker_count = 2;
		this->workers.reserve (this->worker_count);
		log () << "Creating " << this->worker_count << " server workers." << std::endl;
		
		this->workers_stop = false;
		this->workers_ready = false;
		for (int i = 0; i < this->worker_count; ++i)
			{
				struct event_base *base = event_base_new ();
				if (!base)
					{
						this->workers_stop = true;
						for (auto itr = this->workers.begin (); itr != this->workers.end (); ++itr)
							{
								worker &w = *itr;
								if (w.th.joinable ())
									w.th.join ();
								
								event_base_free (w.evbase);
							}
						
						throw server_error ("failed to create workers");
					}
				
				std::thread th (std::bind (std::mem_fn (&hCraft::server::work), this));
				this->workers.push_back (worker (base, std::move (th)));
			}
		
		this->workers_ready = true;
	}
	
	void
	server::destroy_workers ()
	{
		this->workers_stop = true;
		for (auto itr = this->workers.begin (); itr != this->workers.end (); ++itr)
			{
				worker &w = *itr;
				if (w.th.joinable ())
					w.th.join ();
				
				event_base_free (w.evbase);
			}
	}
	
	
	
//----
	// init_listener (), destroy_listener ():
	/* 
	 * Creates the listening socket and starts listening on the IP address and
	 * port number specified by the user in the configuration file for incoming
	 * connections.
	 */
	void
	server::init_listener ()
	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port   = htons (this->cfg.port);
		inet_pton (AF_INET, this->cfg.ip, &addr.sin_addr);
		
		worker &w = this->get_min_worker ();
		this->listener = evconnlistener_new_bind (w.evbase,
			&hCraft::server::handle_accept, this, LEV_OPT_CLOSE_ON_FREE
			| LEV_OPT_REUSEABLE, -1, (struct sockaddr *)&addr, sizeof addr);
		if (!this->listener)
			throw server_error ("failed to create listening socket (port taken?)");
		
		++ w.event_count;
		log () << "Started listening on port " << this->cfg.port << "." << std::endl;
	}
	
	void
	server::destroy_listener ()
	{
		evconnlistener_free (this->listener);
	}
}

