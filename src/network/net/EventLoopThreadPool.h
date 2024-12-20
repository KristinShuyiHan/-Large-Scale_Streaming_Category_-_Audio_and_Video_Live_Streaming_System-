#pragma once
// 防止头文件被重复包含，确保该头文件只会被编译一次。
// Prevents the header file from being included multiple times during compilation.

#include "base/NonCopyable.h"  
// 引入 NonCopyable 类，表示该类无法被拷贝或赋值，继承此类会禁用拷贝构造和赋值操作符。
// Includes the NonCopyable class to make the derived class non-copyable (copy constructor and assignment operator are deleted).

#include "EventLoop.h"  
// 引入 EventLoop 类，用于事件循环的处理。
// Includes EventLoop class, which handles event loops.

#include "EventLoopThread.h"  
// 引入 EventLoopThread 类，表示每个线程将运行一个事件循环。
// Includes EventLoopThread, indicating that each thread will run an EventLoop.

#include <vector>  
// 引入 vector 容器，用于存储多个 EventLoopThread 对象。
// Includes the vector container to store multiple EventLoopThread objects.

#include <atomic>  
// 引入 atomic 库，用于线程安全的原子操作，主要用于 loop_index_。
// Includes atomic for thread-safe atomic operations, primarily for loop_index_.

namespace tmms  
{
    namespace network  
    {
        using EventLoopThreadPtr = std::shared_ptr<EventLoopThread>;
        // 定义别名 EventLoopThreadPtr，简化 shared_ptr<EventLoopThread> 的使用。
        // Defines an alias for shared_ptr<EventLoopThread> to simplify its usage.

        class EventLoopThreadPool : public base::NonCopyable  
        // EventLoopThreadPool 类继承 NonCopyable，表示该类不能被拷贝或赋值。
        // The EventLoopThreadPool class inherits from NonCopyable, making it non-copyable.

        {
        public:
            EventLoopThreadPool(int thread_num, int start = 0, int cpus = 4);
            // 构造函数：
            // - thread_num: 线程数量（EventLoopThread 数量）
            // - start: 起始线程索引（默认为 0）
            // - cpus: CPU 核心数量（默认为 4）
            // Constructor:
            // - thread_num: Number of threads (EventLoopThreads).
            // - start: Starting thread index (default is 0).
            // - cpus: Number of CPU cores (default is 4).

            ~EventLoopThreadPool();
            // 析构函数，清理资源。
            // Destructor to clean up resources.

            std::vector<EventLoop *> GetLoops() const;
            // 获取所有 EventLoop 对象的指针，并将其存储在 vector 中返回。
            // Returns pointers to all EventLoop objects in a vector.

            EventLoop *GetNextLoop();
            // 轮询获取下一个 EventLoop 对象指针，确保负载均衡。
            // Returns the next EventLoop pointer in a round-robin fashion for load balancing.

            size_t Size();
            // 返回线程池中线程的数量。
            // Returns the number of threads in the pool.

            void Start();
            // 启动线程池，创建并启动所有的 EventLoopThread。
            // Starts the thread pool by creating and starting all EventLoopThreads.

        private:
            std::vector<EventLoopThreadPtr> threads_;
            // 线程池中存储所有 EventLoopThread 的智能指针。
            // A vector to store shared pointers to EventLoopThread objects in the thread pool.

            std::atomic_int32_t loop_index_{0};
            // 原子操作变量 loop_index_，用于轮询获取下一个 EventLoop，确保线程安全。
            // Atomic variable loop_index_ to safely retrieve the next EventLoop in a thread-safe way.
        };
    }
}
