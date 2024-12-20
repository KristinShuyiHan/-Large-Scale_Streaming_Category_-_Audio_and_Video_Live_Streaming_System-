#pragma once
// 确保头文件只会被编译一次，避免重复包含
// Ensures the header file is included only once to prevent duplication.

#include "network/base/InetAddress.h" // 引入用于存储网络地址的类
#include "Event.h"                    // 引入事件基类
#include "EventLoop.h"                // 引入事件循环类
#include <functional>                 // 引入 std::function 处理回调函数
#include <unordered_map>              // 引入哈希表，用于存储上下文
#include <memory>                     // 引入智能指针 std::shared_ptr
#include <atomic>                     // 引入原子操作类，用于线程安全

namespace tmms // 定义命名空间 tmms
{
    namespace network // 在 tmms 命名空间中定义 network 子命名空间
    {
        // 定义一些上下文类型的枚举值，用于标识不同类型的连接上下文
        // Enum values used to identify different types of connection contexts.
        enum 
        {
            kNormalContext = 0,  // 普通上下文
            kRtmpContext,        // RTMP 协议上下文
            kHttpContext,        // HTTP 协议上下文
            kUserContext,        // 用户自定义上下文
            kFlvContext,         // FLV 协议上下文
        };

        // 定义一个结构体 BufferNode，用于存储缓冲区数据的地址和大小
        // A structure to hold buffer address and its size.
        struct BufferNode
        {
            BufferNode(void *buf,size_t s)
            :addr(buf),size(s)
            {} 
            // 构造函数，初始化地址和大小
            // Constructor to initialize buffer address and size.

            void *addr{nullptr}; // 缓冲区地址 (Buffer address)
            size_t size{0};      // 缓冲区大小 (Buffer size)
        };

        using BufferNodePtr = std::shared_ptr<BufferNode>; // 定义智能指针类型，管理 BufferNode
        using ContextPtr = std::shared_ptr<void>;          // 通用上下文类型，使用 shared_ptr 管理
        class Connection;                                  // 前向声明 Connection 类
        using ConnectionPtr = std::shared_ptr<Connection>; // Connection 类的智能指针类型
        using ActiveCallback = std::function<void(const ConnectionPtr&)>; // 定义回调函数类型

        // Connection 类继承自 Event 类，表示一个网络连接
        // The Connection class represents a network connection and inherits from Event.
        class Connection:public Event
        {
        public:
            // 构造函数，初始化连接对象
            // Constructor to initialize the connection object.
            Connection(EventLoop *loop, 
                        int fd, 
                        const InetAddress &localAddr, 
                        const InetAddress &peerAddr);

            virtual ~Connection() = default; // 虚析构函数，确保派生类能正确析构

            // 设置本地地址
            // Set the local address.
            void SetLocalAddr(const InetAddress &local);

            // 设置对端地址
            // Set the peer address.
            void SetPeerAddr(const InetAddress &peer);

            // 获取本地地址
            // Get the local address.
            const InetAddress &LocalAddr() const;

            // 获取对端地址
            // Get the peer address.
            const InetAddress &PeerAddr() const;

            // 设置指定类型的上下文，左值引用版本
            // Set context for a specific type using lvalue reference.
            void SetContext(int type,const std::shared_ptr<void> &context);

            // 设置指定类型的上下文，右值引用版本（移动语义）
            // Set context for a specific type using rvalue reference.
            void SetContext(int type,std::shared_ptr<void> &&context);

            // 获取指定类型的上下文，使用模板函数，支持类型转换
            // Template function to retrieve context for a given type.
            template <typename T> 
            std::shared_ptr<T> GetContext(int type) const
            {
                auto iter = contexts_.find(type); // 查找指定类型的上下文
                if(iter!=contexts_.end()) // 如果找到了
                {
                    return std::static_pointer_cast<T>(iter->second); // 类型转换并返回
                }
                return std::shared_ptr<T>(); // 返回空指针
            }

            // 清除指定类型的上下文
            // Clear context for a specific type.
            void ClearContext(int type);

            // 清除所有上下文
            // Clear all contexts.
            void ClearContext();

            // 设置激活回调函数，左值引用版本
            // Set the active callback using lvalue reference.
            void SetActiveCallback(const ActiveCallback &cb);

            // 设置激活回调函数，右值引用版本（移动语义）
            // Set the active callback using rvalue reference.
            void SetActiveCallback(ActiveCallback &&cb);

            // 激活当前连接，执行回调函数
            // Activate the connection and invoke the callback function.
            void Active();

            // 将当前连接设为非激活状态
            // Deactivate the connection.
            void Deactive();

            // 强制关闭连接，纯虚函数，需要派生类实现
            // Force close the connection. A pure virtual function to be implemented by derived classes.
            virtual void ForceClose() = 0;

        private:
            std::unordered_map<int,ContextPtr> contexts_; // 存储上下文的哈希表
            ActiveCallback active_cb_; // 激活时调用的回调函数
            std::atomic<bool> active_{false}; // 原子布尔变量，表示连接是否处于激活状态

        protected:
            InetAddress local_addr_; // 本地地址
            InetAddress peer_addr_;  // 对端地址
        };
    }
}


// 举例：网络聊天服务器中的连接管理
// 假设一个聊天室服务，每个用户连接都会创建一个 Connection 对象：

// 本地地址：服务器监听的 IP 和端口。
// 对端地址：用户的 IP 和端口。
// 上下文数据：可以存储用户信息、消息历史等。
// 回调函数：当连接激活时，向用户发送“欢迎加入聊天室”的消息。
// 激活和去激活：用户上线时激活连接，下线时去激活连接。
// 代码实现：

// cpp
// Copy code
// Connection conn(loop, fd, serverAddr, clientAddr);
// conn.SetActiveCallback([](const std::shared_ptr<Connection>& conn) {
//     std::cout << "用户连接成功: " << conn->PeerAddr().ToString() << std::endl;
// });
// conn.Active();  // 激活连接
// 这样，Connection 类确保了对网络连接的高效管理和操作，同时使用智能指针和回调机制确保了内存安全和可扩展性。



// 总结（中文 + 英文）
// 类功能：
// Connection 类用于管理单个网络连接，提供设置/获取地址、管理上下文、设置回调函数，以及激活/去激活连接的功能。
// The Connection class manages a single network connection, offering methods to set/get addresses, manage contexts, configure callback functions, and activate/deactivate connections.

// 重要成员变量：

// local_addr_ 和 peer_addr_：存储本地地址和对端地址。
// Stores local and peer addresses.
// contexts_：存储任意类型的上下文数据（通过类型作为键）。
// A map to store arbitrary context data by type.
// active_cb_：当连接激活时调用的回调函数。
// A callback function triggered when the connection becomes active.
// active_：使用 atomic 类型，标记连接是否激活，确保多线程安全。
// An atomic variable marking the connection's active status for thread safety.
// 激活流程（Active 方法）：

// 如果连接未激活，调用 loop_->RunInLoop 异步执行激活逻辑。
// If the connection is inactive, it uses loop_->RunInLoop to execute the activation logic asynchronously.
// 执行回调函数 active_cb_，并将当前连接对象作为参数传入。
// Executes the callback active_cb_ with the current connection object as a parameter.
// 去激活（Deactive 方法）：
// 直接将 active_ 设置为 false，表示连接不再活跃。
// Directly sets active_ to false, indicating the connection is no longer active.
