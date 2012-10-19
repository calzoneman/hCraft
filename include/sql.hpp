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

#ifndef _hCraft__SQL_H_
#define _hCraft__SQL_H_

#include <stdexcept>
#include <string>
#include <sqlite3.h>


namespace hCraft {
	
	class sql_error: public std::runtime_error
	{
	public:
		sql_error (const char *what)
			: std::runtime_error (what)
			{ }
	};
	
	/* 
	 * A collection of classes that wrap around sqlite3 functions.
	 */
	namespace sql {
		
		// forward defs:
		class database;
		
		
		typedef int return_code;
		const return_code ok    = SQLITE_OK;
		const return_code done  = SQLITE_DONE;
		const return_code busy  = SQLITE_BUSY;
		const return_code row   = SQLITE_ROW;
		const return_code error = SQLITE_ERROR;
		
		extern void (*dctor_static)(void *);
		extern void (*dctor_transient)(void *);
		
		
		class statement
		{
			friend class database;
			sqlite3_stmt *stmt;
			database& db;
			const char *code;
			
			
		private:
			statement (database &db, const char *sql, const char **tail = nullptr);
			
		public:
			~statement ();
			void finalize ();
			
			void bind_text (int index, const char *text, int len = -1,
				void (*dctor)(void *) = dctor_static);
			
			void execute ();
			return_code step ();
			void reset ();
			
			int column_int (int col);
			long long column_int64 (int col);
			double column_double (int col);
			const unsigned char* column_text (int col);
			int column_bytes (int col);
		};
		
		
		class database
		{
			friend class statement;
			sqlite3 *db;
			bool opened;
			
		public:
			database ();
			database (const char *path);
			~database ();
			
			
			void open (const char *path);
			void close ();
			
			
			statement create (const char *sql);
			statement create (const std::string& sql);
			void execute (const char *sql);
			int scalar_int (const char *sql);
		};
	}
}

#endif

