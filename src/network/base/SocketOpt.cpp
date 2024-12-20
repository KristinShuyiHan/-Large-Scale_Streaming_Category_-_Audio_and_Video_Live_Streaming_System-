#include "SocketOpt.h"
#include "Network.h"

using namespace tmms::network;

// 构造函数：初始化套接字和IP版本标志
// Constructor: Initialize socket descriptor and IPv6 flag
SocketOpt::SocketOpt(int sock,bool v6)
:sock_(sock),is_v6_(v6) // sock_ 保存套接字描述符，is_v6_ 标记是否为IPv6
{
    // 构造时不做其他操作，只设置成员变量
}

// 创建一个非阻塞的 TCP 套接字
// Create a non-blocking TCP socket
int SocketOpt::CreateNonblockingTcpSocket(int family)
{
    // 使用 socket() 系统调用创建套接字
    // SOCK_STREAM 表示 TCP，SOCK_NONBLOCK 表示非阻塞，SOCK_CLOEXEC 表示关闭时自动释放
    int sock = ::socket(family,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,IPPROTO_TCP);
    if(sock<0) // 检查套接字是否创建失败
    {
        NETWORK_ERROR << "socket failed."; // 输出错误日志
    }
    return sock; // 返回创建的套接字描述符
}

// 创建一个非阻塞的 UDP 套接字
// Create a non-blocking UDP socket
int SocketOpt::CreateNonblockingUdpSocket(int family)
{
    // SOCK_DGRAM 表示 UDP
    int sock = ::socket(family,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,IPPROTO_UDP);
    if(sock<0)
    {
        NETWORK_ERROR << "socket failed."; // 输出错误日志
    }
    return sock;
}

// 将套接字绑定到本地地址
// Bind the socket to a local address
int SocketOpt::BindAddress(const InetAddress &localaddr)
{
    if(localaddr.IsIpV6()) // 检查是否为 IPv6 地址
    {
        struct sockaddr_in6 addr; // 定义 IPv6 地址结构体
        localaddr.GetSockAddr((struct sockaddr*)&addr); // 获取地址
        socklen_t size = sizeof(struct sockaddr_in6); // 地址长度
        return ::bind(sock_,(struct sockaddr*)&addr,size); // 绑定地址
    }
    else 
    {
        struct sockaddr_in addr; // 定义 IPv4 地址结构体
        localaddr.GetSockAddr((struct sockaddr*)&addr);
        socklen_t size = sizeof(struct sockaddr_in);
        return ::bind(sock_,(struct sockaddr*)&addr,size);
    }
}

// 将套接字设置为监听状态（仅适用于 TCP）
// Set the socket to listen mode (for TCP)
int SocketOpt::Listen()
{
    return ::listen(sock_,SOMAXCONN); // SOMAXCONN 表示系统允许的最大连接数
}

// 接收一个新的连接请求，返回新套接字描述符
// Accept a new connection request and return a new socket descriptor
int SocketOpt::Accept(InetAddress *peeraddr)
{
    struct sockaddr_in6 addr; // 存储对方地址
    socklen_t len= sizeof(struct sockaddr_in6);
    // accept4: 接收连接，SOCK_CLOEXEC 和 SOCK_NONBLOCK 保持新套接字的属性
    int sock = ::accept4(sock_,(struct sockaddr*)&addr,&len,SOCK_CLOEXEC|SOCK_NONBLOCK);
    if(sock>0) // 如果接收成功
    {
        if(addr.sin6_family == AF_INET) // 如果是 IPv4 地址
        {
            char ip[16] = {0,};
            struct sockaddr_in *saddr = (struct sockaddr_in*)&addr;
            ::inet_ntop(AF_INET,&(saddr->sin_addr.s_addr),ip,sizeof(ip)); // 将地址转换为字符串格式
            peeraddr->SetAddr(ip); // 设置地址
            peeraddr->SetPort(ntohs(saddr->sin_port)); // 设置端口
        }
        else if(addr.sin6_family == AF_INET6) // 如果是 IPv6 地址
        {
            char ip[INET6_ADDRSTRLEN] = {0,};
            ::inet_ntop(AF_INET6,&(addr.sin6_addr),ip,sizeof(ip));
            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(addr.sin6_port));
            peeraddr->SetIsIPV6(true); // 设置为 IPv6
        }
    }
    return sock; // 返回新连接的套接字描述符
}

// 连接到一个远程地址
// Connect to a remote address
int SocketOpt::Connect(const InetAddress &addr)
{
    struct sockaddr_in6 addr_in; // 存储目标地址
    addr.GetSockAddr((struct sockaddr*)&addr_in); // 获取地址
    return ::connect(sock_,(struct sockaddr*)&addr_in,sizeof(struct sockaddr_in6)); // 发起连接
}

