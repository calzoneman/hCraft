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
#include "../server.hpp"
#include "../player.hpp"
#include "../world.hpp"
#include "../worldprovider.hpp"
#include "../worldgenerator.hpp"
#include <chrono>
#include <functional>


namespace hCraft {
	namespace commands {
		
		const char*
		c_wcreate::get_name ()
			{ return "wcreate"; }
		
		const char*
		c_wcreate::get_summary ()
			{ return "Creates a new world, and if requested, also loads it into the server's world list."; }
		
		int
		c_wcreate::get_usage_count ()
			{ return 3; }
		
		const char*
		c_wcreate::get_usage (int n)
		{
			static const char *usage[3] =
				{
					"/wcreate [--load] <world>",
					"/wcreate [--load] [--provider <prov>] [--generator <gen>, "
						"--seed <seed>] [--width <width>] [--depth <depth>] <world>",
					"/wcreate [--help/--summary]",
				};
			
			return (n >= this->get_usage_count ()) ? "" : usage[n];
		}
		
		const char*
		c_wcreate::get_usage_help (int n)
		{
			static const char *help[3] =
				{
					"Creates a new infinite world called <world> using the default "
					"world provider \"hw\" (HWv1) and world generator \"flatgrass\". "
					"If flag --load is specified, the created world will then be "
					"immediately loaded into the server's world list.",
					
					"A more detailed form of the above: the user can choose the size "
					"of the new world, possibly making it finite; and use a custom "
					"world provider and/or a generator (a seed number to the generator "
					"can be provided using --seed). To get a list of currently "
					"supported providers: do \"/wproviders\". As for generators, "
					"execute \"/wgenerators\".",
					
					"Same as calling >/help< on >wcreate< (\"/help [-s] wcreate\")",
				};
			
			return (n >= this->get_usage_count ()) ? "" : help[n];
		}
		
		const char**
		c_wcreate::get_examples ()
		{
			static const char *examples[] =
				{
					"/wcreate world2",
					"/wcreate -w 256 -d 256 testworld",
					"/wcreate --provider=\"mcsharp\" -w 512 -d 512 another-world",
					"/wcreate --provider=\"hw\" default-world",
					nullptr,
				};
			
			return examples;
		}
		
		const char**
		c_wcreate::get_permissions ()
		{
			static const char *perms[] =
				{
					"command.world.wcreate",
					nullptr,
				};
			
			return perms;
		}
		
		
		
	//----
		
		void
		c_wcreate::execute (player *pl, command_reader& reader)
		{
			reader.add_option ("help", "h", false, false, false);
			reader.add_option ("summary", "s", false, false, false);
			reader.add_option ("load", "l", false, false, false);
			reader.add_option ("width", "w", true, true, false);
			reader.add_option ("depth", "d", true, true, false);
			reader.add_option ("provider", "p", true, true, false);
			reader.add_option ("generator", "g", true, true, false);
			reader.add_option ("seed", "", true, true, false);
			if (!reader.parse_args (pl))
				return;
			
			if (reader.get_option ("help")->found)
				{ this->show_help (pl); return; }
			else if (reader.get_option ("summary")->found)
				{ this->show_summary (pl); return; }
			
			if (reader.no_args () || reader.arg_count () > 1)
				{ this->show_usage (pl); return; }
			
		//----
			/* 
			 * Parse arguments:
			 */
			
			// world name
			std::string& world_name = reader.arg (0);
			if (!world::is_valid_name (world_name.c_str ()))
				{
					pl->message ("§c * §eWorld names must be under §a32 §echaracters long and "
											 "may only contain alpha§f-§enumeric characters§f, §edots§f, "
											 "§ehyphens and underscores§f.");
					return;
				}
			
			// world width
			int world_width = 0;
			auto opt_width = reader.get_option ("width");
			if (opt_width->found)
				{
					if (!opt_width->is_int ())
						{
							pl->message_nowrap ("§c * §eArgument to flag §c--width §emust be an integer§f.");
							return;
						}
					world_width = opt_width->as_int ();
					if (world_width < 0)
						world_width = 0;
				}
			
			// world depth
			int world_depth = 0;
			auto opt_depth = reader.get_option ("depth");
			if (opt_depth->found)
				{
					if (!opt_depth->is_int ())
						{
							pl->message_nowrap ("§c * §eArgument to flag §c--depth §emust be an integer§f.");
							return;
						}
					world_depth = opt_depth->as_int ();
					if (world_depth < 0)
						world_depth = 0;
				}
			
			// world provider
			std::string provider_name ("hw");
			auto opt_prov = reader.get_option ("provider");
			if (opt_prov->found)
				{
					provider_name.assign (opt_prov->as_string ());
				}
			
			// world generator
			std::string gen_name ("flatgrass");
			auto opt_gen = reader.get_option ("generator");
			if (opt_gen->found)
				{
					gen_name.assign (opt_gen->as_string ());
				}
			
			// generator seed
			int gen_seed = std::chrono::duration_cast<std::chrono::milliseconds> (
				std::chrono::high_resolution_clock::now ().time_since_epoch ()).count ()
				& 0x7FFFFFFF;
			auto opt_seed = reader.get_option ("seed");
			if (opt_seed->found)
				{
					if (opt_seed->is_int ())
						gen_seed = opt_seed->as_int ();
					else
						gen_seed = std::hash<std::string> () (opt_seed->as_string ()) & 0x7FFFFFFF;
				}
			
			// load world
			bool load_world = reader.get_option ("load")->found;
			
		//----
			
			if (load_world && (pl->get_server ().find_world (world_name.c_str ()) != nullptr))
				{
					pl->message_nowrap ("§c * §eA world with the same name is already loaded§f.");
					return;
				}
			
			world_provider *prov = world_provider::create (provider_name.c_str (),
				"worlds", world_name.c_str ());
			if (!prov)
				{
					pl->message ("§c * §eInvalid world provider§f: §c" + provider_name);
					return;
				}
			
			world_generator *gen = world_generator::create (gen_name.c_str (), gen_seed);
			if (!gen)
				{
					pl->message ("§c * §eInvalid world generator§f: §c" + provider_name);
					delete prov;
					return;
				}
			
			pl->message ("§f - §aCreating world §e" + world_name + "§f...");
			world *wr = new world (world_name.c_str (), gen, prov);
			wr->set_width (world_width);
			wr->set_depth (world_depth);
			wr->prepare_spawn (10);
			
			pl->message_nowrap ("§f - §aSaving§f...");
			wr->save_all ();
			
			if (load_world)
				{
					pl->message_nowrap ("§f - §aLoading§f...");
					if (!pl->get_server ().add_world (wr))
						{
							delete wr;
							pl->message_nowrap ("§cFailed to load world§7.");
						}
				}
			else
				{
					delete wr;
				}
			
			pl->message_nowrap ("§eWorld §a" + world_name + " §ehas been successfully created§f.");
		}
	}
}

