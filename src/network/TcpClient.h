#pragma once  
// 防止头文件被重复包含，保证编译过程中只会加载一次。
// Prevents this header file from being included multiple times during compilation.

#include "network/base/InetAddress.h"  // 引入网络地址类，用于表示服务器的地址。
// Includes the InetAddress class, which is used to represent the server's address.

#include "network/net/TcpConnection.h"  // 引入 TcpConnection 类，提供 TCP 连接的基本功能。
// Includes TcpConnection, a class that provides basic functionalities for TCP connections.

#include "network/net/EventLoop.h"  // 引入事件循环类，用于处理网络事件。
// Includes EventLoop class, which handles network events like reading, writing, and connections.

#include <functional>  // 引入 std::function，用于定义回调函数类型。
// Includes <functional> for using `std::function` to define callback functions.

namespace tmms // 定义命名空间 tmms，避免与其他代码冲突。
// Defines the `tmms` namespace to avoid name conflicts with other parts of the codebase.

{
    namespace network // 定义子命名空间 network，表示网络相关代码。
// Defines the `network` sub-namespace, indicating that this code is related to networking.

    {
        enum 
        {
            kTcpConStatusInit = 0,       // TCP 连接初始状态，尚未开始连接。
            // Initial state of the TCP connection; the connection process has not started.

            kTcpConStatusConnecting = 1, // TCP 正在尝试连接到服务器。
            // The TCP connection is currently in the process of connecting to the server.

            kTcpConStatusConnected = 2,  // TCP 连接已成功建立。
            // The TCP connection has been successfully established.

            kTcpConStatusDisConnected = 3, // TCP 连接已断开。
            // The TCP connection has been disconnected.
        };

        using ConnectionCallback = std::function<void (const TcpConnectionPtr &con,bool)>;  
        // 定义 ConnectionCallback 类型，表示当连接状态变化时要执行的回调函数。
        // This defines `ConnectionCallback`, a callback function type triggered when the connection status changes.
        // It takes a `TcpConnectionPtr` (connection object) and a `bool` indicating success or failure.

        class TcpClient : public TcpConnection  
        // 定义 TcpClient 类，继承自 TcpConnection，表示一个客户端的 TCP 连接。
        // Defines the TcpClient class, inheriting from TcpConnection, representing a TCP client connection.

        {
        public:
            TcpClient(EventLoop *loop, const InetAddress &server);  
            // 构造函数，初始化事件循环和服务器地址。
            // Constructor: initializes the event loop and the server address.

            virtual ~TcpClient();  
            // 析构函数，释放资源。
            // Destructor: releases resources when TcpClient is destroyed.

            void SetConnectCallback(const ConnectionCallback &cb);  
            // 设置连接回调函数（左值引用版本），用于监听连接成功或失败。
            // Sets the connection callback function (lvalue reference), used to handle connection success or failure.

            void SetConnectCallback(ConnectionCallback &&cb);  
            // 设置连接回调函数（右值引用版本），提高效率。
            // Sets the connection callback function (rvalue reference), improving efficiency by moving the callback.

            void Connect();  
            // 启动连接到服务器的操作。
            // Initiates the process to connect to the server.

            void OnRead() override;  
            // 重写 TcpConnection 类的 OnRead 方法，处理读取事件。
            // Overrides the `OnRead` method of TcpConnection, to handle read events.

            void OnWrite() override;  
            // 重写 TcpConnection 类的 OnWrite 方法，处理写入事件。
            // Overrides the `OnWrite` method of TcpConnection, to handle write events.

            void OnClose() override;  
            // 重写 TcpConnection 类的 OnClose 方法，处理连接关闭事件。
            // Overrides the `OnClose` method of TcpConnection, to handle connection closure events.

            void Send(std::list<BufferNodePtr>&list);  
            // 发送数据，参数为一组 BufferNodePtr（数据缓存列表）。
            // Sends data using a list of `BufferNodePtr`, which is a list of buffered data nodes.

            void Send(const char *buf, size_t size);  
            // 发送数据，参数为指向字符的指针和数据大小。
            // Sends data using a character pointer and the size of the data.

        private:
            void ConnectInLoop();  
            // 在事件循环中执行连接操作，确保线程安全。
            // Performs the connection operation within the event loop, ensuring thread safety.

            void UpdateConnectionStatus();  
            // 更新当前的连接状态，例如设置为已连接或断开。
            // Updates the current connection status, such as marking it as connected or disconnected.

            bool CheckError();  
            // 检查连接过程中是否出现错误，返回错误状态。
            // Checks if any errors occurred during the connection process and returns the error status.

            InetAddress server_addr_;  
            // 保存服务器的地址信息。
            // Stores the server address information.

            int32_t status_{kTcpConStatusInit};  
            // 当前 TCP 连接的状态，默认为初始状态。
            // Holds the current TCP connection status, initialized to `kTcpConStatusInit`.

            ConnectionCallback connected_cb_;  
            // 存储用户设置的连接回调函数。
            // Stores the user-defined connection callback function.
        };
    }
}
