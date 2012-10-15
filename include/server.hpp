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

#ifndef _hCraft__SERVER_H_
#define _hCraft__SERVER_H_

#include "cistring.hpp"
#include "logger.hpp"
#include "player.hpp"
#include "playerlist.hpp"
#include "scheduler.hpp"
#include "world.hpp"
#include "threadpool.hpp"
#include "commands/command.hpp"

#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <event2/event.h>
#include <event2/listener.h>


namespace hCraft {
	
	/* 
	 * Base class for all exceptions thrown by the server.
	 */
	class server_error: public std::runtime_error
	{
	public:
		server_error (const char *what)
		: std::runtime_error (what)
			{ }
	};
	
	
	/* 
	 * A simple POD structure for settings needed by the server to run.
	 */
	struct server_config
	{
		char srv_name[81];
		char srv_motd[81];
		int  max_players;
		char main_world[33];
		
		char ip[16];
		int  port;
	};
	
	
	/* 
	 * 
	 */
	class server
	{
		friend struct worker;
		
		/* 
		 * To simplify the process of starting up a server, the actual logic of the
		 * start () member function is divided and moved into smaller functions
		 * (the initializer and destroyer functions). When an initializer function
		 * fails (i.e: an exception is thrown), the destroyer functions for all
		 * initializers that have successfully completed up until that point are
		 * invoked to reclaim the resources that had been allocated.
		 */
		struct initializer
		{
			std::function<void ()> init;
			std::function<void ()> destroy;
			bool initialized;
			
			// constructor.
			initializer (std::function<void ()>&& init,
				std::function<void ()>&& destroy);
			
			// move constructor.
			initializer (initializer&& other);
			
			initializer (const initializer &);
		};
		
		
		/* 
		 * Represents a server worker, an entity to which a thread and an event base
		 * are attached. Every server worker handles I/O for all events that are
		 * registered with it in a separate thread of execution, equally dividing
		 * the server's load.
		 */
		struct worker
		{
			struct event_base *evbase;
			std::atomic_int event_count;
			std::thread th;
			
			// constructor.
			worker (struct event_base *base, std::thread&& th);
			
			// move constructor.
			worker (worker&& w);
			
			worker (const worker&) = delete;
		};
		
	private:
		logger& log;
		server_config cfg;
		
		std::vector<initializer> inits; // <init, destroy> pairs
		bool running;
		
		std::vector<worker> workers;
		int worker_count;
		bool workers_ready;
		bool workers_stop;
		
		struct evconnlistener *listener;
		
		playerlist *players;
		std::vector<player *> connecting;
		std::mutex connecting_lock;
		int id_counter;
		std::mutex id_lock;
		
		scheduler sched;
		thread_pool tpool;
		
		std::unordered_map<cistring, world *> worlds;
		std::mutex world_lock;
		world *main_world;
		
		command_list *commands;
		
	private:
		// <init, destroy> functions:
		
		/* 
		 * Loads settings from the configuration file ("server-config.yaml",
		 * in YAML form) into the server's `cfg' structure. If "server-config.yaml"
		 * does not exist, it will get created with default settings.
		 */
		void init_config ();
		void destroy_config ();
		
		/* 
		 * Initializes various data structures and variables needed by the server.
		 */
		void init_core ();
		void destroy_core ();
		
		/* 
		 * Reads reads and their associated permissions from "ranks.yaml".
		 * If the file does not exist, it will get created with default settings.
		 */
		void init_ranks ();
		void destroy_ranks ();
		
		/* 
		 * Loads up commands.
		 */
		void init_commands ();
		void destroy_commands ();
		
		/* 
		 * Loads up and initializes worlds.
		 */
		void init_worlds ();
		void destroy_worlds ();
		
		/* 
		 * Creates and starts server workers.
		 * The total number of workers created depends on how many cores the user
		 * has installed on their system, which means that on a multi-core system,
		 * the work will be parallelized between all cores.
		 */
		void init_workers ();
		void destroy_workers ();
		
		/* 
		 * Creates the listening socket and starts listening on the IP address and
		 * port number specified by the user in the configuration file for incoming
		 * connections.
		 */
		void init_listener ();
		void destroy_listener ();
		
	private:
		/* 
		 * The function executed by worker threads.
		 * Waits for incoming connections.
		 */
		void work ();
		
		/* 
		 * Returns the worker that has the least amount of events associated with
		 * it.
		 */
		worker& get_min_worker ();
		
		/* 
		 * Wraps the accepted connection around a player object and associates it
		 * with a server worker.
		 */
		static void handle_accept (struct evconnlistener *listener,
			evutil_socket_t sock, struct sockaddr *addr, int len, void *ptr);
			
	private:
		// scheduler callbacks:
		
		/* 
		 * Removes and destroys disconnected players.
		 */
		static void cleanup_players (scheduler_task& task);
		
	public:
		inline bool is_running () { return this->running; }
		inline const server_config& get_config () { return this->cfg; }
		inline logger& get_logger () { return this->log; }
		inline playerlist& get_players () { return *this->players; }
		inline scheduler& get_scheduler () { return this->sched; }
		inline thread_pool& get_thread_pool () { return this->tpool; }
		inline world* get_main_world () { return this->main_world; }
		inline command_list& get_commands () { return *this->commands; }
		
	public:
		/* 
		 * Constructs a new server.
		 */
		server (logger &log);
		
		// copy constructor.
		server (const server &) = delete;
		
		/* 
		 * Class destructor.
		 * 
		 * Calls `stop ()' if the server is still running.
		 */
		~server ();
		
		
		
		/* 
		 * Attempts to start the server up.
		 * Throws `server_error' on failure.
		 */
		void start ();
		
		/* 
		 * Stops the server, kicking all connected players and freeing resources
		 * previously allocated by the start () member function.
		 */
		void stop ();
		
		
		
		/* 
		 * Attempts to insert the specifed world into the server's world list.
		 * Returns true on success, and false on failure (due to a name collision).
		 */
		bool add_world (world *w);
		
		/* 
		 * Removes the specified world from the server's world list.
		 */
		void remove_world (world *w);
		void remove_world (const char *name);
		
		/* 
		 * Searches the server's world list for a world that has the specified name.
		 */
		world* find_world (const char *name);
		
		
		
		/* 
		 * Returns a unique number that can be used for entity identification.
		 */
		int next_entity_id ();
		
		/* 
		 * Removes the specified player from the "connecting" list, and then inserts
		 * that player into the global player list.
		 * 
		 * If the server is full, or if the same player connected twice, the function
		 * returns false and the player is kicked with an appropriate message.
		 */
		bool done_connecting (player *pl);
	};
}

#endif

