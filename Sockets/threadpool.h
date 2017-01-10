/*
	Thread pool class
*/

#pragma once

#include <thread>
#include <vector>

#include "threadqueue.h"

class ThreadPool
{
public:

	

	struct ITask
	{
		virtual void execute() = 0;
		virtual ~ITask() {}
	};

	ThreadPool() : ThreadPool((int)std::thread::hardware_concurrency()) {}

	ThreadPool(int num_threads)
	{
		for (int i = 0; i < num_threads; i++)
		{
			m_threads.push_back(std::thread(&ThreadPool::procedure, this, i));
		}
	}

	explicit ThreadPool(const ThreadPool&) = delete;

	~ThreadPool()
	{
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			m_queue.push(std::make_pair(false, nullptr));
		}

		for (auto& t : m_threads)
			t.join();
	}

	void add_task(ITask* task)
	{
		m_queue.push(std::make_pair(true, task));
	}

	void operator<<(ITask* task)
	{
		return add_task(task);
	}

private:

	typedef std::pair<bool, ITask*> package_t;

	std::vector<std::thread> m_threads;
	ThreadQueue<package_t> m_queue;

	void procedure(int thread_index)
	{
		while (true)
		{
			package_t pack = m_queue.pop();

			if (!pack.first)
				break;

			pack.second->execute();
		}
	}
};