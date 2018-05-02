#pragma once
#include <mutex>

class semaphore {
public:
	explicit semaphore(const long count = 0)
		: count_(count) {
	}

	void set() {
		std::unique_lock<std::mutex> lock(mutex_);
		++count_;
		cv_.notify_one();
	}
	void reset() {
		std::unique_lock<std::mutex> lock(mutex_);
		count_ = 0;
	}

	void wait_one() {
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock, [=] { return count_ > 0; });
		--count_;
	}

private:
	std::mutex mutex_;
	std::condition_variable cv_;
	long count_;
};
