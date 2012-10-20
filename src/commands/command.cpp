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

#include "command.hpp"
#include "player.hpp"

#include "infoc.hpp"
#include "chatc.hpp"
#include "miscc.hpp"
#include "worldc.hpp"

#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <cctype>


namespace hCraft {
	
	static command* create_c_help () { return new commands::c_help (); }
	static command* create_c_me () { return new commands::c_me (); }
	static command* create_c_ping () { return new commands::c_ping (); }
	static command* create_c_wcreate () { return new commands::c_wcreate (); }
	static command* create_c_world () { return new commands::c_world (); }
	
	/* 
	 * Returns a new instance of the command named @{name}.
	 */
	command*
	command::create (const char *name)
	{
		static std::unordered_map<std::string, command* (*)()> creators {
			{ "help", create_c_help },
			{ "me", create_c_me },
			{ "ping", create_c_ping },
			{ "wcreate", create_c_wcreate },
			{ "world", create_c_world },
			};
		
		auto itr = creators.find (name);
		if (itr == creators.end ())
			return nullptr;
		
		return itr->second ();
	}
	
//----
	
	/* 
	 * Returns a null-terminated array of aliases (secondary names).
	 */
	const char**
	command::get_aliases ()
	{
		static const char* arr[] = {
			nullptr,
		};
		return arr;
	}
	
	
	
//----
	
	bool
	command_reader::option::is_int ()
	{
		if (!this->got_arg)
			return false;
		
		int i = 0;
		std::string& str = this->as_string ();
		if (str[0] == '-')
			{
				if (str.size () == 1)
					return false;
				++i;
			}
		
		for (; i < str.size (); ++i)
			{
				int c = str[i];
				if (!std::isdigit (c))
					return false;
			}
		
		return true;
	}
	
	int
	command_reader::option::as_int ()
	{
		std::string& str = this->as_string ();
		std::istringstream ss {str};
		int num;
		ss >> num;
		return num;
	}
	
	
	
//----
	
	/* 
	 * Constructs a new command reader from the given string (should be in the
	 * form of: /<cmd> <arg1> <arg2> ... <argN>
	 */
	command_reader::command_reader (const std::string& str)
	{
		// extract the command name.
		int i = 1;
		std::string::size_type sp = str.find_first_of (' ');
		this->name.reserve (((sp == std::string::npos) ? str.size () : sp) + 1);
		for (int j = ((sp == std::string::npos) ? str.size () : sp); i < j; ++i)
			this->name.push_back (str[i]);
		
		// extract the arguments as a whole.
		this->args.reserve ((str.size () - i) + 1);
		for (++i; i < str.size (); ++i)
			this->args.push_back (str[i]);
	}
	
	
	
	/* 
	 * Returns the name of the parsed command.
	 */
	const std::string&
	command_reader::command_name ()
	{
		return this->name;
	}
	
	
	
	/* 
	 * Adds the specified option to the parser's option list.
	 * These will be parsed in a call to parse_args ().
	 */
	void
	command_reader::add_option (const char *long_name, const char *short_name,
		bool has_arg, bool arg_required, bool opt_required)
	{
		this->options.push_back ({long_name, short_name, has_arg, arg_required,
			opt_required, false, "", false});
	}
	
	/* 
	 * Finds and returns the option with the given long name.
	 */
	command_reader::option*
	command_reader::get_option (const char *long_name)
	{
		for (option& opt : this->options)
			if (std::strcmp (opt.lname, long_name) == 0)
				return &opt;
		return nullptr;
	}
	
	
	
