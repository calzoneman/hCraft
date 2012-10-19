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

#include "sql.hpp"
#include <string>


namespace hCraft {
	
	namespace sql {
		
		void (*dctor_static)(void *) = SQLITE_STATIC;
		void (*dctor_transient)(void *) = SQLITE_TRANSIENT;
		
		
		database::database ()
		{
			this->db = nullptr;
			this->opened = false;
		}
		
		database::database (const char *path)
		{
			this->open (path);
		}
	
		database::~database ()
		{
			this->close ();
		}
		
		
		
		void
		database::open (const char *path)
		{
			if (this->opened)
				this->close ();
			
			int err;
			err = sqlite3_open (path, &this->db);
			if (err != SQLITE_OK)
				{
					sqlite3_close (this->db);
					throw sql_error ("failed to open database");
				}
			this->opened = true;
		}
		
		void
		database::close ()
		{
			if (!this->opened)
				return;
			sqlite3_close (this->db);
		}
		
		
		
		statement
		database::create (const char *sql)
		{
			return statement (*this, sql);
		}
		
		statement
		database::create (const std::string& sql)
		{
			return this->create (sql.c_str ());
		}
		
		void
		database::execute (const char *sql)
		{
			const char *str = sql;
			int err;
			while (*str)
				{
					statement stmt (*this, str, &str);
					stmt.execute ();
				}
		}
		
		int
		database::scalar_int (const char *sql)
		{
			statement stmt (*this, sql);
			if (stmt.step () == row)
				return stmt.column_int (0);
			throw sql_error ("no rows were fetched");
		}
		
		
		
	//----
		
		statement::statement (database &db, const char *sql, const char **tail)
			: db (db), code (sql)
		{
			int err;
			err = sqlite3_prepare_v2 (db.db, sql, -1, &this->stmt, tail);
			if (err != SQLITE_OK)
				throw sql_error (sqlite3_errmsg (db.db));
		}
		
		statement::~statement ()
		{
			this->finalize ();
		}
		
		void
		statement::finalize ()
		{
			if (this->stmt)
				{
					sqlite3_finalize (this->stmt);
				}
		}
		
		
		
		void
		statement::bind_text (int index, const char *text, int len,
			void (*dctor)(void *))
		{
			if (!this->stmt)
				return;
			if (sqlite3_bind_text (this->stmt, index, text, len, dctor) != SQLITE_OK)
				throw sql_error (sqlite3_errmsg (this->db.db));
		}
		
		
		
		void
		statement::execute ()
		{
			int ret;
			for (;;)
				{
					ret = this->step ();
					if (ret == sql::done)
						break;
					if (ret != sql::row)
						throw sql_error (sqlite3_errmsg (this->db.db));
				}
		}
		
		return_code
		statement::step ()
		{
			if (this->stmt)
				return sqlite3_step (this->stmt);
			return done;
		}
		
		void
		statement::reset ()
		{
			sqlite3_reset (this->stmt);
		}
		
		
		
		int
		statement::column_int (int col)
		{
			return sqlite3_column_int (this->stmt, col);
		}
		
		long long
		statement::column_int64 (int col)
		{
			return sqlite3_column_int64 (this->stmt, col);
		}
		
		double
		statement::column_double (int col)
		{
			return sqlite3_column_double (this->stmt, col);
		}
		
		const unsigned char*
		statement::column_text (int col)
		{
			return sqlite3_column_text (this->stmt, col);
		}
		
		int
		statement::column_bytes (int col)
		{
			return sqlite3_column_bytes (this->stmt, col);
		}
	}
}

