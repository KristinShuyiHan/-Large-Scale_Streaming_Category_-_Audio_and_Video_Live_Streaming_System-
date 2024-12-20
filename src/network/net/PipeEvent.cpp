#include "PipeEvent.h"    // 包含 PipeEvent 类的头文件，定义类及其方法
#include "network/base/Network.h" // 包含网络基础设施相关的头文件
#include <unistd.h>       // 提供 POSIX 系统调用，如 pipe 和 close
#include <fcntl.h>        // 提供文件控制功能，如 O_NONBLOCK
#include <iostream>       // 用于输出信息
#include <errno.h>        // 提供错误码定义和获取

using namespace tmms::network;  // 使用 tmms 网络命名空间

// 构造函数：初始化 PipeEvent 并创建非阻塞管道
PipeEvent::PipeEvent(EventLoop *loop)  
:Event(loop)  // 调用基类 Event 的构造函数，并将事件绑定到事件循环
{
    int fd[2] = {0,};  // 创建一个存储管道文件描述符的数组，fd[0] 为读端，fd[1] 为写端
    auto ret = ::pipe2(fd, O_NONBLOCK);  // 调用 pipe2 创建一个非阻塞的管道
    if(ret < 0)  // 如果返回值小于 0，表示创建管道失败
    {
        NETWORK_ERROR << "pipe open failed."; // 输出错误信息
        exit(-1);  // 退出程序，表示致命错误
    }
    fd_ = fd[0];      // 管道的读端文件描述符
    write_fd_ = fd[1]; // 管道的写端文件描述符
}

// 析构函数：关闭管道的写端
PipeEvent::~PipeEvent()
{
    if(write_fd_ > 0)  // 检查写端文件描述符是否有效
    {
        ::close(write_fd_);  // 关闭写端文件描述符
        write_fd_ = -1;      // 将写端置为无效
    }
}

// **OnRead 方法**：当有数据可读时调用
void PipeEvent::OnRead()
{
    int64_t tmp = 0;  // 临时变量，用于存储读取的数据
    auto ret = ::read(fd_, &tmp, sizeof(tmp));  // 从管道的读端读取数据
    if(ret < 0)  // 如果读取失败
    {
        NETWORK_ERROR << "pipe read error. error:" << errno;  // 输出错误码
        return;  // 返回，不继续处理
    }
    // 可选：输出读取到的数据（此处注释掉）
    /// std::cout << "pipe read tmp:" << tmp << std::endl;
}

// **OnClose 方法**：关闭管道写端
void PipeEvent::OnClose()
{
    if(write_fd_ > 0)  // 检查写端文件描述符是否有效
    {
        ::close(write_fd_);  // 关闭写端文件描述符
        write_fd_ = -1;      // 将写端置为无效
    }
}

// **OnError 方法**：当发生错误时调用
void PipeEvent::OnError(const std::string &msg)
{
    std::cout << "error:" << msg << std::endl;  // 输出错误信息
}

// **Write 方法**：向管道的写端写入数据
void PipeEvent::Write(const char* data, size_t len)
{
    ::write(write_fd_, data, len);  // 将指定的数据写入管道的写端
}
