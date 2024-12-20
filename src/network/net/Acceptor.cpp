#include "Acceptor.h"
#include "network/base/Network.h"

using namespace tmms::network;

// 构造函数，初始化 Acceptor 对象，设置事件循环和地址。
// Constructor: Initializes Acceptor with an event loop and an address.
Acceptor::Acceptor(EventLoop *loop, const InetAddress &addr)
    : Event(loop), addr_(addr) // 初始化父类 Event 和地址对象 addr_
{
    // Nothing else to initialize here.
}

// 析构函数：清理资源。
// Destructor: Cleans up resources.
Acceptor::~Acceptor()
{
    Stop(); // 停止当前的事件监听
    if (socket_opt_) // 如果 socket_opt_ 不为空，释放内存。
    {
        delete socket_opt_;  // 删除 SocketOpt 对象，防止内存泄漏。
        socket_opt_ = nullptr; // 将指针置为 nullptr，避免悬挂指针。
    }
}

// 设置连接回调函数，接收拷贝的函数对象。
// Sets the accept callback function using a copy.
void Acceptor::SetAcceptCallback(const AcceptCallback &cb)
{
    accept_cb_ = cb; // 将传入的回调函数保存到 accept_cb_
}

// 设置连接回调函数，接收右值引用（移动操作）。
// Sets the accept callback function using a move operation.
void Acceptor::SetAcceptCallback(AcceptCallback &&cb)
{
    accept_cb_ = std::move(cb); // 使用 std::move 将回调函数移动到 accept_cb_
}

// 打开套接字，初始化监听。
// Opens a socket and starts listening for incoming connections.
void Acceptor::Open()
{
    // 1. 如果之前有打开的套接字，先关闭它。
    if (fd_ > 0)
    {
        ::close(fd_); // 调用系统的 close 关闭文件描述符
        fd_ = -1; // 重置文件描述符
    }

    // 2. 根据 IP 类型创建非阻塞套接字（IPv6 或 IPv4）
    if (addr_.IsIpV6()) // 如果地址是 IPv6
    {
        fd_ = SocketOpt::CreateNonblockingTcpSocket(AF_INET6);
    }
    else // 如果地址是 IPv4
    {
        fd_ = SocketOpt::CreateNonblockingTcpSocket(AF_INET);
    }

    // 3. 检查套接字是否创建成功
    if (fd_ < 0)
    {
        NETWORK_ERROR << "socket failed.errno:" << errno; // 输出错误信息
        exit(-1); // 退出程序
    }

    // 4. 如果 socket_opt_ 已存在，释放旧的资源
    if (socket_opt_)
    {
        delete socket_opt_;
        socket_opt_ = nullptr;
    }

    // 5. 添加事件到事件循环（EventLoop）中
    loop_->AddEvent(std::dynamic_pointer_cast<Acceptor>(shared_from_this()));

    // 6. 初始化 socket_opt_ 并设置套接字选项
    socket_opt_ = new SocketOpt(fd_);
    socket_opt_->SetReuseAddr(true);  // 设置地址重用，减少端口绑定失败。
    socket_opt_->SetReusePort(true);  // 设置端口重用，多个进程可以绑定同一端口。
    socket_opt_->BindAddress(addr_);  // 绑定地址和端口
    socket_opt_->Listen();            // 开启监听，等待客户端连接
}

// 开启 Acceptor，在线程安全的 EventLoop 中调用 Open。
// Starts the Acceptor in the EventLoop, ensuring thread safety.
void Acceptor::Start()
{
    loop_->RunInLoop([this](){
        Open(); // 在事件循环线程中执行 Open 方法
    });
}

// 停止监听，移除事件。
// Stops the Acceptor and removes it from the event loop.
void Acceptor::Stop()
{
    loop_->DelEvent(std::dynamic_pointer_cast<Acceptor>(shared_from_this())); // 从事件循环中删除当前对象
}

// 处理可读事件，接受新连接。
// Handles readable events to accept new connections.
void Acceptor::OnRead()
{
    if (!socket_opt_) // 如果 socket_opt_ 不存在，直接返回
    {
        return;
    }
    
    while (true) // 循环接受所有待处理的连接
    {
        InetAddress addr; // 用于存储客户端的地址
        auto sock = socket_opt_->Accept(&addr); // 调用 Accept 方法接收一个连接

        if (sock >= 0) // 如果连接接收成功
        {
            if (accept_cb_) // 如果设置了回调函数
            {
                accept_cb_(sock, addr); // 调用回调函数，传递客户端套接字和地址
            }
        }
        else // 如果接收失败
        {
            if (errno != EINTR && errno != EAGAIN) // 忽略中断错误或没有更多连接的错误
            {
                NETWORK_ERROR << "acceptor error.errno:" << errno; // 输出错误信息
                OnClose(); // 执行关闭逻辑
            }
            break; // 退出循环
        }
    }
}

// 处理错误事件，输出错误信息并关闭 Acceptor。
// Handles errors, outputs error message, and closes the Acceptor.
void Acceptor::OnError(const std::string &msg)
{
    NETWORK_ERROR << "acceptor error:" << msg; // 输出错误信息
    OnClose(); // 执行关闭逻辑
}

// 关闭并重新打开 Acceptor。
// Closes and reopens the Acceptor.
void Acceptor::OnClose()
{
    Stop(); // 停止监听，移除事件
    Open(); // 重新打开套接字，恢复监听状态
}
// 中文总结（Inline Comment 解释）
// 类的功能：
// Acceptor 类负责接受客户端的网络连接，并调用回调函数 accept_cb_ 处理连接。

// 重要函数：

// Open()：创建套接字、设置监听，并加入事件循环。
// Start()：确保 Open() 方法在线程安全的 EventLoop 中执行。
// OnRead()：循环接受客户端连接，并调用回调函数。
// OnError() 和 OnClose()：处理错误，重启 Acceptor。
// 示例场景：
// 假设你有一个服务器，需要不断地接收来自客户端的连接：

// Start() 启动监听，等待连接。
// 每当有新客户端连接时，OnRead() 会接收连接，并调用回调函数通知用户处理连接。
// 如果监听出现问题，OnClose() 会重启监听，确保服务不中断。
