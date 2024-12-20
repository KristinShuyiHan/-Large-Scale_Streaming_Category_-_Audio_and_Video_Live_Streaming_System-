#include "network/net/Acceptor.h"        // 包含 Acceptor 类的头文件，用于处理客户端连接请求
#include "network/net/EventLoop.h"       // 包含 EventLoop 类的头文件，用于事件循环
#include "network/net/EventLoopThread.h" // 包含 EventLoopThread 类的头文件，管理独立的事件循环线程

#include <iostream>                     // 标准输入输出流库，用于打印信息到控制台

using namespace tmms::network;          // 使用 tmms::network 命名空间，省略前缀

EventLoopThread eventloop_thread;       // 声明一个 EventLoopThread 对象，用于启动一个独立线程的事件循环
std::thread th;                         // 声明一个标准线程对象，用于未来可能的线程操作

int main(int argc, const char **argv)    // 主函数，程序入口点
{
    // 启动事件循环线程
    eventloop_thread.Run(); // 启动独立的事件循环线程，并创建一个 EventLoop 对象
    EventLoop *loop = eventloop_thread.Loop(); // 获取事件循环对象指针（EventLoop*）
    
    if(loop) // 如果成功获取到 EventLoop 对象，继续执行
    {
        // 创建服务器监听的地址，格式是 IP 地址:端口号
        InetAddress server("192.168.1.200:34444"); // 定义服务器地址：IP = 192.168.1.200，端口号 = 34444
        
        // 创建 Acceptor 对象，用于监听新的客户端连接请求
        std::shared_ptr<Acceptor> acceptor = std::make_shared<Acceptor>(loop, server); 
        // Acceptor 构造函数参数：
        // - loop: 事件循环对象，接收连接请求的事件将在此事件循环中处理
        // - server: 监听的 IP 和端口号

        // 设置 Acceptor 的连接回调函数（SetAcceptCallback）
        acceptor->SetAcceptCallback([](int fd, const InetAddress &addr) {
            // 这个 lambda 回调函数会在接收到新连接时被调用
            // 参数：
            // - fd: 新的客户端套接字文件描述符
            // - addr: 客户端的地址信息（IP 和端口号）
            std::cout << "host:" << addr.ToIpPort() << std::endl; 
            // 打印连接客户端的 IP 地址和端口号
        });

        // 启动 Acceptor 开始监听客户端连接请求
        acceptor->Start(); // 开始监听，等待客户端连接请求
        
        // 主线程进入一个无限循环，不断休眠，保持程序运行
        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1)); 
            // 每隔 1 秒休眠，避免主线程占用过多 CPU 资源
        }
    }    
    return 0; // 程序结束
}

// 总结：
// 这个程序是一个简单的服务器端示例，它通过 EventLoopThread 创建一个事件循环线程，使用 Acceptor 监听客户端连接。
// 当有新连接到达时，SetAcceptCallback 设置的回调函数会被调用，打印出连接客户端的 IP 地址和端口号。
// 主线程通过 sleep_for 保持程序运行，确保服务器持续监听。
// 比喻：

// EventLoop 就像服务员，不断监听和处理事件。
// Acceptor 是门卫，负责接收新客人（连接）。
// 主线程像是餐厅的后台，确保整个系统稳定运行，不会退出。