	static bool
	_read_string (std::istringstream& ss, std::string& iarg, std::string& out,
		player *err)
	{
		std::ostringstream oss;
		int i;
		bool found_end = false;
		
		for (i = 1; i < iarg.size (); ++i)
			{
				if (iarg[i] == '\\')
					{
						// character escape
						if (iarg.size () > (i + 1) && iarg[i + 1] == '"')
							{ oss << '"'; ++i; }
						else
							oss << '\\';
					}
				else if (iarg[i] == '"')
					{ found_end = true; break; }
				else
					oss << iarg[i];
			}
		
		char c;
		ss >> std::noskipws;
		while (!found_end && !ss.fail () && !ss.eof ())
			{
				ss >> c;
				if (c == '"')
					{ found_end = true; break; }
				else
					oss << c;
			}
		ss >> std::skipws;
		
		if (!found_end)
			{
				err->message_nowrap ("§c * §eIncomplete string§f.");
				return false;
			}
		
		out = oss.str ();
		return true;
	}
	
	/* 
	 * Parses the argument list.
	 * In case of an error, false is returned and an appropriate message is sent
	 * to player @{err}.
	 */
	bool
	command_reader::parse_args (player *err)
	{
		std::istringstream ss {this->args};
		std::string str;
		std::string opt_name;
		std::string opt_arg;
		bool has_str = false;
		
		// read options
		while (!ss.fail () && !ss.eof ())
			{
				if (!has_str)
					{
						str.clear ();
						ss >> str;
					}
				has_str = false;
				
				if (str.empty ())
					break;
				
				if (str[0] == '-')
					{
						if (str.size () > 1 && str[1] == '-')
							{
								if (str.size () == 2)
									break; // end of option list.
								
								/* long option */
								std::string::size_type eq = str.find_first_of ('=');
								bool has_arg = (eq != std::string::npos);
								opt_name = str.substr (2, (eq == std::string::npos)
									? (str.size () - 2) : (eq - 2));
								
								auto itr = std::find_if (this->options.begin (), this->options.end (),
									[&opt_name] (const option& opt) -> bool
										{
											return std::strcmp (opt.lname, opt_name.c_str ()) == 0;
										});
								if (itr == this->options.end ())
									{
										err->message ("§c * §eUnrecognized option§f: §c--" + opt_name);
										return false;
									}
								
								option& opt = *itr;
								opt.found = true;
								if (opt.has_arg)
									{
										if (has_arg)
											{
												opt_arg = str.substr (eq + 1, str.size () - (eq + 1));
												if (opt_arg.size () > 0 && opt_arg[0] == '"')
													{
														if (!_read_string (ss, opt_arg, opt_arg, err))
															return false;
													}
												
												opt.arg = std::move (opt_arg);
												opt.got_arg = true;
											}
										else if (opt.arg_req)
											{
												err->message ("§c * §eArgument required for option§f: §c--" + opt_name);
												return false;
											}
									}
							}
						else
							{
								if (str.size () == 1)
									{
										this->non_opts.push_back (str);
										continue;
									}
									
								/* short option(s) */
								for (int i = 1; i < str.size (); ++i)
									{
										char optc = str[i];
										auto itr = std::find_if (this->options.begin (), this->options.end (),
											[optc] (const option& opt) -> bool
												{
													return (opt.sname && opt.sname[0] == optc);
												});
										if (itr == this->options.end ())
											{
												err->message ("§c * §eUnrecognized option§f: §c-" + std::string (1, optc));
												return false;
											}
										
										option& opt = *itr;
										opt.found = true;
										
										if (opt.has_arg)
											{
												if (opt.arg_req && i != (str.size () - 1))
													{
														err->message ("§c * §eArgument required for option§f: §c-" + std::string (opt.sname));
														return false;
													}
												
												if (i == (str.size () - 1))
													{
														if (opt.arg_req && (ss.eof () || ss.fail ()))
															{
																err->message ("§c * §eArgument required for option§f: §c-" + std::string (opt.sname));
																return false;
															}
															
														ss >> str;
														if (str[0] == '-')
															{
																has_str = true;
															}
														else
															{
																opt.got_arg = true;
																if (str[0] == '"')
																	{
																		if (!_read_string (ss, str, str, err))
																			return false;
																	}
																
																opt.arg = std::move (str);
															}
													}
											}
									}
							}
					}
				
				// non-option arguments:
				else
					{
						if (str[0] == '"')
							{
								if (!_read_string (ss, str, str, err))
									return false;
							}
						
						this->non_opts.push_back (str);
					}
			}
		
		// read non-option arguments
		while (!ss.fail () && !ss.eof ())
			{
				ss >> str;
				if (str.empty ()) break;
				if (str[0] == '"')
					{
						if (!_read_string (ss, str, str, err))
							return false;
					}
				this->non_opts.push_back (str);
			}
		
		for (option& opt : this->options)
			{
				if (opt.required && !opt.found)
					{
						err->message ("§c * §eRequired argument not found§f: §c--" + std::string (opt.lname));
						return false;
					}
			}
		
		return true;
	}
	
	
	
//------------
	
