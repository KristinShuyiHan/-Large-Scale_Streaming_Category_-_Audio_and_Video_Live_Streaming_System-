#include "TcpServer.h"
#include "network/base/Network.h"
using namespace tmms::network;

// 构造函数：初始化 TcpServer 对象  
// Constructor: Initializes the TcpServer object
TcpServer::TcpServer(EventLoop *loop, const InetAddress &addr)
    : loop_(loop), addr_(addr) // 将传入的事件循环（loop）和地址（addr）初始化成员变量
{
    // 创建 Acceptor，用于监听新连接  
    // Create an Acceptor to listen for new connections
    acceptor_ = std::make_shared<Acceptor>(loop, addr);
}

// 析构函数：清理 TcpServer 资源  
// Destructor: Cleans up TcpServer resources
TcpServer::~TcpServer()
{
}

// 设置新连接回调函数（传引用的版本）  
// Set the callback for handling new connections (by reference)
void TcpServer::SetNewConnectionCallback(const NewConnectionCallback &cb)
{
    new_connection_cb_ = cb;
}

// 设置新连接回调函数（传右值引用的版本）  
// Set the callback for handling new connections (using move semantics)
void TcpServer::SetNewConnectionCallback(NewConnectionCallback &&cb)
{
    new_connection_cb_ = std::move(cb); // 使用 std::move 提高性能
}

// 设置连接销毁回调函数（传引用的版本）  
// Set the callback for handling destroyed connections (by reference)
void TcpServer::SetDestroyConnectionCallback(const DestroyConnectionCallback &cb)
{
    destroy_connection_cb_ = cb;
}

// 设置连接销毁回调函数（传右值引用的版本）  
// Set the callback for handling destroyed connections (using move semantics)
void TcpServer::SetDestroyConnectionCallback(DestroyConnectionCallback &&cb)
{
    destroy_connection_cb_ = std::move(cb);
}

// 设置活动回调函数（传引用的版本）  
// Set the callback for active events (by reference)
void TcpServer::SetActiveCallback(const ActiveCallback &cb)
{
    active_cb_ = cb;
}

// 设置活动回调函数（传右值引用的版本）  
// Set the callback for active events (using move semantics)
void TcpServer::SetActiveCallback(ActiveCallback &&cb)
{
    active_cb_ = std::move(cb);
}

// 设置写完成回调函数（传引用的版本）  
// Set the callback for write completion (by reference)
void TcpServer::SetWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    write_complete_cb_ = cb;
}

// 设置写完成回调函数（传右值引用的版本）  
// Set the callback for write completion (using move semantics)
void TcpServer::SetWriteCompleteCallback(WriteCompleteCallback &&cb)
{
    write_complete_cb_ = std::move(cb);
}

// 设置消息接收回调函数（传引用的版本）  
// Set the callback for message reception (by reference)
void TcpServer::SetMessageCallback(const MessageCallback &cb)
{
    message_cb_ = cb;
}

// 设置消息接收回调函数（传右值引用的版本）  
// Set the callback for message reception (using move semantics)
void TcpServer::SetMessageCallback(MessageCallback &&cb)
{
    message_cb_ = std::move(cb);
}

// 处理新连接的回调函数  
// Callback function for handling new connections
void TcpServer::OnAccet(int fd, const InetAddress &addr)
{
    NETWORK_TRACE << "new connection fd:" << fd << " host:" << addr.ToIpPort(); // 打印新连接的信息
    // 创建一个新的 TcpConnection 对象，代表一个新的客户端连接  
    // Create a new TcpConnection object representing the new client connection
    TcpConnectionPtr con = std::make_shared<TcpConnection>(loop_, fd, addr_, addr);

    // 设置连接关闭的回调函数  
    // Bind the close callback to handle connection closure
    con->SetCloseCallback(std::bind(&TcpServer::OnConnectionClose, this, std::placeholders::_1));

    // 如果写完成回调函数存在，设置它  
    // Set the write completion callback if it exists
    if (write_complete_cb_)
    {
        con->SetWriteCompleteCallback(write_complete_cb_);
    }

    // 如果活动回调函数存在，设置它  
    // Set the active callback if it exists
    if (active_cb_)
    {
        con->SetActiveCallback(active_cb_);
    }

    // 设置接收消息的回调函数  
    // Set the callback for receiving messages
    con->SetRecvMsgCallback(message_cb_);

    // 将新连接添加到连接集合中  
    // Add the new connection to the connections set
    connections_.insert(con);

    // 将事件添加到事件循环中  
    // Add the connection's event to the event loop
    loop_->AddEvent(con);

    // 启用连接的空闲超时检测，30 秒  
    // Enable idle timeout detection for the connection (30 seconds)
    con->EnableCheckIdleTimeout(30);

    // 如果新连接回调函数存在，调用它  
    // Call the new connection callback if it exists
    if (new_connection_cb_)
    {
        new_connection_cb_(con);
    }
}

// 处理连接关闭的回调函数  
// Callback function for handling connection closure
void TcpServer::OnConnectionClose(const TcpConnectionPtr &con)
{
    NETWORK_TRACE << "host:" << con->PeerAddr().ToIpPort() << " closed."; // 打印关闭连接的信息
    loop_->AssertInLoopThread(); // 确保当前线程是事件循环线程
    connections_.erase(con); // 从连接集合中移除关闭的连接
    loop_->DelEvent(con); // 从事件循环中删除事件
    if (destroy_connection_cb_) // 如果销毁回调函数存在，调用它
    {
        destroy_connection_cb_(con);
    }
}

// 启动 TcpServer，开始监听新连接  
// Start the TcpServer and begin listening for new connections
void TcpServer::Start()
{
    // 设置 Acceptor 的回调函数，当有新连接时调用 OnAccet  
    // Set the Acceptor's callback to call OnAccet when a new connection arrives
    acceptor_->SetAcceptCallback(std::bind(&TcpServer::OnAccet, this, std::placeholders::_1, std::placeholders::_2));

    // 启动 Acceptor，开始监听端口  
    // Start the Acceptor to listen on the specified port
    acceptor_->Start();
}

// 停止 TcpServer，停止监听新连接  
// Stop the TcpServer and stop listening for new connections
void TcpServer::Stop()
{
    acceptor_->Stop(); // 调用 Acceptor 的 Stop 方法，停止监听
}

// 代码总结与解释
// TcpServer 类的作用：

// TcpServer 是一个网络服务器，主要用于监听端口、接受客户端连接，并管理连接的生命周期。
// 关键函数与回调：

// Set*Callback 系列函数：允许外部设置各种事件的回调函数，如新连接、关闭、激活、消息接收等。
// OnAccet：当有新的客户端连接时，创建 TcpConnection 对象，设置各种回调并加入事件循环。
// OnConnectionClose：处理连接关闭事件，清理资源，调用关闭回调。
// Start 和 Stop：分别启动和停止服务器。
// 现实例子：

// 你可以把 TcpServer 想象成一个“电话总机”：
// Start：打开总机，开始接收电话（客户端连接）。
// SetCallback：设定每个电话发生特定事件（如接通、挂断、消息）时要做的事情。
// OnAccet：当有人打电话时，创建一个“通话”（TcpConnection），设置各种处理逻辑。
// Stop：关闭总机，不再接受新电话。
// 代码设计：

// 使用 std::shared_ptr 进行资源管理，确保连接对象的生命周期。
// 回调函数 和 事件驱动机制 提高了代码的灵活性和可扩展性。