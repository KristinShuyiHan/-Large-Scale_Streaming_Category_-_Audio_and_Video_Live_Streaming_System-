#pragma once
// 避免头文件被重复包含，保证只编译一次
// Ensures the header file is only included once during compilation.

#include "InetAddress.h"  // 包含 InetAddress 类的声明
#include <sys/socket.h>   // 提供套接字（Socket）编程的系统调用
#include <sys/types.h>    // 定义了一些数据类型，如 size_t、pid_t 等
#include <netinet/in.h>   // 提供与 Internet 地址结构体相关的定义
#include <netinet/tcp.h>  // 提供 TCP 协议相关的选项，比如 TCP_NODELAY
#include <unistd.h>       // 提供 close() 函数，用于关闭文件描述符
#include <fcntl.h>        // 提供控制文件描述符属性的函数，如 fcntl()
#include <memory>         // 提供智能指针 std::shared_ptr

namespace tmms // 命名空间 "tmms"，用于防止命名冲突
{
    namespace network // 子命名空间 "network"，与网络相关的功能
    {
        using InetAddressPtr = std::shared_ptr<InetAddress>; 
        // 使用 shared_ptr 管理 InetAddress 类的实例，避免内存泄漏
        // Shared pointer to manage InetAddress object, ensuring no memory leaks.

        class SocketOpt
        {
        public:
            SocketOpt(int sock, bool v6 = false);
            // 构造函数，初始化套接字和是否为 IPv6 标识符
            // Constructor initializes socket descriptor and IPv6 flag.
            
            ~SocketOpt() = default;
            // 默认析构函数，无需自定义清理逻辑
            // Default destructor as no custom cleanup logic is required.

            static int CreateNonblockingTcpSocket(int family);
            // 创建一个非阻塞的 TCP 套接字，family 指定地址族 (AF_INET/AF_INET6)
            // Creates a non-blocking TCP socket. 'family' specifies address family (AF_INET or AF_INET6).

            static int CreateNonblockingUdpSocket(int family);
            // 创建一个非阻塞的 UDP 套接字，family 指定地址族
            // Creates a non-blocking UDP socket with specified address family.

            int BindAddress(const InetAddress &localaddr);
            // 将套接字绑定到指定的本地地址
            // Binds the socket to the specified local address.

            int Listen();
            // 将套接字设置为监听模式，等待客户端连接
            // Sets the socket to listen mode, waiting for incoming client connections.

            int Accept(InetAddress *peeraddr);
            // 接受客户端连接，返回一个新的套接字，并将对端地址存储在 peeraddr 中
            // Accepts a client connection, returning a new socket and storing the peer address in 'peeraddr'.

            int Connect(const InetAddress &addr);
            // 连接到指定的地址（用于客户端套接字）
            // Connects to the specified address (used for client sockets).

            InetAddressPtr GetLocalAddr();
            // 获取本地套接字地址，返回一个智能指针
            // Retrieves the local socket address as a shared pointer.

            InetAddressPtr GetPeerAddr();
            // 获取对端套接字地址，返回一个智能指针
            // Retrieves the peer socket address as a shared pointer.

            void SetTcpNoDelay(bool on);
            // 设置 TCP_NODELAY 选项，关闭 Nagle 算法，减少延迟
            // Enables/disables TCP_NODELAY to reduce latency by disabling Nagle's algorithm.

            void SetReuseAddr(bool on);
            // 设置 SO_REUSEADDR 选项，允许重新绑定已在使用的地址
            // Enables/disables SO_REUSEADDR to allow reusing an address already in use.

            void SetReusePort(bool on);
            // 设置 SO_REUSEPORT 选项，允许多个套接字绑定到同一端口
            // Enables/disables SO_REUSEPORT to allow multiple sockets to bind to the same port.

            void SetKeepAlive(bool on);
            // 设置 SO_KEEPALIVE 选项，启用 TCP 保活机制，检测断开连接
            // Enables/disables SO_KEEPALIVE to detect broken connections.

            void SetNonBlocking(bool on);
            // 设置套接字为非阻塞模式
            // Sets the socket to non-blocking mode.

        private:
            int sock_{-1};   // 套接字文件描述符，初始化为 -1，表示无效
            // Socket file descriptor initialized to -1 (invalid).

            bool is_v6_{false}; // 标记是否为 IPv6 套接字，默认值为 false
            // Boolean flag to indicate if the socket is IPv6. Defaults to false.
        };
    }
}