	static bool
	is_punc (char c)
	{
		switch (c)
			{
				case '`': case '\'': case '[': case ']': case '(': case ')':
				case '{': case '}': case '<': case '>': case ':': case ',':
				case '-': case '.': case '?': case '"': case ';': case '/':
				case '!':
					return true;
			}
		return false;
	}
	
	static std::string
	color_string (const char *in)
	{
		std::ostringstream ss;
		bool at_name = false, at_flag = false, at_opt = false, in_word = false;
		bool in_quotes = false, is_marked = false;
		char last_col = 'f';
		
		const char *ptr = in;
		if (*ptr == '/')
			at_name = true;
		
		int c;
		while (c = *ptr++)
			{
				if (!is_marked && !in_quotes && c == '-')
					{
						int n = *ptr;
						if ((ptr != in) && !std::isalnum (*(ptr - 2)))
							{
								if (n == '-' || std::isalpha (n))
									{
										at_flag = true;
										if (last_col != 'c')
											{
												ss << "§c";
												last_col = 'c';
											}
										if (n == '-')
											{ ss << "--"; ++ ptr; }
										else
											ss << '-';
										continue;
									}
							}
						
					}
				else if (c == '<')
					{
						if (is_marked)
							{ is_marked = false; continue; }
						
						int n = *ptr;
						if (n != 0 && std::isalpha (n))
							at_opt = true;
						if (last_col != '7')
							{
								ss << "§7";
								last_col = '7';
							}
						ss << '<';
						continue;
					}
				else if (c == '>')
					{
						if (at_opt)
							ss << '>';
						else
							{
								if (last_col != '6')
									{
										ss << "§6";
										last_col = '6';
									}
								is_marked = true;
							}
						continue;
					}
				else if (!is_marked && c == '"')
					{
						if (last_col != 'e')
							{
								ss << "§e";
								last_col = 'e';
							}
						ss << '"';
						in_quotes = !in_quotes;
						continue;
					}
				
				if (in_quotes)
					{
						if (last_col != 'b')
							{
								ss << "§b";
								last_col = 'b';
							}
					}
				else if (is_marked)
					{
						if (last_col != '6')
							{
								ss << "§6";
								last_col = '6';
							}
					}
				else
					{
						if (is_punc (c))
							{
								char col = (*ptr == ' ' || *ptr == '\0') ? 'f' : (at_flag ? '4' : 'f');
								if (last_col != col)
									{
										ss << "§" << col;
										last_col = col;
									}
							}
						else if (std::isalpha (c))
							{
								char col = at_name ? '6' : ((at_flag) ? 'c' : ((at_opt) ? '7' : 'e'));
								if (last_col != col)
									{
										ss << "§" << col;
										last_col = col;
									}
						
								in_word = true;
							}
						else if (std::isdigit (c))
							{
								if (!in_word)
									{
										if (last_col != 'c')
											{
												ss << "§c";
												last_col = 'c';
											}
									}
							}
						else if (c == '>')
							{
								at_opt = false;
							}
						else if (c == ' ')
							{
								in_word = false;
								at_name = false;
								at_flag = false;
								at_opt  = false;
							}
					}
				
				ss << (char)c;
			}
		
		return ss.str ();
	}
	
	
	void
	command::show_summary (player *pl)
	{
		pl->message ("§6Summary for command §e" + std::string (this->get_name ()) + "§f:");
		pl->message_spaced ("    " + color_string (this->get_summary ()));
		if (this->get_aliases ()[0] != nullptr)
			{
				std::ostringstream ss;
				ss << "§8  Aliases§7: ";
				
				const char **aliases = this->get_aliases();
				int count = 0;
				while (*aliases)
					{
						if (count > 0)
							ss << "§7, ";
						ss << "§c" << (*aliases++);
						++ count;
					}
				
				pl->message (ss.str ());
			}
	}
	
