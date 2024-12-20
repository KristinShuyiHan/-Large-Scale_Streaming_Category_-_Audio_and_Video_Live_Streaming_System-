#pragma once  
// 防止头文件被重复包含，确保头文件只被编译一次  
// Prevents this header file from being included multiple times

#include "Event.h"         // 引入事件类的头文件，用于表示事件  
#include "PipeEvent.h"     // 引入管道事件的头文件，处理线程间通信的唤醒机制  
#include "TimingWheel.h"   // 引入时间轮定时器的头文件，用于定时任务管理  
#include <vector>          // 使用 vector 容器  
#include <sys/epoll.h>     // epoll 系统调用，用于高效 I/O 事件监听  
#include <memory>          // 智能指针 std::shared_ptr  
#include <unordered_map>   // 使用哈希表存储事件  
#include <functional>      // std::function 用于存储回调函数  
#include <queue>           // 使用队列存储任务  
#include <mutex>           // 互斥锁，保护共享数据线程安全  

namespace tmms  // 声明一个命名空间 `tmms`  
// Namespace 是一个逻辑分区，用来组织代码，避免命名冲突  
// Namespace groups related classes and functions together to avoid name conflicts
{
    namespace network // 在 `tmms` 命名空间下定义了子命名空间 `network`  
    {
        using EventPtr = std::shared_ptr<Event>;  
        // 定义 EventPtr 为 std::shared_ptr<Event> 的别名，表示事件的智能指针  
        // EventPtr is an alias for shared_ptr<Event>, which safely manages Event objects.

        using Func = std::function<void()>;  
        // 定义 Func 为无参无返回值的回调函数类型  
        // Func is an alias for a callable function object that takes no arguments and returns void.

        // 定义 EventLoop 类，负责事件的监听、处理和调度  
        class EventLoop  
        {
        public:
            EventLoop();  // 构造函数，初始化事件循环  
            ~EventLoop(); // 析构函数，清理资源  

            void Loop();  // 启动事件循环，处理事件和任务  
            void Quit();  // 退出事件循环  

            void AddEvent(const EventPtr &event);  // 添加一个事件到监听列表  
            void DelEvent(const EventPtr &event);  // 删除一个事件  

            bool EnableEventWriting(const EventPtr &event, bool enable);  
            // 启用/禁用事件的写操作监听  
            // Enable or disable monitoring write events.

            bool EnableEventReading(const EventPtr &event, bool enable);  
            // 启用/禁用事件的读操作监听  
            // Enable or disable monitoring read events.

            void AssertInLoopThread();  // 断言当前线程是否为事件循环线程  
            bool IsInLoopThread() const;  // 判断是否在事件循环线程中  

            void RunInLoop(const Func &f);  // 添加回调函数到事件循环  
            void RunInLoop(Func &&f);       // 移动语义版本，减少开销  
            // RunInLoop 把任务（函数）放入队列，在事件循环中执行  
            // Adds a function to the task queue to be executed in the loop.

            void InsertEntry(uint32_t delay, EntryPtr entryPtr);  
            // 在时间轮中插入一个延时任务  
            // Adds a delayed task into the timing wheel.

            void RunAfter(double delay, const Func &cb);  
            void RunAfter(double delay, Func &&cb);  
            // 在指定延迟后执行回调函数  
            // Executes a callback function after a specific delay.

            void RunEvery(double interval, const Func &cb);  
            void RunEvery(double interval, Func &&cb);  
            // 每隔指定时间间隔重复执行回调函数  
            // Executes a callback function repeatedly at a fixed interval.

        private:
            void RunFuctions();  // 运行队列中的任务函数  
            void WakeUp();       // 唤醒事件循环，例如通过管道事件  
            
            bool looping_{false};  // 事件循环是否正在运行  
            int epoll_fd_{-1};     // epoll 文件描述符，用于监听事件  
            std::vector<struct epoll_event> epoll_events_;  
            // epoll 返回的事件列表，用于处理就绪事件  

            std::unordered_map<int, EventPtr> events_;  
            // 哈希表存储事件，key 是文件描述符，value 是事件的智能指针  
            // A hash map to store events with file descriptor as the key.

            std::queue<Func> functions_;  // 存储需要执行的任务队列  
            std::mutex lock_;             // 互斥锁，保护任务队列的线程安全  

            PipeEventPtr pipe_event_;     // 管道事件，用于跨线程唤醒事件循环  
            TimingWheel wheel_;           // 时间轮，用于管理定时任务  
        };
    }
}
