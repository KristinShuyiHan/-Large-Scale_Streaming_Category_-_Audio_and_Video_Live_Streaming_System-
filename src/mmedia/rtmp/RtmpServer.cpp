
#include "RtmpServer.h"  // 引入RtmpServer类的头文件，声明该类的接口和定义。
// Include the RtmpServer header file that declares the RtmpServer class.

#include "mmedia/base/MMediaLog.h"  // 用于日志输出的工具类，提供DEBUG、TRACE等日志级别。
// Include MMediaLog for logging purposes such as DEBUG and TRACE messages.

#include "RtmpContext.h"  // 引入RtmpContext类，用于管理RTMP的上下文（握手、解析等）。
// Include RtmpContext, which handles the context of RTMP connections, such as handshaking and message parsing.

using namespace tmms::mm;  // 使用tmms::mm命名空间，简化代码中对tmms库的访问。
// Use the namespace "tmms::mm" to avoid prefixing every function with the full namespace.



// 构造函数：初始化RTMP服务器
// Constructor: Initializes the RtmpServer object.
RtmpServer::RtmpServer(EventLoop *loop, const InetAddress &local, RtmpHandler *handler)
    : TcpServer(loop, local), rtmp_handler_(handler)  // 调用基类TcpServer的构造函数，初始化事件循环和本地地址。
{
    // rtmp_handler_ 是RTMP连接处理器的指针，用于回调连接相关事件。
    // rtmp_handler_ points to a handler object that processes RTMP connection events.
}

// 析构函数：停止RTMP服务器
// Destructor: Stops the RtmpServer when the object is destroyed.
RtmpServer::~RtmpServer()
{
    Stop();  // 调用Stop()方法，停止服务器
}


// 启动服务器，绑定事件回调函数
// Start the server and set callback functions for various events.
void RtmpServer::Start()
{
    // 设置客户端连接激活的回调函数
    TcpServer::SetActiveCallback(std::bind(&RtmpServer::OnActive, this, std::placeholders::_1));
    // Set callback for when a connection becomes active.

    // 设置客户端连接销毁的回调函数
    TcpServer::SetDestroyConnectionCallback(std::bind(&RtmpServer::OnDestroyed, this, std::placeholders::_1));
    // Set callback for when a connection is destroyed.

    // 设置新连接建立时的回调函数
    TcpServer::SetNewConnectionCallback(std::bind(&RtmpServer::OnNewConnection, this, std::placeholders::_1));
    // Set callback for when a new connection is established.

    // 设置写入完成时的回调函数
    TcpServer::SetWriteCompleteCallback(std::bind(&RtmpServer::OnWriteComplete, this, std::placeholders::_1));
    // Set callback for when data is completely written to the connection.

    // 设置消息接收时的回调函数
    TcpServer::SetMessageCallback(std::bind(&RtmpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    // Set callback for when a message is received.

    // 启动TcpServer，开始监听客户端连接
    TcpServer::Start();
    RTMP_DEBUG << "RtmpServer Start";  // 输出服务器启动日志
    // Log a debug message indicating the server has started.
}

// 停止服务器
// Stops the server.
void RtmpServer::Stop()
{
    TcpServer::Stop();  // 调用基类TcpServer的Stop()方法，停止监听和清理连接。
    // Call the base class TcpServer's Stop() method to stop listening and clean up connections.
}


// 处理新连接的回调函数
// Callback when a new connection is established.
void RtmpServer::OnNewConnection(const TcpConnectionPtr &conn)
{
    if (rtmp_handler_)  // 检查是否存在RTMP处理器
    {
        rtmp_handler_->OnNewConnection(conn);  // 通知处理器有新连接建立
        // Notify the handler that a new connection has been established.
    }

    // 创建一个RtmpContext，用于管理RTMP握手和上下文
    RtmpContextPtr shake = std::make_shared<RtmpContext>(conn, rtmp_handler_);
    // Create an RtmpContext to manage RTMP handshakes and connection context.

    conn->SetContext(kRtmpContext, shake);  // 设置RTMP上下文到连接中
    // Set the RTMP context to the connection.

    shake->StartHandShake();  // 开始RTMP握手流程
    // Start the RTMP handshake process.
}

// 处理连接销毁的回调函数
// Callback when a connection is destroyed.
void RtmpServer::OnDestroyed(const TcpConnectionPtr &conn)
{
    if (rtmp_handler_)  // 检查是否存在RTMP处理器
    {
        rtmp_handler_->OnConnectionDestroy(conn);  // 通知处理器连接销毁
        // Notify the handler that a connection is being destroyed.
    }
    conn->ClearContext(kRtmpContext);  // 清理连接中的RTMP上下文
    // Clear the RTMP context stored in the connection.
}

// 处理接收到的消息
// Callback when a message is received.
void RtmpServer::OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf)
{
    RtmpContextPtr shake = conn->GetContext<RtmpContext>(kRtmpContext);  // 获取RTMP上下文
    // Retrieve the RTMP context from the connection.

    if (shake)  // 确保RTMP上下文存在
    {
        int ret = shake->Parse(buf);  // 解析消息内容
        // Parse the incoming message buffer.

        if (ret == 0)  // 握手成功
        {
            RTMP_TRACE << "host: " << conn->PeerAddr().ToIpPort() << " handshake success.";
            // Log handshake success with the client's IP and port.
        }
        else if (ret == -1)  // 握手失败，关闭连接
        {
            conn->ForceClose();  // 强制关闭连接
            // Force close the connection if handshake fails.
        }
    }
}

// 处理数据写入完成
// Callback when data has been completely written to the connection.
void RtmpServer::OnWriteComplete(const ConnectionPtr &conn)
{
    RtmpContextPtr shake = conn->GetContext<RtmpContext>(kRtmpContext);  // 获取RTMP上下文
    // Retrieve the RTMP context from the connection.

    if (shake)
    {
        shake->OnWriteComplete();  // 通知上下文写入完成
        // Notify the context that the write operation is complete.
    }
}

// 处理连接激活
// Callback when a connection becomes active.
void RtmpServer::OnActive(const ConnectionPtr &conn)
{
    if (rtmp_handler_)
    {
        rtmp_handler_->OnActive(conn);  // 通知处理器连接激活
        // Notify the handler that the connection has become active.
    }
}

// 在C++大型流媒体项目中的作用
// 底层服务器实现：

// 该代码实现了一个RTMP（Real-Time Messaging Protocol）服务器，用于接收和处理直播推流、消息、握手等功能。
// RTMP是流媒体系统的关键协议，广泛用于直播推流服务。
// 模块化设计：

// 使用事件回调（如 OnNewConnection、OnMessage）实现高扩展性，方便加入新功能。
// 上层业务可以通过 RtmpHandler 处理具体的推流逻辑。
// 并发处理：

// 通过TcpServer和事件驱动模型，支持高并发连接，满足千万级直播流量的需求。
// 上下文管理：

// 使用 RtmpContext 管理每个连接的状态（如握手和消息解析），实现逻辑的清晰分离。
// 日志跟踪：

// 使用 MMediaLog 提供调试和日志输出，便于系统监控和排查问题。
// 总结
// 在一个千万级直播系统中，这段代码实现了RTMP服务器的核心功能，提供了高并发支持、上下文管理和事件回调机制，使服务器能够稳定地接收和处理直播推流请求。这种设计保证了流媒体系统的高性能和可扩展性，是底层到应用层的关键组件之一。