#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class queue_limited_s

{
private:
	size_t max_len_ = 0;
	// data_queue访问信号量
	mutable std::mutex mut_;
	mutable std::condition_variable cond_has_data_;
	mutable std::condition_variable cond_has_space_;
	using queue_type = std::queue<T>;
	queue_type data_queue_;
public:

	using value_type = typename queue_type::value_type;
	using container_type = typename queue_type::container_type;
	queue_limited_s(const size_t len = 0) :max_len_(len)
	{

	}
	~queue_limited_s() = default;

	queue_limited_s(const queue_limited_s&) = delete;
	queue_limited_s& operator=(const queue_limited_s&) = delete;
	queue_limited_s(queue_limited_s&&) = delete;
	queue_limited_s& operator=(queue_limited_s&&) = delete;

	void set_max_len(const size_t len)
	{
		std::lock_guard<std::mutex>lk(mut_);
		max_len_ = len;
	}


#pragma region push



	/**
	* \brief 尝试向队列中加入一个元素,如果队列已满返回false
	* */
	bool try_push(const value_type &new_value) {
		std::lock_guard<std::mutex>lk(mut_);
		if (full())
			return false;
		data_queue_.push(std::move(new_value));
		cond_has_data_.notify_one();
		return true;
	}
	/**
	*  \brief 将元素加入队列
	* */
	void push(const value_type &new_value) {
		std::lock_guard<std::mutex>lk(mut_);

		data_queue_.push(std::move(new_value));
		cond_has_data_.notify_one();
	}
	template<class _Predicate>
	bool wait_push_while(value_type& value, const int64_t spend_time, _Predicate _Pred) {
		std::unique_lock<std::mutex>lk(mut_);
		while (full())
		{
			cond_has_space_.wait_for(lk, std::chrono::milliseconds(spend_time), [this] {return !full(); });
			if (!_Pred())
			{
				return false;
			}
		}
		//mtx_.try_lock_for(std::chrono::milliseconds(spend_time));

		if (full())
			return false;

		data_queue_.push(std::move(value));
		cond_has_data_.notify_one();

		return true;
	}

#pragma endregion




#pragma region pop


	/**
	* \brief 在一定时间内尝试从队列中弹出一个元素,如果超时则返回 false
	* \param value out value
	* \param milliseconds milliseconds
	* \return
	*/
	bool try_pop_for(value_type& value, const int64_t milliseconds) {

		//mtx_.try_lock_for(std::chrono::milliseconds(milliseconds));
		std::unique_lock<std::mutex>lk(mut_);

		cond_has_data_.wait_for(lk, std::chrono::milliseconds(milliseconds), [this] {return !this->data_queue_.empty(); });
		if (data_queue_.empty())
			return false;

		value = std::move(data_queue_.front());
		data_queue_.pop();
		cond_has_space_.notify_one();
		return true;
	}

	template<class _Predicate>
	bool wait_pop_while(value_type& value, const int64_t spend_time, _Predicate _Pred) {
		std::unique_lock<std::mutex>lk(mut_);
		while (data_queue_.empty())
		{
			cond_has_data_.wait_for(lk, std::chrono::milliseconds(spend_time), [this] {return !this->data_queue_.empty(); });
			if (!_Pred())
			{
				return false;
			}
		}
		//mtx_.try_lock_for(std::chrono::milliseconds(spend_time));

		if (data_queue_.empty())
			return false;

		value = std::move(data_queue_.front());
		data_queue_.pop();
		cond_has_space_.notify_one();
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
		cond_has_space_.notify_one();
		return true;
	}

	/**
	* \brief 从队列中弹出一个元素,如果队列为空就阻塞
	* */
	value_type wait_and_pop() {
		std::unique_lock<std::mutex>lk(mut_);

		cond_has_data_.wait(lk, [this] {return !this->data_queue_.empty(); });
		auto value = std::move(data_queue_.front());
		data_queue_.pop();
		cond_has_space_.notify_one();
		return value;
	}
#pragma endregion


	/**
	* \brief 返回队列是否为空
	* */
	auto empty() const->decltype(data_queue_.empty()) {
		std::lock_guard<std::mutex>lk(mut_);
		return data_queue_.empty();
	}
	auto full() const {
		//std::lock_guard<std::mutex>lk(mut_);		
		return (max_len_ > 0 && data_queue_.size() >= max_len_);
	}

	/**
	* \brief 返回队列中元素数个
	* */
	auto size() const->decltype(data_queue_.size()) {
		std::lock_guard<std::mutex>lk(mut_);
		return data_queue_.size();
	}


};
