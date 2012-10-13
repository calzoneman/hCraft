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

#include "scheduler.hpp"
#include <memory>


namespace hCraft {
	
	/* 
	 * Constructs a new scheduler task from the given callback function.
	 */
	scheduler_task::scheduler_task (scheduler& sched,
		std::function<void (scheduler_task&)>&& cb, void* ctx)
		: sched (sched), cb (std::move (cb)), ctx (ctx)
	{
		this->enabled = false;
		this->stopped = false;
		this->recurring = false;
		this->repeat_counter = 0;
		this->repeat_max = -1;
		
		this->interval = std::chrono::milliseconds {1000};
		this->delay = std::chrono::milliseconds::zero ();
		this->next_time = std::chrono::system_clock::now ();
	}
	
	
	
	/* 
	 * Runs the task once, and the immediately stops.
	 */
	 
	scheduler_task&
	scheduler_task::run_once (int delay_ms)
	{
		this->next_time = std::chrono::system_clock::now ()
			+ std::chrono::milliseconds (delay_ms);
		this->recurring = false;
		this->delay = std::chrono::milliseconds {delay_ms};
		this->enabled = true;
		this->sched.add_task (this);
		return *this;
	}
	
	scheduler_task&
	scheduler_task::run_once (int delay_ms, void *ctx)
	{
		this->next_time = std::chrono::system_clock::now ()
			+ std::chrono::milliseconds (delay_ms);
		this->recurring = false;
		this->delay = std::chrono::milliseconds {delay_ms};
		this->ctx = ctx;
		this->enabled = true;
		this->sched.add_task (this);
		return *this;
	}
	
	
	/* 
	 * Runs the task continuously without stopping.
	 */
	
	scheduler_task&
	scheduler_task::run_forever (int interval_ms, int delay_ms)
	{
		this->next_time = std::chrono::system_clock::now ()
			+ std::chrono::milliseconds (delay_ms);
		this->recurring = true;
		this->interval = std::chrono::milliseconds {interval_ms};
		this->delay = std::chrono::milliseconds {delay_ms};
		this->enabled = true;
		this->sched.add_task (this);
		return *this;
	}
	
	scheduler_task&
	scheduler_task::run_forever (int interval_ms, int delay_ms, void *ctx)
	{
		this->next_time = std::chrono::system_clock::now ()
			+ std::chrono::milliseconds (delay_ms);
		this->recurring = true;
		this->interval = std::chrono::milliseconds {interval_ms};
		this->delay = std::chrono::milliseconds {delay_ms};
		this->ctx = ctx;
		this->enabled = true;
		this->sched.add_task (this);
		return *this;
	}
	
	
	
	/* 
	 * Stops the task from executing and removes it from its scheduler.
	 */
	void
	scheduler_task::stop ()
	{
		this->stopped = true;
	}
	
	
	
//----
	
	/* 
	 * Constructs a new empty scheduler.
	 */
	scheduler::scheduler ()
	{
		this->main_thread = nullptr;
		this->running = false;
		this->sleep_interval = 1000;
	}
	
	/* 
	 * Class destructor.
	 */
	scheduler::~scheduler ()
	{
		std::lock_guard<std::mutex> guard {this->lock};
		for (auto itr = this->tasks.begin (); itr != this->tasks.end (); ++ itr)
			{
				scheduler_task *task = *itr;
				delete task;
			}
		this->tasks.clear ();
	}
	
	
	
	/* 
	 * Iterates over tasks and executes them if conditions are met.
	 * Runs in separate thread.
	 */
	void
	scheduler::main_loop ()
	{
		while (this->running)
			{
				auto time_now = std::chrono::system_clock::now ();
				
				{
					std::lock_guard<std::mutex> guard {this->lock};
					for (auto itr = this->tasks.begin (); itr != this->tasks.end (); )
						{
							scheduler_task *task = *itr;
							if (task->stopped)
								{
									delete task;
									itr = this->tasks.erase (itr);
									continue;
								}
							
							if (task->enabled)
								{
									if (task->next_time <= time_now)
										{
											task->cb (*task);
											if (!task->recurring)
												task->stop ();
											else
												{
													task->next_time = time_now + task->interval;
												}
										}
								}
							
							++ itr;
						}
				}
				
				std::this_thread::sleep_for (
					std::chrono::milliseconds (this->sleep_interval));
			}
	}
	
	
	
	/* 
	 * Starts the scheduler's main thread and begin executing tasks.
	 */
	void
	scheduler::start ()
	{
		if (this->running)
			return;
		
		this->running = true;
		this->main_thread = new std::thread (
			std::bind (std::mem_fn (&hCraft::scheduler::main_loop), this));
	}
	
	/* 
	 * Stops the scheduler and removes all registered tasks.
	 */
	void
	scheduler::stop ()
	{
		if (!this->running)
			return;
		
		this->running = false;
		if (this->main_thread->joinable ())
			this->main_thread->join ();
		delete this->main_thread;
	}
	
	
	
	/* 
	 * Creates and returns a new task.
	 */
	scheduler_task&
	scheduler::new_task (std::function<void (scheduler_task&)>&& cb, void *ctx)
	{
		return *(new scheduler_task (*this, std::move (cb), ctx));
	}
	
	
	
	/* 
	 * Adds the specified task to the scheduler's task list.
	 */
	void
	scheduler::add_task (scheduler_task *task)
	{
		std::lock_guard<std::mutex> guard {this->lock};
		
		this->tasks.push_back (task);
		
		int tick_count = task->interval.count ();
		if (tick_count < sleep_interval && (tick_count >= 10))
			{
				this->sleep_interval = tick_count;
			}
	}
}

