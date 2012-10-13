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

#ifndef _hCraft__SCHEDULER_H_
#define _hCraft__SCHEDULER_H_

#include <chrono>
#include <functional>
#include <list>
#include <thread>
#include <mutex>


namespace hCraft {	
	
	class scheduler;
	
	
	/* 
	 * Represents a task that can be executed by a scheduler.
	 */
	class scheduler_task
	{
		friend class scheduler;
		
		scheduler& sched;
		
		bool enabled;
		bool stopped;
		bool recurring;
		int  repeat_counter;
		int  repeat_max;
		
		std::chrono::milliseconds interval;
		std::chrono::milliseconds delay;
		std::chrono::time_point<std::chrono::system_clock> next_time;
		
		std::function<void (scheduler_task&)> cb;
		void *ctx;
		
	public:
		inline void* get_context () { return this->ctx; }
		inline bool is_enabled () { return this->enabled; }
		inline bool is_recurring () { return this->recurring; }
		
	public:
		/* 
		 * Constructs a new scheduler task from the given callback function.
		 */
		scheduler_task (scheduler& sched,
			std::function<void (scheduler_task&)>&& cb, void* ctx = nullptr);
		
		
		
		/* 
		 * Runs the task once, and the immediately stops.
		 */
		scheduler_task& run_once (int delay_ms = 0);
		scheduler_task& run_once (int delay_ms, void *ctx);
		
		/* 
		 * Runs the task continuously without stopping.
		 */
		scheduler_task& run_forever (int interval_ms = 1000, int delay_ms = 0);
		scheduler_task& run_forever (int interval_ms, int delay_ms, void *ctx);
		
		
		/* 
		 * Stops the task from executing and removes it from its scheduler.
		 */
		void stop ();
	};
	
	
	/* 
	 * General-purpose task scheduler.
	 */
	class scheduler
	{
		std::list<scheduler_task *> tasks;
		int sleep_interval;
		
		std::thread *main_thread;
		std::mutex   lock;
		bool running;
		
	private:
		friend class scheduler_task;
		
		/* 
		 * Adds the specified task to the scheduler's task list.
		 */
		void add_task (scheduler_task *task);
		
		
		/* 
		 * Iterates over tasks and executes them if conditions are met.
		 * Runs in separate thread.
		 */
		void main_loop ();
		
	public:
		inline bool is_running () { return this->running; }
		
	public:
		/* 
		 * Constructs a new empty scheduler.
		 */
		scheduler ();
		
		/* 
		 * Class destructor.
		 */
		~scheduler ();
		
		
		/* 
		 * Starts the scheduler's main thread and begin executing tasks.
		 */
		void start ();
		
		/* 
		 * Stops the scheduler and removes all registered tasks.
		 */
		void stop ();
		
		
		/* 
		 * Creates and returns a new task.
		 */
		scheduler_task& new_task (std::function<void (scheduler_task&)>&& cb,
			void *ctx = nullptr);
	};
}

#endif

