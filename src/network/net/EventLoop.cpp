#include "EventLoop.h"
#include "network/base/Network.h"
#include "base/TTime.h"
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace tmms::network;

// 使用 thread_local 保证每个线程只能有一个 EventLoop 实例。
// Ensures that each thread has its own EventLoop instance.
static thread_local EventLoop * t_local_eventloop = nullptr;

// EventLoop 构造函数: 初始化 epoll 实例，并设置事件列表大小为 1024。
// EventLoop constructor: Initializes the epoll instance and event list size to 1024.
EventLoop::EventLoop()
:epoll_fd_(::epoll_create(1024)), // 创建 epoll 文件描述符
epoll_events_(1024)              // 分配初始事件列表大小为 1024
{
    if(t_local_eventloop) // 如果当前线程已有 EventLoop，则报错退出
    {
        NETWORK_ERROR << " there alread had a eventloop!!!";
        exit(-1);
    }
    t_local_eventloop = this; // 将当前 EventLoop 赋给线程局部变量
}

// EventLoop 析构函数，调用 Quit() 停止事件循环。
// EventLoop destructor: Calls Quit() to stop the event loop.
EventLoop::~EventLoop()
{
    Quit();
}

// EventLoop 主循环，持续监听 epoll 事件并处理它们。
// The main loop of EventLoop that continuously listens for and processes epoll events.
void EventLoop::Loop()
{
    looping_ = true; // 标记进入事件循环
    int64_t timeout = 1000; // epoll_wait 超时时间为 1000 毫秒

    while(looping_) // 无限循环，直到调用 Quit()
    {
        // 清零事件数组，避免旧数据影响
        memset(&epoll_events_[0],0x00,sizeof(struct epoll_event)*epoll_events_.size());

        // 调用 epoll_wait 等待事件触发
        auto ret = ::epoll_wait(epoll_fd_,
                                (struct epoll_event*)&epoll_events_[0],
                                static_cast<int>(epoll_events_.size()),
                               timeout);

        if(ret >= 0) // 成功返回有事件触发
        {
            for(int i = 0; i < ret; i++) // 遍历每个触发的事件
            {
                struct epoll_event &ev = epoll_events_[i];
                if(ev.data.fd <= 0) // 如果文件描述符无效，跳过
                {
                    continue;
                }

                // 查找当前事件对应的 Event
                auto iter = events_.find(ev.data.fd);
                if(iter == events_.end())
                {
                    continue;
                }

                EventPtr &event = iter->second; // 获取事件对象

                // 处理错误事件
                if(ev.events & EPOLLERR)
                {
                    int error = 0;
                    socklen_t len = sizeof(error);
                    getsockopt(event->Fd(), SOL_SOCKET, SO_ERROR, &error, &len);
                    event->OnError(strerror(error)); // 调用错误回调
                    continue;
                }
                
                // 处理连接关闭事件
                if((ev.events & EPOLLHUP) && !(ev.events & EPOLLIN))
                {
                    event->OnClose(); // 调用关闭回调
                    continue;
                }

                // 处理读事件
                if(ev.events & (EPOLLIN | EPOLLPRI))
                {
                    event->OnRead(); // 调用读回调
                }
                
                // 处理写事件
                if(ev.events & EPOLLOUT)
                {
                    event->OnWrite(); // 调用写回调
                }
            }

            // 如果事件数组满了，动态扩展容量
            if(ret == epoll_events_.size())
            {
                epoll_events_.resize(epoll_events_.size()*2);
            }

            RunFuctions(); // 执行通过 RunInLoop 添加的回调函数
            int64_t now = tmms::base::TTime::NowMS();
            wheel_.OnTimer(now); // 定时任务管理
        }
        else if(ret < 0) // 处理 epoll_wait 错误
        {
            NETWORK_ERROR << "epoll wait error.error:" << errno;
        }
    }
}

// 退出事件循环
// Exits the event loop.
void EventLoop::Quit()
{
    looping_ = false; // 设置循环标志为 false，事件循环会退出
}

// 添加事件到 epoll 监听中
// Adds an event to the epoll instance for monitoring.
void EventLoop::AddEvent(const EventPtr &event)
{
    auto iter = events_.find(event->Fd());
    if(iter != events_.end()) // 如果事件已存在，直接返回
    {
        return;
    }
    event->event_ |= kEventRead; // 默认启用读事件
    events_[event->Fd()] = event; // 添加到事件映射

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = event->event_; // 设置事件类型
    ev.data.fd = event->fd_;   // 设置事件关联的文件描述符
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event->fd_, &ev); // 添加到 epoll
}

// 删除事件
// Removes an event from the epoll instance.
void EventLoop::DelEvent(const EventPtr &event)
{
    auto iter = events_.find(event->Fd());
    if(iter == events_.end()) // 如果事件不存在，直接返回
    {
        return;
    }
    events_.erase(iter); // 从事件映射中删除

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = event->event_;
    ev.data.fd = event->fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event->fd_, &ev); // 从 epoll 删除
}

bool EventLoop::EnableEventWriting(const EventPtr &event, bool enable)
{
    auto iter = events_.find(event->Fd()); // 检查事件的文件描述符是否在事件列表中
    if (iter == events_.end())             // 如果找不到该事件，输出错误并返回 false
    {
        NETWORK_ERROR << "cant find event fd:" << event->Fd();
        return false;
    }
    
    if (enable) // 如果 enable 为 true，启用写事件
    {
        event->event_ |= kEventWrite; // 设置写事件的标志位
    }
    else 
    {
        event->event_ &= ~kEventWrite; // 否则清除写事件的标志位
    }

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event)); // 初始化 epoll 事件结构体
    ev.events = event->event_;  // 设置事件类型（包含读/写事件标志位）
    ev.data.fd = event->fd_;    // 设置文件描述符

    // 使用 epoll_ctl 修改事件监听状态：EPOLL_CTL_MOD 表示修改事件
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &ev);
    return true; // 成功返回 true
}

