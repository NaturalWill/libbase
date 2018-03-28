#pragma once
#include "queue_s.h"


template <typename T>
class queue_limited_s

{
private:
	size_t max_len_;
	// data_queue访问信号量
	mutable std::mutex mut_;
	mutable std::condition_variable cond_pop_data_;
	mutable std::condition_variable cond_push_data_;
	using queue_type = std::queue<T>;
	queue_type data_queue_;
public:

	using value_type = typename queue_type::value_type;
	using container_type = typename queue_type::container_type;
	queue_limited_s(size_t max_len) :max_len_(max_len)
	{

	}
	~queue_limited_s() = default;

	queue_limited_s(const queue_limited_s&) = delete;
	queue_limited_s& operator=(const queue_limited_s&) = delete;
	queue_limited_s(queue_limited_s&&) = delete;
	queue_limited_s& operator=(queue_limited_s&&) = delete;


	/**
	* \brief 尝试向队列中加入一个元素,如果队列已满返回false
	* */
	bool try_push(const value_type &new_value) {
		std::lock_guard<std::mutex>lk(mut_);
		if (data_queue_.size() >= max_len_)
			return false;
		data_queue_.push(std::move(new_value));
		cond_pop_data_.notify_one();
		return true;
	}
	/**
	*  \brief 将元素加入队列
	* */
	void push(const value_type &new_value) {
		std::lock_guard<std::mutex>lk(mut_);

		data_queue_.push(std::move(new_value));
		cond_pop_data_.notify_one();
	}




	/**
	* \brief 在一定时间内尝试从队列中弹出一个元素,如果超时则返回 false
	* \param value out value
	* \param spend_time milliseconds
	* \return
	*/
	bool try_pop_for(value_type& value, const int64_t spend_time) {

		//mtx_.try_lock_for(std::chrono::milliseconds(spend_time));
		std::lock_guard<std::mutex>lk(mut_);

		cond_pop_data_.wait_for(lk, std::chrono::milliseconds(spend_time), empty);
		if (data_queue_.empty())
			return false;

		value = std::move(data_queue_.front());
		data_queue_.pop();
		return true;
	}
	/**
	* \brief 从队列中弹出一个元素,如果队列为空返回false
	* */
	bool try_pop(value_type& value) {
		std::lock_guard<std::mutex>lk(mut_);
		if (data_queue_.empty())
			return false;
		value = std::move(data_queue_.front());
		data_queue_.pop();
		return true;
	}

	/**
	* \brief 从队列中弹出一个元素,如果队列为空就阻塞
	* */
	value_type wait_and_pop() {
		std::unique_lock<std::mutex>lk(mut_);

		cond_pop_data_.wait(lk, [this] {return !this->data_queue_.empty(); });
		auto value = std::move(data_queue_.front());
		data_queue_.pop();
		return value;
	}

	/**
	* \brief 返回队列是否为空
	* */
	auto empty() const->decltype(data_queue_.empty()) {
		std::lock_guard<std::mutex>lk(mut_);
		return data_queue_.empty();
	}
	/**
	* \brief 返回队列中元素数个
	* */
	auto size() const->decltype(data_queue_.size()) {
		std::lock_guard<std::mutex>lk(mut_);
		return data_queue_.size();
	}


};
