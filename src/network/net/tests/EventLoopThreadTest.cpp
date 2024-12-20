#include "network/net/EventLoop.h"  // 包含 EventLoop 事件循环的头文件
#include "network/net/EventLoopThread.h" // 包含单个 EventLoop 线程管理类
#include "network/net/EventLoopThreadPool.h" // 包含 EventLoop 线程池类
#include "network/net/PipeEvent.h" // 包含管道事件类
#include "base/TTime.h"  // 时间工具类
#include <iostream>
#include <thread>

using namespace tmms::network; // 使用 tmms::network 命名空间

EventLoopThread eventloop_thread; // 创建一个单独的事件循环线程
std::thread th;  // 创建一个全局线程对象

// 测试单线程事件循环
void TestEventLoopThread()
{
    eventloop_thread.Run(); // 启动事件循环线程
    EventLoop *loop = eventloop_thread.Loop(); // 获取事件循环对象指针

    if(loop) // 确保 loop 获取成功
    {
        std::cout << "loop:" << loop << std::endl; // 输出事件循环指针地址
        PipeEventPtr pipe = std::make_shared<PipeEvent>(loop); // 创建一个管道事件，绑定当前 loop
        loop->AddEvent(pipe); // 将管道事件添加到事件循环中

        // 写入初始测试数据到管道
        int64_t test = 12345;
        pipe->Write((const char*)&test, sizeof(test)); // 将 test 数据写入管道

        // 创建一个新线程，不断向管道写入当前时间
        th = std::thread([&pipe](){
            while(1) // 无限循环
            {
                int64_t now = tmms::base::TTime::NowMS(); // 获取当前时间（毫秒）
                pipe->Write((const char*)&now, sizeof(now)); // 将当前时间写入管道
                std::this_thread::sleep_for(std::chrono::seconds(1)); // 线程暂停 1 秒
            }
        });

        while(1) // 主线程无限循环
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 每 50 毫秒睡眠一次，避免空跑
        }
    }
}

// 测试事件循环线程池
void TestEventLoopThreadPool()
{
    EventLoopThreadPool pool(2, 0, 2); // 创建一个线程池，初始化 2 个线程
    pool.Start(); // 启动线程池，创建多个 EventLoop 线程

    EventLoop *loop = pool.GetNextLoop(); // 从线程池中获取一个 EventLoop
    std::cout << "loop:" << loop << std::endl; // 输出获取的 EventLoop 对象

    // 添加一个延时任务：1 秒后执行
    loop->RunAfter(1, [](){
        std::cout << "run after 1s now:" << tmms::base::TTime::Now() << std::endl;
    });

    // 添加一个延时任务：5 秒后执行
    loop->RunAfter(5, [](){
        std::cout << "run after 5s now:" << tmms::base::TTime::Now() << std::endl;
    });

    // 添加一个周期性任务：每 1 秒执行一次
    loop->RunEvery(1, [](){
        std::cout << "run every 1s now:" << tmms::base::TTime::Now() << std::endl;
    });

    // 添加一个周期性任务：每 5 秒执行一次
    loop->RunEvery(5, [](){
        std::cout << "run every 5s now:" << tmms::base::TTime::Now() << std::endl;
    });

    // 主线程无限循环，防止程序退出
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 每 1 秒暂停一次
    }      
}

// 程序的入口点
int main(int argc, const char ** argv)
{
    TestEventLoopThreadPool(); // 测试线程池中的事件循环

    while(1)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 主线程无限循环，防止程序退出
    }

    return 0; // 返回 0 表示程序正常退出（实际上不会执行到这里）
}
