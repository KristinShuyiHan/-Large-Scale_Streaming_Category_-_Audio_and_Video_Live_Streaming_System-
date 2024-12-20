#pragma once
// 防止头文件被重复包含，确保该文件只被编译一次。
// Prevent this header file from being included multiple times during compilation.

#include "network/net/TcpConnection.h"   // 包含TCP连接类的定义，用于管理网络连接。
// Include definition of TcpConnection class for managing network connections.

#include "network/net/EventLoop.h"       // 包含事件循环类的定义，用于处理事件循环。
// Include definition of EventLoop class for handling event loops.

#include "network/net/Acceptor.h"        // 包含连接接受器类，用于接收新连接。
// Include definition of Acceptor class for accepting new connections.

#include "network/base/InetAddress.h"    // 包含网络地址类的定义，表示IP和端口。
// Include definition of InetAddress class representing IP address and port.

#include <functional>    // 包含 std::function，用于回调函数。
// Include <functional> for std::function, used to store callback functions.

#include <memory>        // 包含智能指针 std::shared_ptr，用于资源管理。
// Include <memory> for std::shared_ptr, used to manage shared resources.

#include <unordered_set> // 包含无序集合，用于存储活跃的连接。
// Include <unordered_set> for storing active connections in an unordered collection.

namespace tmms
{
    namespace network
    {
        // 定义回调类型：用于处理新连接的回调函数。
        // Define a callback type for handling new connections.
        using NewConnectionCallback = std::function<void(const TcpConnectionPtr &)>;

        // 定义回调类型：用于处理连接销毁的回调函数。
        // Define a callback type for handling destroyed connections.
        using DestroyConnectionCallback = std::function<void(const TcpConnectionPtr&)>;

        // 定义 TcpServer 类：一个用于管理 TCP 服务器的类。
        // Define the TcpServer class, which manages the TCP server.
        class TcpServer
        {
        public:
            // 构造函数：初始化服务器。
            // Constructor: Initialize the TCP server.
            TcpServer(EventLoop *loop, const InetAddress &addr);

            // 虚析构函数：确保正确释放资源。
            // Virtual destructor: Ensures proper resource cleanup.
            virtual ~TcpServer();

            // 设置新连接回调函数（传左值引用）。
            // Set a callback function for new connections (lvalue reference).
            void SetNewConnectionCallback(const NewConnectionCallback &cb);

            // 设置新连接回调函数（传右值引用）。
            // Set a callback function for new connections (rvalue reference).
            void SetNewConnectionCallback(NewConnectionCallback &&cb);

            // 设置连接销毁回调函数（传左值引用）。
            // Set a callback function for destroyed connections (lvalue reference).
            void SetDestroyConnectionCallback(const DestroyConnectionCallback &cb);

            // 设置连接销毁回调函数（传右值引用）。
            // Set a callback function for destroyed connections (rvalue reference).
            void SetDestroyConnectionCallback(DestroyConnectionCallback &&cb);

            // 设置连接活动回调函数（传左值引用）。
            // Set a callback function for active connections (lvalue reference).
            void SetActiveCallback(const ActiveCallback &cb);

            // 设置连接活动回调函数（传右值引用）。
            // Set a callback function for active connections (rvalue reference).
            void SetActiveCallback(ActiveCallback &&cb);

            // 设置写完成回调函数（传左值引用）。
            // Set a callback function for write complete events (lvalue reference).
            void SetWriteCompleteCallback(const WriteCompleteCallback &cb);

            // 设置写完成回调函数（传右值引用）。
            // Set a callback function for write complete events (rvalue reference).
            void SetWriteCompleteCallback(WriteCompleteCallback &&cb);

            // 设置消息回调函数（传左值引用）。
            // Set a callback function for incoming messages (lvalue reference).
            void SetMessageCallback(const MessageCallback &cb);

            // 设置消息回调函数（传右值引用）。
            // Set a callback function for incoming messages (rvalue reference).
            void SetMessageCallback(MessageCallback &&cb);

            // 当有新连接到来时被调用。
            // Called when a new connection is accepted.
            void OnAccet(int fd, const InetAddress &addr);

            // 当连接关闭时被调用。
            // Called when a connection is closed.
            void OnConnectionClose(const TcpConnectionPtr &con);

            // 启动服务器：进入事件循环，开始接受新连接。
            // Start the server: Enter event loop and begin accepting new connections.
            virtual void Start();

            // 停止服务器：退出事件循环，清理资源。
            // Stop the server: Exit the event loop and clean up resources.
            virtual void Stop();

        private:
            EventLoop * loop_{nullptr};   // 指向事件循环对象的指针，用于处理事件。
            // Pointer to the EventLoop object for handling events.

            InetAddress addr_;            // 服务器的监听地址（IP 和端口）。
            // Server listening address (IP and port).

            std::shared_ptr<Acceptor> acceptor_; // 接受新连接的接受器对象。
            // Shared pointer to the Acceptor object for accepting new connections.

            NewConnectionCallback new_connection_cb_; // 新连接的回调函数。
            // Callback function for new connections.

            std::unordered_set<TcpConnectionPtr> connections_; // 存储当前活跃的连接。
            // Store active connections using an unordered set.

            MessageCallback message_cb_;     // 处理消息的回调函数。
            // Callback function for handling incoming messages.

            ActiveCallback active_cb_;       // 处理活动连接的回调函数。
            // Callback function for handling active connections.

            WriteCompleteCallback write_complete_cb_; // 处理写完成事件的回调函数。
            // Callback function for handling write complete events.

            DestroyConnectionCallback destroy_connection_cb_; // 处理连接销毁的回调函数。
            // Callback function for handling destroyed connections.
        };
    }
}
