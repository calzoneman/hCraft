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

#ifndef _hCraft__COMMAND_H_
#define _hCraft__COMMAND_H_

#include <vector>
#include <string>
#include <unordered_map>


namespace hCraft {
	
	class player;
	
	
	/* 
	 * Command argument parser.
	 */
	class command_reader
	{
	public:
		struct option
		{
			const char *lname;
			const char *sname;
			bool has_arg;
			bool arg_req;
			bool required;
			bool found;
			
			std::string arg;
			bool got_arg;
		};
		
	private:
		std::string name;
		std::string args;
		std::vector<option> options;
		std::vector<std::string> non_opts;
		
	public:
		inline std::vector<option>& get_flags () { return this->options; }
		inline std::vector<std::string>& get_args () { return this->non_opts; }
		
		inline bool no_args () { return this->non_opts.size () == 0; }
		inline bool have_args () { return this->non_opts.size () > 0; }
		inline int  arg_count () { return this->non_opts.size (); }
		inline std::string& arg (int n) { return this->non_opts[n]; }
		inline const std::string& get_arg_string () { return this->args; }
		
		inline bool no_flags () { return this->options.size () == 0; }
		inline bool have_flags () { return this->options.size () > 0; }
		inline int  flag_count () { return this->options.size (); }
		
		inline std::vector<std::string>::iterator
		begin () { return this->non_opts.begin (); }
		
		inline std::vector<std::string>::iterator
		end () { return this->non_opts.end (); }
		
	public:
		/* 
		 * Constructs a new command reader from the given string (should be in the
		 * form of: /<cmd> <arg1> <arg2> ... <argN>
		 */
		command_reader (const std::string& str);
		
		
		/* 
		 * Returns the name of the parsed command.
		 */
		const std::string& command_name ();
		
		
		
		/* 
		 * Adds the specified option to the parser's option list.
		 * These will be parsed in a call to parse_args ().
		 */
		void add_option (const char *long_name, const char *short_name, bool has_arg,
			bool arg_required, bool opt_required);
		
		/* 
		 * Finds and returns the option with the given long name.
		 */
		option* get_option (const char *long_name);
		
		/* 
		 * Parses the argument list.
		 * In case of an error, false is returned and an appropriate message is sent
		 * to player @{err}.
		 */
		bool parse_args (player *err);
	};
	
	
	/* 
	 * The base class for all commands.
	 */
	class command
	{
	public:
		/* 
		 * Returns the name of the command (the same one used to execute it, /<name>).
		 */
		virtual const char* get_name () = 0;
		
		/* 
		 * Returns a short summary of what the command does (a brief sentence or two,
		 * highlighting the main points).
		 */
		virtual const char* get_summary () = 0;
		
		/* 
		 * Returns the total number of usage patterns available.
		 */
		virtual int get_usage_count () = 0;
		
		/* 
		 * Returns the N'th usage pattern.
		 */
		virtual const char* get_usage (int n) = 0;
		
		/* 
		 * Returns a string containing a detailed and descriptive information about
		 * the N'th usage pattern.
		 */
		virtual const char* get_usage_help (int n) = 0;
		
		/* 
		 * Returns a null terminated array of command invocation examples.
		 */
		virtual const char** get_examples () = 0;
		
		/* 
		 * Returns the permissions string (in the form of:
		 * <node1>.<node2>. ... .<nodeN>).
		 */
		virtual const char* get_permissions () = 0;
		
	//----
		
		virtual void show_summary (player *pl);
		virtual void show_usage (player *pl);
		virtual void show_help (player *pl);
		
	//----
		
		/* 
		 * Executes the command on the specified player.
		 */
		virtual void execute (player *pl, command_reader& reader) = 0;
		
	//----
		
		/* 
		 * Returns a new instance of the command named @{name}.
		 */
		static command* create (const char *name);
	};
	
	
	class command_list
	{
		std::unordered_map<std::string, command *> commands;
		
	public:
		/* 
		 * Class destructor.
		 * Destroys all registered commands.
		 */
		~command_list ();
		
		
		/* 
		 * Adds the specified command to the list.
		 */
		void add (command *cmd);
		
		/* 
		 * Finds the command that has the specified name (case-sensitive).
		 */
		command* find (const char *name);
	};
}

#endif

