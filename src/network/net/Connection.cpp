#include "Connection.h" // 包含 Connection 类的头文件
// Include header file for the Connection class

using namespace tmms::network; // 使用 tmms::network 命名空间
// Use the namespace "tmms::network"

/// Connection 类的构造函数 (Constructor)
Connection::Connection(EventLoop *loop, 
            int fd, 
            const InetAddress &localAddr, 
            const InetAddress &peerAddr)
:Event(loop,fd),local_addr_(localAddr),peer_addr_(peerAddr)
{
    // 初始化 Connection 类对象，将事件循环（EventLoop）、文件描述符（fd）
    // 本地地址（localAddr）和远端地址（peerAddr）传入。
    // Initialize the Connection object with:
    // - EventLoop pointer: loop (for event handling)
    // - File descriptor: fd (a unique ID for the connection)
    // - Local address: localAddr
    // - Peer address: peerAddr
}

/// 设置本地地址 Set Local Address
void Connection::SetLocalAddr(const InetAddress &local)
{
    local_addr_ = local; // 更新本地地址对象
    // Update the local address with the given value
}

/// 设置远端地址 Set Peer Address
void Connection::SetPeerAddr(const InetAddress &peer)
{
    peer_addr_ = peer; // 更新远端地址对象
    // Update the peer address with the given value
}

/// 获取本地地址 Get Local Address
const InetAddress &Connection::LocalAddr() const
{
    return local_addr_; // 返回本地地址对象的引用
    // Return a constant reference to the local address
}

/// 获取远端地址 Get Peer Address
const InetAddress &Connection::PeerAddr() const
{
    return peer_addr_; // 返回远端地址对象的引用
    // Return a constant reference to the peer address
}

/// 设置上下文信息（带共享指针） Set Context with shared_ptr (Copy)
void Connection::SetContext(int type,const std::shared_ptr<void> &context)
{
    contexts_[type] = context; // 将上下文信息存储在 map 容器中
    // Store the shared context pointer into a map, indexed by type
}

/// 设置上下文信息（使用移动语义） Set Context with shared_ptr (Move)
void Connection::SetContext(int type,std::shared_ptr<void> &&context)
{
    contexts_[type] = std::move(context); // 使用移动语义存储上下文，减少拷贝开销
    // Move the context pointer into the map for efficiency
}

/// 清除特定类型的上下文信息 Clear Context by type
void Connection::ClearContext(int type)
{
    contexts_[type].reset(); // 将指定类型的上下文指针置空
    // Reset (clear) the context pointer for the specified type
}

/// 清除所有上下文信息 Clear All Context
void Connection::ClearContext()
{
    contexts_.clear(); // 清空整个上下文 map
    // Clear the entire map of contexts
}

/// 设置活动回调函数（拷贝版本） Set Active Callback (Copy)
void Connection::SetActiveCallback(const ActiveCallback &cb)
{
    active_cb_ = cb; // 存储回调函数
    // Store the callback function for active state
}

/// 设置活动回调函数（移动版本） Set Active Callback (Move)
void Connection::SetActiveCallback(ActiveCallback &&cb)
{
    active_cb_ = std::move(cb); // 使用移动语义存储回调函数
    // Move the callback function for active state
}

/// 激活当前连接 Active the Connection
void Connection::Active()
{
    if(!active_.load()) // 检查连接是否未被激活
    {
        loop_->RunInLoop([this](){ 
            // 将激活操作提交到事件循环中，保证线程安全
            // Submit the activation operation to the event loop for thread safety
            active_.store(true); // 设置 active_ 状态为 true
            if(active_cb_) // 如果设置了回调函数
            {
                // 执行回调函数，传入当前 Connection 的智能指针
                active_cb_(std::dynamic_pointer_cast<Connection>(shared_from_this()));
                // Execute the callback function with the current Connection's shared_ptr
            }
        });
    }
}

/// 取消激活当前连接 Deactivate the Connection
void Connection::Deactive()
{
    active_.store(false); // 设置 active_ 状态为 false
    // Set the active state to false (mark connection as inactive)
}


// 说人话总结 (中文+英文):
// 代码目的 (Purpose):
// 这段代码实现了一个 Connection 类，用来管理网络连接。包括：

// 设置和获取本地/远程地址 (Local/Peer Address)。
// 管理上下文数据（用 std::shared_ptr 存储）。
// 设置激活状态并执行回调函数 (Active State and Callbacks)。
// 激活连接 (Active)：
// 当新用户连接时，可以通过 Active 方法触发回调，比如打印 IP 地址或处理业务逻辑。

// cpp
// Copy code
// connection->SetActiveCallback([](std::shared_ptr<Connection> conn) {
//     std::cout << "新用户连接，IP: " << conn->PeerAddr() << std::endl;
// });
// connection->Active();
// 上下文管理 (Context Management)：
// 使用 SetContext 和 ClearContext 方法管理不同类型的数据，比如用户状态、配置信息。

// 线程安全操作 (Thread-Safety):
// 所有激活逻辑通过 EventLoop 确保在线程安全的环境中执行。

// 简单来说：这个类提供了一套网络连接的管理机制，适合高性能服务器处理用户连接、管理状态并触发相关回调。