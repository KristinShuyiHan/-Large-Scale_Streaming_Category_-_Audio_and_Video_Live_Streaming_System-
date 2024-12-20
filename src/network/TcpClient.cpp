#include "TcpClient.h"
#include "network/base/Network.h"
#include "network/base/SocketOpt.h"

using namespace tmms::network;

// **TcpClient 类的构造函数**  
// Constructor initializes TcpConnection (父类), 并设置目标服务器地址
// Initialize TcpConnection base class with a loop, invalid file descriptor (-1), 
// default local address, and the provided server address.
TcpClient::TcpClient(EventLoop *loop,const InetAddress &server)
:TcpConnection(loop,-1,InetAddress(),server),server_addr_(server)
{
    // Nothing specific to do here, 初始化工作在基类中已经完成  
    // Example: 传入的 server 地址是目标服务器的 IP 和端口  
}

// **TcpClient 的析构函数**
// Destructor ensures that the connection is safely closed
TcpClient::~TcpClient()
{
    OnClose(); // 确保连接关闭
    // Ensure all resources (file descriptors, event listeners) are cleaned up properly
}

// **设置连接成功时的回调函数 (重载版本1)**  
// Set a callback function for connection success (copy version)
void TcpClient::SetConnectCallback(const ConnectionCallback &cb)
{
    connected_cb_ = cb;  // 保存传入的回调函数  
}

// **设置连接成功时的回调函数 (重载版本2)**  
// Set a callback function for connection success (move version for efficiency)
void TcpClient::SetConnectCallback(ConnectionCallback &&cb)
{
    connected_cb_ = std::move(cb);  // 使用 std::move 提高效率，减少拷贝  
}

// **发起连接请求**  
// Public method to start the connection process
void TcpClient::Connect()
{
    // 在 EventLoop 线程中执行连接操作  
    // Ensure the connection logic is executed within the EventLoop thread  
    loop_->RunInLoop([this](){
        ConnectInLoop();
    });
}

// **实际执行连接操作**  
// Internal method to handle the connection process
void TcpClient::ConnectInLoop()
{
    loop_->AssertInLoopThread();  // 确保当前线程是 EventLoop 线程  
    fd_ = SocketOpt::CreateNonblockingTcpSocket(AF_INET); // 创建非阻塞套接字 (TCP socket)
    if(fd_ < 0)  // 如果套接字创建失败  
    {
        OnClose(); // 关闭连接  
        return ;
    }
    status_ = kTcpConStatusConnecting;  // 更新状态为 "正在连接"  
    loop_->AddEvent(std::dynamic_pointer_cast<TcpClient>(shared_from_this()));  
    EnableWriting(true);  // 监听可写事件，以完成非阻塞连接  
    SocketOpt opt(fd_);  // 设置套接字选项  
    auto ret = opt.Connect(server_addr_);  // 发起连接请求  
    if(ret == 0)  // 如果连接立即成功  
    {
        UpdateConnectionStatus();  // 更新连接状态  
        return;
    }
    else if(ret == -1)  // 如果连接失败  
    {
        if(errno != EINPROGRESS) // EINPROGRESS 表示连接正在进行中 (非阻塞)  
        {
            NETWORK_ERROR << "connect to server:" << server_addr_.ToIpPort() << " error:" << errno;
            OnClose(); // 连接失败，关闭连接  
            return ;
        }
    }
}

// **更新连接状态为已连接**  
// Update the connection status and call the user-defined callback function
void TcpClient::UpdateConnectionStatus()
{
    status_=kTcpConStatusConnected; // 更新状态为已连接  
    if(connected_cb_) // 如果用户设置了回调函数  
    {
        connected_cb_(std::dynamic_pointer_cast<TcpClient>(shared_from_this()),true);
        // 调用回调函数，通知连接成功  
    }
}

// **检查套接字是否有错误**  
// Check for socket errors
bool TcpClient::CheckError()
{
    int error = 0;
    socklen_t len = sizeof(error);
    ::getsockopt(fd_,SOL_SOCKET,SO_ERROR,&error,&len); // 获取套接字的错误状态  
    return error != 0;  // 返回是否有错误  
}

// **处理读事件**  
// Handle readable events (data is available or connection is established)
void TcpClient::OnRead()
{
    if(status_ == kTcpConStatusConnecting)  // 如果当前状态是 "正在连接"  
    {
        if(CheckError())  // 检查连接是否出错  
        {
            NETWORK_ERROR << "connect to server:" << server_addr_.ToIpPort() << " error:" << errno;
            OnClose(); // 关闭连接  
            return;
        }
        UpdateConnectionStatus(); // 更新状态为已连接  
        return ;
    }
    else if(status_ == kTcpConStatusConnected)  // 如果已连接，处理数据读取  
    {
        TcpConnection::OnRead();  // 调用基类的 OnRead 处理数据  
    }
}

// **处理写事件**  
// Handle writable events (data can be written or connection established)
void TcpClient::OnWrite()
{
    if(status_ == kTcpConStatusConnecting)  // 如果当前状态是 "正在连接"  
    {
        if(CheckError())  // 检查连接是否出错  
        {
            NETWORK_ERROR << "connect to server:" << server_addr_.ToIpPort() << " error:" << errno;
            OnClose(); // 关闭连接  
            return;
        }
        UpdateConnectionStatus(); // 更新状态为已连接  
        return ;
    }
    else if(status_ == kTcpConStatusConnected)  // 如果已连接，处理数据写入  
    {
        TcpConnection::OnWrite();  // 调用基类的 OnWrite 处理数据  
    }
}

// **关闭连接**  
// Close the connection and clean up resources
void TcpClient::OnClose()
{
    if(status_ == kTcpConStatusConnecting ||
        status_ == kTcpConStatusConnected) // 如果连接未完成或已连接  
    {
        loop_->DelEvent(std::dynamic_pointer_cast<TcpClient>(shared_from_this()));
        // 从事件循环中删除当前连接  
    }
    status_ = kTcpConStatusDisConnected;  // 更新状态为 "已断开"  
    TcpConnection::OnClose();  // 调用基类的 OnClose 清理资源  
}

// **发送数据 (列表版本)**  
// Send data if the connection is established
void TcpClient::Send(std::list<BufferNodePtr>&list)
{
    if(status_ == kTcpConStatusConnected) // 确保连接已建立  
    {
        TcpConnection::Send(list); // 调用基类发送数据  
    }
}

// **发送数据 (字符指针版本)**  
// Send raw data if the connection is established
void TcpClient::Send(const char *buf,size_t size)
{
    if(status_ == kTcpConStatusConnected) // 确保连接已建立  
    {
        TcpConnection::Send(buf,size); // 调用基类发送数据  
    }
}
