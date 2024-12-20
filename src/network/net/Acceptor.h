#pragma once  // 防止头文件被多次包含，避免重复定义。
// #pragma once ensures the file is included only once during compilation.

#include "network/base/InetAddress.h"   // 包含定义网络地址的头文件。
// Includes the header for managing internet addresses.

#include "network/base/SocketOpt.h"     // 包含定义套接字操作的头文件。
// Includes the header for socket operations.

#include "network/net/Event.h"          // 包含事件基类的头文件。
// Includes the base class for events.

#include "network/net/EventLoop.h"      // 包含事件循环类的头文件，用于管理事件。
// Includes the event loop class, which manages and handles events.

#include <functional>  // 包含 std::function，用于定义回调函数。
// Includes std::function, which allows defining function callbacks.

namespace tmms  // 命名空间，避免名字冲突。
// Namespace `tmms` to organize code and prevent naming conflicts.

{
    namespace network  // 二级命名空间，用于网络模块。
// Sub-namespace `network` for network-related functionalities.

    {
        // 定义一个回调函数类型，当新连接到来时调用。
        // AcceptCallback 是一个函数对象，接受两个参数：套接字描述符和客户端地址。
        using AcceptCallback = std::function<void(int sock, const InetAddress &addr)>;
        // AcceptCallback is a function type for handling new connections. 
        // It takes a socket descriptor (int sock) and client address (InetAddress).

        // Acceptor 类继承自 Event 类，用于监听新连接请求。
        // The Acceptor class inherits from Event and listens for incoming connection requests.
        class Acceptor : public Event
        {
        public:
            // 构造函数，接受一个事件循环指针和监听地址。
            // Constructor takes an EventLoop pointer and an address to listen on.
            Acceptor(EventLoop *loop, const InetAddress &addr);

            // 析构函数，清理资源。
            // Destructor for cleaning up resources.
            ~Acceptor();

            // 设置接收回调函数（传入左值引用）。
            // Sets the callback function for accepting connections (L-value reference).
            void SetAcceptCallback(const AcceptCallback &cb);

            // 设置接收回调函数（传入右值引用）。
            // Sets the callback function for accepting connections (R-value reference).
            void SetAcceptCallback(AcceptCallback &&cb);

            // 开始监听新连接。
            // Starts listening for new connections.
            void Start();

            // 停止监听。
            // Stops listening for connections.
            void Stop();

            // 读取事件处理函数，当有数据可读时触发。
            // OnRead is called when there is data to read (overrides base class).
            void OnRead() override;

            // 错误事件处理函数，当发生错误时触发。
            // Handles error events and outputs the error message (overrides base class).
            void OnError(const std::string &msg) override;

            // 关闭事件处理函数，当连接关闭时触发。
            // Handles the close event (overrides base class).
            void OnClose() override;

        private:
            // 打开监听套接字。
            // Opens the listening socket.
            void Open();

            InetAddress addr_;            // 监听地址，保存要监听的网络地址。
            // InetAddress to store the address to listen on.

            AcceptCallback accept_cb_;    // 接收连接的回调函数，用于处理新连接。
            // AcceptCallback to handle new incoming connections.

            SocketOpt *socket_opt_{nullptr};  // 套接字操作对象，管理底层套接字选项。
            // SocketOpt pointer for managing socket options (initialized as nullptr).
        };
    }
}

// Acceptor 是服务器网络编程中的一个关键组件，它负责监听客户端连接，当有新连接时通知用户（通过回调函数）。这样就把“监听”和“处理”的职责分开，便于代码的扩展和维护。

// 现实类比：

// Acceptor 就像一位门卫，负责接待来访的客人（新客户端连接）。
// 回调函数就像接待流程，门卫会把客人的信息（套接字和地址）交给服务员（业务逻辑）去处理。
// 这样一来，服务器程序可以更专注于业务逻辑，而不用直接处理底层的连接监听。






