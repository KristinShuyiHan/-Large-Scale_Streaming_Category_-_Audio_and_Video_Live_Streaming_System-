#include "Event.h"         // 引入 Event 类的头文件，用于声明 Event 类和相关成员函数。
#include "EventLoop.h"     // 引入 EventLoop 类的头文件，Event 类依赖 EventLoop 类进行事件管理。
#include <unistd.h>        // 引入系统头文件，提供 close() 系统调用，用于关闭文件描述符。
using namespace tmms::network; // 使用命名空间，避免每次使用类名前加上命名空间。

// 构造函数：初始化 Event 对象并关联到 EventLoop。
Event::Event(EventLoop *loop)
:loop_(loop) // 初始化成员变量 loop_，指向 EventLoop 实例。
{
    // 这是一个基本构造函数，接收 EventLoop 指针，并初始化它。
    // 示例：Event event(loop); 表示将当前事件绑定到指定的事件循环。
}

// 构造函数重载：初始化 Event 对象并设置文件描述符（fd）。
Event::Event(EventLoop *loop, int fd)
:loop_(loop), fd_(fd) // 初始化成员变量 loop_ 和 fd_，fd_ 是文件描述符。
{
    // 这个构造函数在初始化 Event 对象时，除了指定事件循环，还关联一个文件描述符（fd）。
    // 示例：Event event(loop, socket_fd); 表示事件与某个 socket 文件描述符相关联。
}

// 析构函数：释放 Event 对象资源。
Event::~Event()
{
    Close(); // 调用 Close 函数，确保释放文件描述符资源。
    // 当 Event 对象销毁时，自动关闭关联的文件描述符，防止资源泄露。
}

// 关闭文件描述符的函数。
void Event::Close()
{
    if (fd_ > 0) // 检查文件描述符是否有效（大于 0）。
    {
        ::close(fd_); // 调用系统的 close 函数，关闭文件描述符。
        fd_ = -1;     // 将 fd_ 设置为 -1，表示该文件描述符已关闭。
    }
    // 解释：为了防止重复关闭或非法操作，将文件描述符重置为 -1。
    // 示例：当一个网络连接关闭时，Event 对象会自动释放关联的 socket 文件描述符。
}

// 启用或禁用写事件监听的函数。
bool Event::EnableWriting(bool enable)
{
    // 调用 EventLoop 对象的 EnableEventWriting 方法，开启或关闭写事件监听。
    return loop_->EnableEventWriting(shared_from_this(), enable);
    // shared_from_this()：获取当前 Event 对象的 shared_ptr 指针，确保对象生命周期安全。
    // enable：布尔值，true 表示开启写事件监听，false 表示关闭。
    // 示例：event.EnableWriting(true); 表示监听当前事件对应 fd_ 的写操作。
}

// 启用或禁用读事件监听的函数。
bool Event::EnableReading(bool enable)
{
    // 调用 EventLoop 对象的 EnableEventReading 方法，开启或关闭读事件监听。
    return loop_->EnableEventReading(shared_from_this(), enable);
    // 类似于 EnableWriting，但作用于读事件。
    // 示例：event.EnableReading(true); 表示监听当前事件对应 fd_ 的读操作。
}

// 获取当前 Event 对象的文件描述符。
int Event::Fd() const
{
    return fd_; // 返回成员变量 fd_ 的值。
    // 解释：该函数用于获取当前事件关联的文件描述符。
    // 示例：int fd = event.Fd(); 获取事件关联的文件描述符，用于后续操作。
}
