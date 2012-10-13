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

#include "logger.hpp"
#include "server.hpp"
#include <iostream>
#include <cstring>
#include <exception>


int
main (int argc, char *argv[])
{
	hCraft::logger log;
	hCraft::server srv (log);
	
	try
		{
			srv.start ();
		}
	catch (const std::exception& ex)
		{
			log (hCraft::LT_FATAL) << "Failed to start server." << std::endl;
			log (hCraft::LT_INFO) << " -> " << ex.what () << std::endl;
			return -1;
		}
	
	log (hCraft::LT_CONSOLE) << "Type \"/stop\" to stop the server." << std::endl;
	
	char input[1024];
	while (srv.is_running ())
		{
			std::cin.getline (input, sizeof input);
			if (std::cin.fail ())
				{ std::cin.clear (); continue; }
			
			if (std::strcmp (input, "/stop") == 0)
				break;
		}
	
	return 0;
}