// 启用或禁用事件的读取功能
bool EventLoop::EnableEventReading(const EventPtr &event, bool enable)
{
    auto iter = events_.find(event->Fd()); // 在事件集合中查找对应文件描述符 (fd)
    if (iter == events_.end()) // 如果未找到，记录错误日志并返回 false
    {
        NETWORK_ERROR << "cant find event fd:" << event->Fd();
        return false;
    }

    if (enable) // 如果启用事件读取
    {
        event->event_ |= kEventRead; // 使用位或操作，添加可读事件标志位
    }
    else // 如果禁用事件读取
    {
        event->event_ &= ~kEventRead; // 使用位与和取反操作，移除可读事件标志位
    }

    struct epoll_event ev; // 定义 epoll 事件结构体
    memset(&ev, 0x00, sizeof(struct epoll_event)); // 将结构体内存清零
    ev.events = event->event_; // 设置事件的类型 (读/写/错误等)
    ev.data.fd = event->fd_; // 设置文件描述符

    // 修改 epoll 中对应 fd 的事件状态，EPOLL_CTL_MOD 表示修改操作
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &ev);
    return true; // 操作成功，返回 true
}
void EventLoop::AssertInLoopThread()
{
    if (!IsInLoopThread()) // 如果当前线程不是事件循环线程，输出错误并退出程序
    {
        NETWORK_ERROR << "It is forbidden to run loop on other thread!!!";
        exit(-1);
    }
}

bool EventLoop::IsInLoopThread() const
{
    return this == t_local_eventloop; // 判断当前线程是否是事件循环所属线程
}

// 在当前事件循环中执行任务（传入的是常量引用的函数）
void EventLoop::RunInLoop(const Func &f)
{
    if (IsInLoopThread()) // 如果当前线程就是事件循环的线程，直接执行任务
    {
        f();
    }
    else // 如果不是事件循环线程
    {
        std::lock_guard<std::mutex> lk(lock_); // 加锁保护任务队列
        functions_.push(f); // 将任务放入队列中
        WakeUp(); // 唤醒事件循环，确保任务能被执行
    }
}

// 在当前事件循环中执行任务（传入的是右值引用的函数，支持移动语义）
void EventLoop::RunInLoop(Func &&f)
{
    if (IsInLoopThread()) // 如果当前线程是事件循环线程，直接执行任务
    {
        f();
    }
    else 
    {
        std::lock_guard<std::mutex> lk(lock_); // 加锁保护任务队列
        functions_.push(std::move(f)); // 移动任务到任务队列
        WakeUp(); // 唤醒事件循环
    }
}
void EventLoop::RunFunctions()
{
    std::lock_guard<std::mutex> lk(lock_); // 加锁保护任务队列
    while (!functions_.empty()) // 当任务队列不为空时，依次执行队列中的任务
    {
        const Func &f = functions_.front();
        f();
        functions_.pop(); // 执行后弹出任务
    }
}
void EventLoop::WakeUp()
{
    if (!pipe_event_) // 如果 pipe_event_ 未初始化，创建一个用于唤醒事件循环的 PipeEvent
    {
        pipe_event_ = std::make_shared<PipeEvent>(this);
        AddEvent(pipe_event_); // 将 pipe_event_ 添加到事件循环中
    }
    int64_t tmp = 1;
    pipe_event_->Write((const char*)&tmp, sizeof(tmp)); // 写入数据，触发唤醒
}


// 在事件循环中插入延时任务
void EventLoop::InsertEntry(uint32_t delay, EntryPtr entryPtr)
{
    if (IsInLoopThread()) // 如果当前线程是事件循环线程，直接插入任务
    {
        wheel_.InsertEntry(delay, entryPtr);
    }
    else // 如果不是事件循环线程，异步提交任务
    {
        RunInLoop([this, delay, entryPtr] {
            wheel_.InsertEntry(delay, entryPtr); // 异步执行插入任务
        });
    }
}
// 设定延迟任务（常量引用版本）
void EventLoop::RunAfter(double delay, const Func &cb)
{
    if (IsInLoopThread()) // 如果是事件循环线程，直接执行
    {
        wheel_.RunAfter(delay, cb);
    }
    else // 否则异步执行任务
    {
        RunInLoop([this, delay, cb] {
            wheel_.RunAfter(delay, cb);
        });
    }
}

// 设定延迟任务（右值引用版本）
void EventLoop::RunAfter(double delay, Func &&cb)
{
    if (IsInLoopThread()) // 如果是事件循环线程，直接执行
    {
        wheel_.RunAfter(delay, cb);
    }
    else // 否则异步执行任务
    {
        RunInLoop([this, delay, cb] {
            wheel_.RunAfter(delay, cb);
        });
    }
}

// 设定周期性任务（常量引用版本）
void EventLoop::RunEvery(double interval, const Func &cb)
{
    if (IsInLoopThread()) // 如果是事件循环线程，直接执行
    {
        wheel_.RunEvery(interval, cb);
    }
    else // 否则异步执行任务
    {
        RunInLoop([this, interval, cb] {
            wheel_.RunEvery(interval, cb);
        });
    }
}

// 设定周期性任务（右值引用版本）
void EventLoop::RunEvery(double interval, Func &&cb)
{
    if (IsInLoopThread()) // 如果是事件循环线程，直接执行
    {
        wheel_.RunEvery(interval, cb);
    }
    else // 否则异步执行任务
    {
        RunInLoop([this, interval, cb] {
            wheel_.RunEvery(interval, cb);
        });
    }
}