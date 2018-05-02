#pragma once
/*
* queue_s.h
*
*/

#ifndef COMMON_SOURCE_CPP_THREADSAFE_QUEUE_H_
#define COMMON_SOURCE_CPP_THREADSAFE_QUEUE_H_
#include <queue>
#include <mutex>
#include <condition_variable>
#include <initializer_list>


/**
* \brief 线程安全队列
*
* 因为有std::mutex和std::condition_variable类成员,所以此类不支持复制构造函数也不支持赋值操作符(=)
* \tparam T T为队列元素类型
*/
template<typename T>
class queue_s {
protected:
	// data_queue访问信号量
	mutable std::mutex mut_;
	mutable std::condition_variable cond_pop_data_;
	mutable std::condition_variable cond_push_data_;
	using queue_type = std::queue<T>;
	queue_type data_queue_;
public:
	using value_type = typename queue_type::value_type;
	using container_type = typename queue_type::container_type;
	queue_s() = default;
	~queue_s() = default;
	queue_s(const queue_s&) = delete;
	queue_s& operator=(const queue_s&) = delete;
	queue_s(queue_s&&) = delete;
	queue_s& operator=(queue_s&&) = delete;
	/**
	* \brief 使用迭代器为参数的构造函数,适用所有容器对象
	*/
	template<typename _InputIterator>
	queue_s(_InputIterator first, _InputIterator last) {
		for (auto itor = first; itor != last; ++itor) {
			data_queue_.push(*itor);
		}
	}
	explicit queue_s(const container_type &c) :data_queue_(c) {}

	/**
	* \brief 使用初始化列表为参数的构造函数
	* \param list
	*/
	queue_s(std::initializer_list<value_type> list) :queue_s(list.begin(), list.end()) {
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
}; /* queue_s */
#endif /* COMMON_SOURCE_CPP_THREADSAFE_QUEUE_H_ */

