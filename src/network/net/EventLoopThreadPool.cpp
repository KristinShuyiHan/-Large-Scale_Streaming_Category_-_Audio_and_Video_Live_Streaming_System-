#include "EventLoopThreadPool.h"   // 包含 EventLoopThreadPool 类的头文件。
// #include 头文件是为了使用 EventLoopThreadPool 类定义和实现。

#include <pthread.h>  // 包含 pthread 线程库，用于线程操作和 CPU 绑定。

using namespace tmms::network;   // 使用 tmms::network 命名空间，简化代码书写。

namespace
{
    // 1. **bind_cpu** 函数: 将线程绑定到特定的 CPU 核心上。
    // **中文**：这个函数可以把线程分配到特定的 CPU 核心，帮助程序实现 CPU 亲和性，提高多核 CPU 下的性能。
    void bind_cpu(std::thread &t, int n)   // 参数: t - 线程对象，n - CPU 核心编号。
    {
        cpu_set_t cpu;       // 定义一个 cpu_set_t 结构体，用于保存 CPU 核心集合。

        CPU_ZERO(&cpu);      // **清空 CPU 集合**，将所有 CPU 号设为未选中状态。
        CPU_SET(n, &cpu);    // **选择第 n 号 CPU**，将第 n 号 CPU 添加到集合中。

        // **将线程绑定到指定的 CPU**。
        // pthread_setaffinity_np 是非标准的 POSIX 接口，用于设置线程的 CPU 亲和性。
        pthread_setaffinity_np(t.native_handle(), sizeof(cpu), &cpu);
    }
}

EventLoopThreadPool::EventLoopThreadPool(int thread_num, int start, int cpus)
{
    // **构造函数**：初始化线程池。
    // 参数说明:
    // - thread_num: 线程数量。
    // - start: 起始 CPU 核心编号。
    // - cpus: 可用的 CPU 核心数。

    if (thread_num <= 0)  // 如果线程数 <= 0，默认设置为 1。
    {
        thread_num = 1;
    }

    for (int i = 0; i < thread_num; i++)   // 循环创建指定数量的线程。
    {
        // 创建一个新的 EventLoopThread 并加入线程列表。
        threads_.emplace_back(std::make_shared<EventLoopThread>());

        if (cpus > 0)   // 如果 CPU 核心数 > 0，绑定线程到具体 CPU 核心。
        {
            int n = (start + i) % cpus;  // 计算当前线程绑定的 CPU 核心编号。
            bind_cpu(threads_.back()->Thread(), n);   // 调用 bind_cpu 绑定线程。
        }
    }
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // **析构函数**：没有特殊操作，因为智能指针会自动释放资源。
}

std::vector<EventLoop *> EventLoopThreadPool::GetLoops() const
{
    // **GetLoops 方法**：返回线程池中所有线程对应的 EventLoop 指针。
    std::vector<EventLoop *> result;   // 创建一个 EventLoop 指针的向量。

    for (auto &t : threads_)   // 遍历所有线程对象。
    {
        result.emplace_back(t->Loop());  // 将每个线程的 EventLoop 添加到结果中。
    }

    return result;   // 返回所有 EventLoop 指针的集合。
}

EventLoop *EventLoopThreadPool::GetNextLoop()
{
    // **GetNextLoop 方法**: 获取下一个可用的 EventLoop。
    int index = loop_index_;  // 记录当前的索引。
    loop_index_++;            // 索引加 1，为下一个调用做准备。

    // 通过取模运算，循环获取线程池中的线程。
    return threads_[index % threads_.size()]->Loop();
}

size_t EventLoopThreadPool::Size()
{
    // **Size 方法**：返回线程池中线程的数量。
    return threads_.size();
}

void EventLoopThreadPool::Start()
{
    // **Start 方法**：启动线程池中的所有线程。
    for (auto &t : threads_)   // 遍历所有线程。
    {
        t->Run();   // 启动每个线程，调用 Run() 方法运行线程逻辑。
    }
}