	void
	command::show_usage (player *pl)
	{
		int usage_count = this->get_usage_count ();
		int i;
		
		pl->message ("§6Usage for command §e" + std::string (this->get_name ()) + "§f:");
		for (i = 0; i < usage_count; ++i)
			pl->message_spaced ("    " + color_string (this->get_usage (i)));
	}
	
	void
	command::show_help (player *pl)
	{
		int usage_count = this->get_usage_count ();
		int i;
		
		std::ostringstream ss;
		
		pl->message ("§6Showing help for command §e" + std::string (this->get_name ()) + "§f:");
		for (i = 0; i < usage_count; ++i)
			{
				ss << "  §f(§c" << (i + 1) << "§f): " << color_string (this->get_usage (i));
				pl->message_spaced (ss.str ());
				ss.str (std::string ()); ss.clear ();
				
				ss << "    " << color_string (this->get_usage_help (i));
				pl->message_spaced (ss.str ());
				ss.str (std::string ()); ss.clear ();
			}
		
		if (this->get_aliases ()[0] != nullptr)
			{
				std::ostringstream ss;
				ss << "§8Aliases§7: ";
				
				const char **aliases = this->get_aliases();
				int count = 0;
				while (*aliases)
					{
						if (count > 0)
							ss << "§7, ";
						ss << "§c" << (*aliases++);
						++ count;
					}
				
				pl->message (ss.str ());
			}
			
		pl->message_nowrap ("§8Examples§7:");
		const char **examples = this->get_examples ();
		for (const char **ex = examples; *ex; ++ex)
			{
				ss << "    §7" << (*ex);
				pl->message_spaced (ss.str ());
				ss.str (std::string ()); ss.clear ();
			}
	}
	
	
	
//------------
	
	/* 
	 * Class destructor.
	 * Destroys all registered commands.
	 */
	command_list::~command_list ()
	{
		for (auto itr = this->commands.begin (); itr != this->commands.end (); ++itr)
			{
				command *cmd = itr->second;
				delete cmd;
			}
		this->commands.clear ();
	}
	
	/* 
	 * Adds the specified command to the list.
	 */
	void
	command_list::add (command *cmd)
	{
		if (!cmd)
			throw std::runtime_error ("command cannot be null");
		
		this->commands[cmd->get_name ()] = cmd;
		
		// register aliases
		const char **aliases = cmd->get_aliases ();
		const char **ptr     = aliases;
		while (*ptr)
			{
				const char *alias = *ptr++;
				auto itr = this->aliases.find (alias);
				if (itr != this->aliases.end ())
					throw std::runtime_error ("alias collision");
				
				this->aliases[alias] = cmd->get_name ();
			}
	}
	
	/* 
	 * Finds the command that has the specified name (case-sensitive).
	 */
	command*
	command_list::find (const char *name)
	{
		auto itr = this->commands.find (name);
		if (itr != this->commands.end ())
			return itr->second;
		
		// check aliases
		auto aitr = this->aliases.find (name);
		if (aitr != this->aliases.end ())
			{
				const std::string& cmd_name = aitr->second;
				itr = this->commands.find (cmd_name);
				if (itr != this->commands.end ())
					return itr->second;
			}
		
		return nullptr;
	}
}