// 获取本地地址
// Get the local address of the socket
InetAddressPtr SocketOpt::GetLocalAddr()
{
    struct sockaddr_in6 addr_in;
    socklen_t len = sizeof(struct sockaddr_in6);
    ::getsockname(sock_,(struct sockaddr*)&addr_in,&len); // 获取本地地址信息
    InetAddressPtr peeraddr = std::make_shared<InetAddress>();
    if(addr_in.sin6_family == AF_INET)
    {
        char ip[16] = {0,};
        struct sockaddr_in *saddr = (struct sockaddr_in*)&addr_in;
        ::inet_ntop(AF_INET,&(saddr->sin_addr.s_addr),ip,sizeof(ip));
        peeraddr->SetAddr(ip);
        peeraddr->SetPort(ntohs(saddr->sin_port));
    }
    else if(addr_in.sin6_family == AF_INET6)
    {
        char ip[INET6_ADDRSTRLEN] = {0,};
        ::inet_ntop(AF_INET6,&(addr_in.sin6_addr),ip,sizeof(ip));
        peeraddr->SetAddr(ip);
        peeraddr->SetPort(ntohs(addr_in.sin6_port));
        peeraddr->SetIsIPV6(true);
    }
    return peeraddr;
}

// 设置 TCP_NODELAY 属性，禁用 Nagle 算法，减少延迟
// Set TCP_NODELAY to disable Nagle's algorithm
void SocketOpt::SetTcpNoDelay(bool on)
{
    int optvalue = on?1:0;
    ::setsockopt(sock_,IPPROTO_TCP,TCP_NODELAY,&optvalue,sizeof(optvalue));
}

// 设置套接字为可重用地址
// Enable address reuse
void SocketOpt::SetReuseAddr(bool on)
{
    int optvalue = on?1:0;
    ::setsockopt(sock_,SOL_SOCKET,SO_REUSEADDR,&optvalue,sizeof(optvalue));
}

// 这段代码提供了 SocketOpt 类的三个成员函数：

// SetReusePort：设置端口复用。
// SetKeepAlive：设置长连接探测。
// SetNonBlocking：设置套接字为非阻塞模式。


void SocketOpt::SetReusePort(bool on)
{
    int optvalue = on ? 1 : 0;  // 如果参数 on 为 true，optvalue = 1；如果为 false，optvalue = 0。
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEPORT, &optvalue, sizeof(optvalue));  
    // 使用 setsockopt 系统调用设置套接字选项：
    // 参数1: sock_   -> 需要设置的套接字描述符。
    // 参数2: SOL_SOCKET -> 设置套接字级别的选项，SOL_SOCKET 表示操作套接字的通用选项。
    // 参数3: SO_REUSEPORT -> 允许多个套接字绑定同一个端口，提升负载均衡性能。
    // 参数4: &optvalue -> 选项值，1 表示启用，0 表示关闭。
    // 参数5: sizeof(optvalue) -> optvalue 的大小，表示传入参数的长度。
}


void SocketOpt::SetKeepAlive(bool on)
{
    int optvalue = on ? 1 : 0;  // 如果 on 为 true，optvalue = 1；否则 optvalue = 0。
    ::setsockopt(sock_, SOL_SOCKET, SO_KEEPALIVE, &optvalue, sizeof(optvalue));  
    // 使用 setsockopt 系统调用设置套接字选项：
    // 参数1: sock_   -> 需要设置的套接字描述符。
    // 参数2: SOL_SOCKET -> 设置套接字级别的选项，SOL_SOCKET 表示通用套接字选项。
    // 参数3: SO_KEEPALIVE -> 启用 TCP 的 Keep-Alive 功能，检测长时间未响应的连接。
    // 参数4: &optvalue -> 选项值，1 表示启用，0 表示关闭。
    // 参数5: sizeof(optvalue) -> optvalue 的大小。
}
void SocketOpt::SetNonBlocking(bool on)
{
    int flag = ::fcntl(sock_, F_GETFL, 0);  // 获取套接字的当前标志位。
    if (on)
    {
        flag |= O_NONBLOCK;  // 如果 on 为 true，将 O_NONBLOCK 标志位添加到当前标志位中。
    }
    else
    {
        flag &= ~O_NONBLOCK;  // 如果 on 为 false，清除 O_NONBLOCK 标志位。
    }
    ::fcntl(sock_, F_SETFL, flag);  // 更新套接字的标志位。
    // 参数1: sock_   -> 需要操作的套接字。
    // 参数2: F_SETFL -> 设置套接字的标志位。
    // 参数3: flag    -> 更新后的标志位，添加或移除 O_NONBLOCK。
}
