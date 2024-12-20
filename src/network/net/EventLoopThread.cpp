#include "EventLoopThread.h" // 包含 EventLoopThread 的头文件
// Include the header file that defines the EventLoopThread class.

using namespace tmms::network; // 使用 tmms::network 命名空间
// Use the namespace tmms::network to avoid repeating it.

/// 构造函数：创建一个线程，并指定线程的执行函数。
// Constructor: Create a thread and assign it a function to execute.
EventLoopThread::EventLoopThread()
:thread_([this](){StartEventLoop();}) // 使用 lambda 表达式绑定成员函数 StartEventLoop
{
    // thread_ 是一个线程对象，创建时传入一个 lambda 函数，该函数会调用 StartEventLoop 方法。
    // The thread_ object is initialized with a lambda function, which calls StartEventLoop().
}

/// 析构函数：确保线程和事件循环安全退出。
// Destructor: Ensures that the thread and event loop exit safely.
EventLoopThread::~EventLoopThread()
{
    Run(); // 确保线程开始运行，防止资源泄漏
    // Ensure that the thread starts running to avoid resource leaks.

    if(loop_) // 如果事件循环对象存在，则通知它退出
    {
        loop_->Quit(); // 调用 EventLoop 的 Quit 方法，停止事件循环。
        // Call the Quit method on the EventLoop to stop the event loop.
    }

    if(thread_.joinable()) // 检查线程是否可连接（还在运行状态）
    {
        thread_.join(); // 等待线程结束，确保资源安全回收。
        // Join the thread to wait for its completion and ensure safe resource cleanup.
    }
}

/// 启动线程：保证线程中的事件循环开始运行。
// Run function: Ensures that the event loop in the thread starts running.
void EventLoopThread::Run()
{
    std::call_once(once_,[this](){ // 确保代码块只执行一次
        // Use std::call_once to guarantee that the code block runs only once.

        {
            std::lock_guard<std::mutex> lk(lock_); // 加锁，防止多线程访问冲突
            running_ = true; // 设置运行状态为 true
            condition_.notify_all(); // 唤醒等待的线程
            // Notify all waiting threads that the condition has changed.
        }

        auto f = promise_loop_.get_future(); // 获取 promise 的 future 对象
        f.get(); // 等待事件循环开始执行
        // Wait for the event loop to start execution.
    });
}

/// 获取事件循环的指针。
// Return the pointer to the event loop.
EventLoop *EventLoopThread::Loop() const
{
    return loop_; // 返回事件循环指针
    // Return the pointer to the event loop object.
}

/// 线程执行的函数：初始化并启动事件循环。
// Function to initialize and start the event loop.
void EventLoopThread::StartEventLoop()
{
    EventLoop loop; // 创建 EventLoop 对象，表示事件循环的主体
    // Create an EventLoop object that represents the event loop.

    std::unique_lock<std::mutex> lk(lock_); // 加锁，保证线程安全
    condition_.wait(lk, [this](){return running_;}); 
    // 等待条件变量，当 running_ 变为 true 时继续执行
    // Wait for the condition variable until running_ is set to true.

    loop_ = &loop; // 将局部 EventLoop 对象的指针赋给成员变量 loop_
    // Assign the pointer of the local EventLoop object to the member variable loop_.

    promise_loop_.set_value(1); // 通知主线程，事件循环已启动
    // Notify the main thread that the event loop has started.

    loop.Loop(); // 开始事件循环，进入事件处理的循环
    // Start the event loop and enter the event handling loop.

    loop_ = nullptr; // 事件循环结束后，重置指针为 nullptr
    // Reset the loop pointer to nullptr after the event loop ends.
}

/// 返回线程对象的引用。
// Return a reference to the thread object.
std::thread &EventLoopThread::Thread() 
{
    return thread_; // 返回线程对象
    // Return the thread object.
}
