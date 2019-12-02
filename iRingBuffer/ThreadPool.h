#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "SafeQueue.h"

class ThreadPool {
public:
	ThreadPool(const int nthreads) : m_threads(std::vector<std::thread>(nthreads)), m_shutdown(false) {}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;

	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	//inits thread pool
	void init() {
		for (int i = 0; i < m_threads.size(); ++i) {
			m_threads[i] = std::thread(ThreadWorker(this, i));
		}
	}

	//waits until threads finish their current task and shutdowns the pool
	void shutdown() {
		m_shutdown = true;
		m_cv.notify_all();

		for (int i = 0; i < m_threads.size(); ++i) {
			if (m_threads[i].joinable()) {
				m_threads[i].join();
			}
		}
	}

	template<typename F, typename... Args>
	auto submit(F&& f, Args&& ...args) -> std::future<decltype(f(args...))> {
		// create a function with bounded parameters ready to execute
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		// encapsulate it into a shared ptr in order to be able to copy construct / assign
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

		//wrap packaged task into void function
		std::function<void()> wrapper_func = [task_ptr]() {
			(*task_ptr)();
		};

		//enqueue generic wrapper function
		m_queue.enqueue(wrapper_func);

		//wake up one thread if its waiting
		m_cv.notify_one();

		//return future from promise
		return task_ptr->get_future();
	}

private:
	class ThreadWorker {
	public: 
		ThreadWorker(ThreadPool* pool, const int id) : m_pool(pool), m_id(id) {}

		void operator()() {
			std::function<void()> func;
			bool dequeued;
			while (!m_pool->m_shutdown) {
				{
					std::unique_lock<std::mutex> lock(m_pool->m_mutex);
					if (m_pool->m_queue.empty()) {
						m_pool->m_cv.wait(lock);
					}
					dequeued = m_pool->m_queue.dequeue(func);
				}

				if (dequeued)
					func();
			}
		}

	private:
		int m_id;
		ThreadPool* m_pool;
	};

private:
	bool m_shutdown;
	SafeQueue<std::function<void()>> m_queue;
	std::vector<std::thread> m_threads;
	std::mutex m_mutex;
	std::condition_variable m_cv;
};