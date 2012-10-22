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

#ifndef _hCraft__COMMANDS__WORLD_H_
#define _hCraft__COMMANDS__WORLD_H_

#include "command.hpp"


namespace hCraft {
	namespace commands {
		
		/* 
		 * /wcreate -
		 * 
		 * Creates a new world, and if requested, loads it into the current world
		 * list.
		 * 
		 * Permissions:
		 *   - command.world.wcreate
		 *       Needed to execute the command.
		 */
		class c_wcreate: public command
		{
		public:
			const char* get_name () { return "wcreate"; }
			
			const char** get_aliases ()
			{
				static const char* aliases[] =
					{
						"create-world",
						"world-create",
						nullptr,
					};
				return aliases;
			}
			
			const char*
			get_summary ()
				{ return "Creates a new world, and if requested, also loads it into the "
								 "server's world list."; }
			
			const char**
			get_usage ()
			{
				static const char *usage[] =
					{
						"/wcreate [--load] <world>",
						"/wcreate [--load] [--provider <prov>] [--generator <gen>, "
							"--seed <seed>] [--width <width>] [--depth <depth>] <world>",
						"/wcreate [--help/--summary]",
						nullptr,
					};
				return usage;
			}
			
			const char**
			get_help ()
			{
				static const char *help[] =
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
						nullptr,
					};
				return help;
			}
			
			const char**
			get_examples ()
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
			
			const char* get_exec_permission () { return "command.world.wcreate"; }
			
		//----
			void execute (player *pl, command_reader& reader);
		};
		
		
		/* 
		 * /wload -
		 * 
		 * Loads a world from disk onto the server's world list. Doing this enables
		 * players to go to that world using /w.
		 * 
		 * Permissions:
		 *   - command.world.wload
		 *       Needed to execute the command.
		 */
		class c_wload: public command
		{
		public:
			const char* get_name () { return "wload"; }
			
			const char**
			get_aliases ()
			{
				static const char* aliases[] =
					{
						"load-world",
						"world-load",
						nullptr,
					};
				return aliases;
			}
			
			const char*
			get_summary ()
				{ return "Loads a world from disk into the server's online world list."; }
			
			const char**
			get_usage ()
			{
				static const char *usage[] =
					{
						"/wload <world>",
						"/wload [--autoload] <world>",
						"/wload [--help/--summary]",
						nullptr,
					};
				return usage;
			}
			
			const char**
			get_help ()
			{
				static const char *help[] =
					{
						"Loads world <world> from disk into the server's world list, enabling "
						"players to teleport to it using the \"/world\" command. >NOTE: Two "
						"worlds with the same name cannot be loaded at once.<",
					
						"Same as above, but also adds the world into the server's >autoload< "
						"list. This will make the server automatically load the world at "
						"startup.",
					
						"Same as calling >/help< on >wload< (\"/help [-s] wload\")",
						nullptr,
					};
				return help;
			}
			
			const char**
			get_examples ()
			{
				static const char *examples[] =
					{
						"/wload test",
						"/wload --autoload mfb3",
						"/wload -a mfb5",
						nullptr,
					};
				return examples;
			}
			
			const char* get_exec_permission () { return "command.world.wload"; }
			
		//----
			void execute (player *pl, command_reader& reader);
		};
		
		
		/* 
		 * /world - 
		 * 
		 * Teleports the player to a requested world.
		 * 
		 * Permissions:
		 *   - command.world.world
		 *       Needed to execute the command.
		 */
		class c_world: public command
		{
		public:
			const char* get_name () { return "world"; }
			
			const char**
			get_aliases ()
			{
				static const char *aliases[] =
					{
						"w",
						nullptr,
					};
				return aliases;
			}
			
			const char*
			get_summary ()
				{ return "Teleports the player to a requested world."; }
			
			const char**
			get_usage ()
			{
				static const char *usage[] =
					{
						"/world",
						"/world <world>",
						"/world [--help/--summary]",
						nullptr,
					};
				return usage;
			}
			
			const char**
			get_help ()
			{
				static const char *help[] =
					{
						"Displays the name of the world that the player is currently in.",
					
						"Teleports the player to world <world>.",
					
						"Same as calling >/help< on >world< (\"/help [-s] world\")",
						nullptr,
					};
				return help;
			}
			
			const char**
			get_examples ()
			{
				static const char *examples[] =
					{
						"/world",
						"/world main",
						"/world freebuild-2",
						nullptr,
					};
				return examples;
			}
			
			const char* get_exec_permission () { return "command.world.world"; }
			
		//----
			void execute (player *pl, command_reader& reader);
		};
		
		
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
		class c_tp: public command
		{
		public:
			const char* get_name () { return "tp"; }
			
			const char*
			get_summary ()
				{ return "Teleports the player to a requested location."; }
			
			const char**
			get_usage ()
			{
				static const char *usage[] =
					{
						"/tp <player>",
						"/tp <player> <dest-player>",
						"/tp <x> <y> <z>",
						"/tp <player> <x> <y> <z>",
						"/tp [--help/--summary]",
						nullptr,
					};
				return usage;
			}
			
			const char**
			get_help ()
			{
				static const char *help[] =
					{
						"Teleports the calling player to the location of player <player>.",
					
						"Teleports the player <player> to the location of player <dest-player>.",
					
						"Teleports the calling player to the exact coordinates <x> <y> <z>.",
					
						"Teleports the player <player> to the exact coordinates <x> <y> <z>.",
					
						"Same as calling >/help< on >tp< (\"/help [-s] tp\")",
						nullptr,
					};
				return help;
			}
			
			const char**
			get_examples ()
			{
				static const char *examples[] =
					{
						"/tp dude123",
						"/tp randomguy dude123",
						"/tp -61 80 5",
						"/tp dude123 -1000 100 1000",
						nullptr,
					};
				return examples;
			}
			
			const char* get_exec_permission () { return "command.world.tp"; }
			
		//----
			void execute (player *pl, command_reader& reader);
		};
	}
}

#endif

