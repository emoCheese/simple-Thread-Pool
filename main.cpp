#include <iostream>
#include <vector>
#include <chrono>  // 用于线程休眠
#include "MyThreadPool.hpp"

int main()
{
    // create thread pool with 4 worker threads
    MyThreadPool pool(4);

    // 用于获取结果
    std::vector< std::future<int> > results;

    for (int i = 0; i < 8; ++i) {
        results.emplace_back(

            // 向线程池里添加任务
            pool.submitTask([i] {
                std::cout << "hello " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << i << std::endl;
                return i * (i+1);
                })
        );
    }

    for (auto&& result : results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;

    return 0;
}
