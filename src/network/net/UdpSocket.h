
#pragma once  
// 防止头文件被重复包含，确保编译时只包含一次。
// Prevents the header file from being included multiple times during compilation.

#include "network/base/InetAddress.h"  // 引入 InetAddress 类，表示网络地址。
// Includes InetAddress class, which represents a network address.

#include "network/base/MsgBuffer.h"  // 引入 MsgBuffer 类，管理消息缓冲区。
// Includes MsgBuffer class for managing message buffers.

#include "network/net/EventLoop.h"  // 引入 EventLoop 类，处理事件循环。
// Includes EventLoop class, which manages event loops for handling I/O events.

#include "network/net/Connection.h"  // 引入 Connection 类，表示连接的基础功能。
// Includes Connection class that provides base functionalities for connections.

#include <list>           // 引入 std::list，用于存储数据缓冲区。
// Includes <list> for storing buffer nodes in a list.

#include <functional>     // 引入 std::function，用于定义回调函数。
// Includes <functional> for defining callback functions.

#include <memory>         // 引入智能指针 std::shared_ptr 和 std::weak_ptr。
// Includes <memory> for smart pointers like `std::shared_ptr` and `std::weak_ptr`.

namespace tmms  // 定义顶层命名空间 tmms，避免与其他代码冲突。
// Defines the top-level namespace `tmms` to avoid conflicts with other code.

