#ifndef _MYHTREADPOOL_H
#define _MYHTREADPOOL_H

#include <vector>
#include <queue>
#include <functional>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <future>

class MyThreadPool
{
public:
	MyThreadPool(int num);

	template<class Func, class... Args>

	auto submitTask(Func&& f, Args&&... args)
		->std::future<typename std::result_of<Func(Args...)>::type>;

	~MyThreadPool();

private:
	MyThreadPool operator=(const MyThreadPool& a) = delete;
	MyThreadPool(MyThreadPool&& a) = delete;

private:
	std::vector<std::thread> m_thread;    // 线程数组
	std::queue<std::function<void()>> m_taskQuene;  // 任务队列

	std::mutex m_mtx;
	std::condition_variable m_cv;
	std::atomic<bool> m_stop;

};

// 创建线程
inline MyThreadPool::MyThreadPool(int num)
	: m_stop(false)
{
	for (int i = 0; i < num; ++i)
	{
		auto work = [this]() {
			std::function<void()> task;
			while (true)
			{
				{
					std::unique_lock<std::mutex> lock(this->m_mtx); // 取任务加锁
					this->m_cv.wait(lock, [this]()->bool {return !this->m_taskQuene.empty() || this->m_stop; });
					if (this->m_stop && this->m_taskQuene.empty())
						return;
					task = std::move(this->m_taskQuene.front());
					this->m_taskQuene.pop();
				}
				
				task();
			}
		};
		m_thread.emplace_back(work);
	}
}

inline MyThreadPool::~MyThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(m_mtx);
		m_stop = true;
	}
	m_cv.notify_all();
	for (std::thread& t : m_thread)
		t.join();
}

template<class Func, class... Args>
auto MyThreadPool::submitTask(Func&& f, Args&&... args)
	->std::future<typename std::result_of<Func(Args...)>::type>
{
	// decltype 用于获取函数返回值类型
	using returnType = typename std::result_of<Func(Args...)>::type;
	// 先打包成无参 void() 类型，再用packaged_task打包
	auto taskptr = std::make_shared<std::packaged_task<returnType()>>(
		std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
	);
	{
		std::unique_lock<std::mutex> lock(m_mtx);
		m_taskQuene.emplace( [taskptr]() { (*taskptr)(); } );
	}
	m_cv.notify_one();
	return taskptr->get_future();
}

#endif