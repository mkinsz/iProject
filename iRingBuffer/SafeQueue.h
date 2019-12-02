#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>

template <typename T>
class SafeQueue {
public:
	SafeQueue() {}
	~SafeQueue() {}

	bool empty() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}

	int size() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

	void enqueue(T& t) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(t);
		m_cv.notify_one();
	}

	bool dequeue(T& t) {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [&] { return !m_queue.empty(); });

		if (m_queue.empty())
			return false;

		t = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}

private:
	std::queue<T>	m_queue;
	std::mutex		m_mutex;
	std::condition_variable m_cv;
};