{
    namespace network  // 定义子命名空间 network，表示网络相关代码。
// Defines the `network` namespace for networking-related code.

    {
        class UdpSocket;  // 前向声明 UdpSocket 类。
// Forward declaration of the `UdpSocket` class.

        using UdpSocketPtr = std::shared_ptr<UdpSocket>;  
        // 定义 UdpSocketPtr，表示指向 UdpSocket 的智能指针。
        // Defines `UdpSocketPtr` as a smart pointer to `UdpSocket`.

        using UdpSocketMessageCallback = std::function<void(const UdpSocketPtr &, const InetAddress &addr, MsgBuffer &buf)>;  
        // 定义消息接收回调函数类型：当接收到消息时触发。
        // Defines a callback type that is triggered when a message is received.

        using UdpSocketWriteCompleteCallback = std::function<void(const UdpSocketPtr &)>;  
        // 定义写完成回调函数类型：当数据发送完成时触发。
        // Callback type triggered when a write operation completes.

        using UdpSocketCloseConnectionCallback = std::function<void(const UdpSocketPtr &)>;  
        // 定义关闭连接的回调函数类型：当连接关闭时触发。
        // Callback type triggered when the connection is closed.

        using UdpSocketTimeoutCallback = std::function<void(const UdpSocketPtr &)>;  
        // 定义超时回调函数类型：当连接空闲超时时触发。
        // Callback type triggered when the connection times out.

        struct UdpTimeoutEntry;  // 前向声明 UdpTimeoutEntry 结构体。
// Forward declaration of `UdpTimeoutEntry` struct.

        struct UdpBufferNode : public BufferNode  
        // 定义 UdpBufferNode 结构体，继承自 BufferNode，用于存储 UDP 消息。
        // Defines `UdpBufferNode`, inheriting from `BufferNode`, used for storing UDP messages.

        {
            UdpBufferNode(void *buf, size_t s, struct sockaddr* saddr, socklen_t len)
            :BufferNode(buf, s), sock_addr(saddr), sock_len(len)
            {}
            // 构造函数，初始化数据指针、大小和地址信息。
            // Constructor to initialize the buffer, size, and address information.

            struct sockaddr *sock_addr{nullptr};  // 存储目标地址。
// Stores the destination address.

            socklen_t sock_len{0};  // 地址长度。
// Length of the address.
        };

        using UdpBufferNodePtr = std::shared_ptr<UdpBufferNode>;  
        // 定义 UdpBufferNodePtr，表示指向 UdpBufferNode 的智能指针。
        // Smart pointer to a `UdpBufferNode`.

        class UdpSocket : public Connection  
        // 定义 UdpSocket 类，继承自 Connection，表示 UDP 套接字连接。
// Defines the `UdpSocket` class inheriting from `Connection`, representing a UDP socket connection.

        {
        public:
            UdpSocket(EventLoop *loop, int socketfd, const InetAddress &localAddr, const InetAddress &peerAddr);
            // 构造函数：初始化事件循环、套接字描述符和地址信息。
            // Constructor initializes the event loop, socket descriptor, and addresses.

            ~UdpSocket();  
            // 析构函数：释放资源。
            // Destructor: releases resources.

            void SetCloseCallback(const UdpSocketCloseConnectionCallback &cb);  
            void SetCloseCallback(UdpSocketCloseConnectionCallback &&cb);  
            // 设置关闭连接的回调函数。
            // Sets the callback for connection closure.

            void SetRecvMsgCallback(const UdpSocketMessageCallback &cb);  
            void SetRecvMsgCallback(UdpSocketMessageCallback &&cb);  
            // 设置消息接收的回调函数。
            // Sets the callback for receiving messages.

            void SetWriteCompleteCallback(const UdpSocketWriteCompleteCallback &cb);  
            void SetWriteCompleteCallback(UdpSocketWriteCompleteCallback &&cb);  
            // 设置写完成的回调函数。
            // Sets the callback for write completion.

            void SetTimeoutCallback(int timeout, const UdpSocketTimeoutCallback &cb);  
            void SetTimeoutCallback(int timeout, UdpSocketTimeoutCallback &&cb);  
            // 设置超时回调函数及超时时间。
            // Sets the timeout callback and the timeout duration.

            void OnTimeOut();  
            // 处理超时事件。
            // Handles timeout events.

            void OnError(const std::string &msg) override;  
            // 处理错误事件。
            // Handles error events.

            void OnRead() override;  
            // 处理数据读取事件。
            // Handles data read events.

            void OnWrite() override;  
            // 处理数据写入事件。
            // Handles data write events.

            void OnClose() override;  
            // 处理连接关闭事件。
            // Handles connection closure events.

            void EnableCheckIdleTimeout(int32_t max_time);  
            // 启用空闲超时检测。
            // Enables idle timeout checking.

            void Send(std::list<UdpBufferNodePtr>&list);  
            // 发送一组缓冲区数据。
            // Sends a list of buffer nodes.

            void Send(const char *buf, size_t size, struct sockaddr * addr, socklen_t len);  
            // 发送单个缓冲区数据。
            // Sends a single buffer to a specified address.

            void ForceClose() override;  
            // 强制关闭连接。
            // Forcibly closes the connection.

        private:
            void ExtendLife();  
            // 延长连接的生命周期。
            // Extends the life of the connection.

            void SendInLoop(std::list<UdpBufferNodePtr>&list);  
            void SendInLoop(const char *buf, size_t size, struct sockaddr*saddr, socklen_t len);  
            // 在事件循环中发送数据。
            // Sends data in the event loop.

            std::list<UdpBufferNodePtr> buffer_list_;  // 数据缓冲区列表。
// List of data buffers.

            bool closed_{false};  // 连接关闭标志。
// Flag to indicate whether the connection is closed.

            int32_t max_idle_time_{30};  // 最大空闲时间。
// Maximum idle time.

            std::weak_ptr<UdpTimeoutEntry> timeout_entry_;  // 弱指针，关联超时事件。
// Weak pointer to manage timeout entries.

            MsgBuffer message_buffer_;  // 消息缓冲区。
// Message buffer.

            UdpSocketMessageCallback message_cb_;  // 消息接收回调。
// Callback for receiving messages.

            UdpSocketWriteCompleteCallback write_complete_cb_;  // 写完成回调。
// Callback for write completion.

            UdpSocketCloseConnectionCallback close_cb_;  // 连接关闭回调。
// Callback for connection closure.
        };

        struct UdpTimeoutEntry  
        // 定义 UdpTimeoutEntry 结构体，用于超时管理。
// Defines `UdpTimeoutEntry` for timeout management.

        {
            UdpTimeoutEntry(const UdpSocketPtr &c) : conn(c) {}
            ~UdpTimeoutEntry()
            {
                auto c = conn.lock();
                if(c)
                {
                    c->OnTimeOut();
                }
            }
            std::weak_ptr<UdpSocket> conn;  
            // 弱指针，关联一个 UdpSocket 实例。
            // Weak pointer to a `UdpSocket` instance.
        };        
    }
}
