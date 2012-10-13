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

#ifndef _hCraft__THREAD_POOL_H_
#define _hCraft__THREAD_POOL_H_

#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <queue>
#include <condition_variable>


namespace hCraft {
	
	/* 
	 * A pool of threads that can be used to asynchronously execute tasks.
	 */
	class thread_pool
	{
		friend struct worker_thread;
		
		struct task
		{
			std::function<void (void *)> callback;
			void *context;
			
			task () {}
			task (std::function<void (void *)>&& cb, void *ctx)
				: callback (std::move (cb)), context (ctx)
				{ }
		};
		
		struct worker_thread
		{
			thread_pool *pool;
			std::thread th;
			
			worker_thread (const worker_thread&) = delete;
			worker_thread (thread_pool *pool, std::thread&& th)
				: pool (pool), th (std::move (th))
				{ }
			worker_thread (worker_thread&& other)
				: pool (other.pool), th (std::move (other.th))
				{ }
		};
		
	private:
		std::vector<worker_thread> workers;
		std::queue<task> tasks;
		std::mutex task_lock;
		std::condition_variable cv;
		bool terminating;
		
	private:
		/* 
		 * The function ran by worker threads.
		 */
		void main_loop ();
		
		/* 
		 * Returns the next available task. If none are currently available, the
		 * function blocks until one is.
		 */
		task get_task ();
		
	public:
		thread_pool ();
		thread_pool (const thread_pool&) = delete;
		
		
		
		/* 
		 * Starts up @{thread_count} amount of worker threads and begins processing
		 * tasks.
		 */
		void start (int thread_count);
		
		/* 
		 * Terminates all running pool threads.
		 */
		void stop ();
		
		
		
		/* 
		 * Schedules the specified task to be ran by a pooled thread.
		 */
		void enqueue (std::function<void (void *)>&& cb, void *context = nullptr);
	};
}

#endif

