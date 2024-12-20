#pragma once  
// 防止头文件被重复包含，保证编译时只加载一次
// Prevents the header file from being included multiple times during compilation.

#include "Connection.h"  // 包含基础连接类的定义
#include "network/base/InetAddress.h"  // 包含网络地址类的定义
#include "network/base/MsgBuffer.h"    // 包含消息缓冲区类的定义
#include <functional>   // 用于 std::function（回调函数）
#include <memory>       // 用于智能指针 std::shared_ptr 和 std::weak_ptr
#include <list>         // 用于 std::list 容器
#include <sys/uio.h>    // 用于 I/O 向量操作

namespace tmms
{
    namespace network
    {
        // 前向声明 TcpConnection 类  
        // Forward declaration of the TcpConnection class.
        class TcpConnection;

        // 定义智能指针类型，便于管理 TcpConnection 的生命周期  
        // Define a shared_ptr for TcpConnection to manage its lifecycle.
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;  

        // 定义各种回调函数类型，便于事件处理  
        // Define callback function types for handling various events.
        using CloseConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
        using MessageCallback = std::function<void(const TcpConnectionPtr &, MsgBuffer &buffer)>;
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
        using TimeoutCallback = std::function<void(const TcpConnectionPtr &)>;

        // 定义一个超时条目结构体，用于管理连接的超时  
        // TimeoutEntry is a structure for managing connection timeouts.
        struct TimeoutEntry;

        // TcpConnection 类继承自 Connection，表示 TCP 连接  
        // TcpConnection class inherits from Connection and represents a TCP connection.
        class TcpConnection : public Connection
        {
        public:
            // 构造函数：初始化 TCP 连接  
            // Constructor: Initializes the TCP connection.
            TcpConnection(EventLoop *loop, 
                          int socketfd, 
                          const InetAddress &localAddr, 
                          const InetAddress &peerAddr);

            // 析构函数：清理资源  
            // Destructor: Cleans up resources.
            virtual ~TcpConnection();

            // 设置关闭连接时的回调函数（重载）
            // Set the callback function when the connection is closed.
            void SetCloseCallback(const CloseConnectionCallback &cb);
            void SetCloseCallback(CloseConnectionCallback &&cb);

            // 设置接收消息的回调函数（重载）
            // Set the callback function when a message is received.
            void SetRecvMsgCallback(const MessageCallback &cb);
            void SetRecvMsgCallback(MessageCallback &&cb);

            // 设置写操作完成时的回调函数（重载）
            // Set the callback function when writing is complete.
            void SetWriteCompleteCallback(const WriteCompleteCallback &cb);
            void SetWriteCompleteCallback(WriteCompleteCallback &&cb);

            // 设置超时回调函数和超时时间  
            // Set the timeout callback and timeout duration.
            void SetTimeoutCallback(int timeout, const TimeoutCallback &cb);
            void SetTimeoutCallback(int timeout, TimeoutCallback &&cb);

            // 重写基类的关闭连接方法  
            // Override the base class method to handle connection closure.
            void OnClose() override;

            // 强制关闭连接  
            // Forcefully close the connection.
            void ForceClose() override;

            // 处理读事件  
            // Handle read events (e.g., data arriving from the network).
            void OnRead() override;

            // 处理错误事件  
            // Handle errors (e.g., logging an error message).
            void OnError(const std::string &msg) override;

            // 处理写事件  
            // Handle write events (e.g., data sent to the network).
            void OnWrite() override;

            // 发送数据：支持两种形式  
            // Send data with two overloads: using a list or a buffer.
            void Send(std::list<BufferNodePtr>& list);
            void Send(const char *buf, size_t size);

            // 处理超时事件  
            // Handle timeout events.
            void OnTimeout();

            // 启用空闲超时检测  
            // Enable idle timeout checks with a maximum idle time.
            void EnableCheckIdleTimeout(int32_t max_time);

        private:
            // 内部发送数据函数  
            // Internal functions to send data in the loop.
            void SendInLoop(const char *buf, size_t size);
            void SendInLoop(std::list<BufferNodePtr>& list);

            // 扩展连接的生命周期  
            // Extend the life of the connection (e.g., reset timeout).
            void ExtendLife();

            // 成员变量  
            // Member variables
            bool closed_{false};   // 标记连接是否关闭 Flag indicating if the connection is closed.
            CloseConnectionCallback close_cb_;  // 关闭回调 Close callback function.
            MsgBuffer message_buffer_;  // 消息缓冲区 Message buffer.
            MessageCallback message_cb_; // 消息回调 Message callback.
            std::vector<struct iovec> io_vec_list_;  // I/O 向量列表 I/O vector list for batch data transfer.
            WriteCompleteCallback write_complete_cb_; // 写完成回调 Write complete callback.
            std::weak_ptr<TimeoutEntry> timeout_entry_; // 弱指针管理超时条目 Weak pointer to manage timeout entries.
            int32_t max_idle_time_{30}; // 最大空闲时间，单位秒 Maximum idle time in seconds (default 30).
        };

        // TimeoutEntry 结构体：用于管理超时的连接条目  
        // TimeoutEntry structure: manages timed-out connection entries.
        struct TimeoutEntry
        {
            // 构造函数：保存连接指针  
            // Constructor: Save the connection pointer.
            TimeoutEntry(const TcpConnectionPtr &c)
                : conn(c)
            {
            }

            // 析构函数：触发超时回调  
            // Destructor: Trigger the timeout callback.
            ~TimeoutEntry()
            {
                auto c = conn.lock(); // 锁定弱指针为共享指针 Lock the weak pointer to a shared pointer.
                if (c)
                {
                    c->OnTimeout(); // 执行超时逻辑 Trigger the timeout logic.
                }
            }

            std::weak_ptr<TcpConnection> conn; // 弱指针指向 TCP 连接 Weak pointer pointing to a TCP connection.
        };

    } // namespace network
} // namespace tmms
