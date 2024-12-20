#pragma once  // 防止头文件被重复包含，确保编译时只包含一次。
// Prevents the header file from being included multiple times during compilation.

#include <sys/socket.h>  // 提供套接字相关函数和结构体，如 `sockaddr`。
// Provides socket-related functions and structures, such as `sockaddr`.

#include <arpa/inet.h>  // 提供 IP 地址转换相关函数，比如 `inet_pton` 和 `inet_ntoa`。
// Includes functions for converting IP addresses, such as `inet_pton` and `inet_ntoa`.

#include <netinet/in.h>  // 包含网络地址结构体定义，比如 `sockaddr_in`。
// Contains definitions for network address structures, such as `sockaddr_in`.

#include <bits/socket.h> // 低级套接字相关定义，通常包含基础网络常量。
// Low-level socket definitions, often used for fundamental constants.

#include <string>  // 提供 `std::string` 类型，用于字符串操作。
// Provides the `std::string` type for string operations.

namespace tmms  // 命名空间 `tmms`，避免与其他代码冲突。
// Namespace `tmms` to avoid name conflicts with other code.
{
    namespace network  // 子命名空间 `network`，表示网络相关的功能模块。
// Sub-namespace `network` for network-related functionalities.
    {
        class InetAddress  // 定义 `InetAddress` 类，用于管理 IP 地址和端口信息。
// Defines the `InetAddress` class for managing IP addresses and port information.
        {
        public:
            // 构造函数：通过 IP 地址和端口初始化对象，可以选择是否为 IPv6 地址。
            // Constructor: Initializes the object with IP address and port, supports IPv6.
            InetAddress(const std::string &ip, uint16_t port, bool bv6 = false);

            // 构造函数：通过主机名初始化对象，可以选择是否为 IPv6 地址。
            // Constructor: Initializes the object with a hostname, supports IPv6.
            InetAddress(const std::string &host, bool is_v6 = false);

            // 默认构造函数：不做任何初始化。
            // Default constructor: Performs no initialization.
            InetAddress() = default;

            // 默认析构函数：用于资源清理。
            // Default destructor: Cleans up resources.
            ~InetAddress() = default;

            // 设置主机名。
            // Sets the hostname.
            void SetHost(const std::string &host);

            // 设置 IP 地址。
            // Sets the IP address.
            void SetAddr(const std::string &addr);

            // 设置端口号。
            // Sets the port number.
            void SetPort(uint16_t port);

            // 设置是否为 IPv6。
            // Sets whether the address is IPv6.
            void SetIsIPV6(bool is_v6);

            // 获取 IP 地址字符串。
            // Returns the IP address as a string.
            const std::string &IP() const;

            // 将 IP 地址转换为 32 位整数，仅适用于 IPv4。
            // Converts the IP address to a 32-bit integer, applicable for IPv4 only.
            uint32_t IPv4() const;

            // 返回 IP 和端口的组合字符串，如 "192.168.1.1:8080"。
            // Returns a combined string of IP and port, e.g., "192.168.1.1:8080".
            std::string ToIpPort() const;

            // 获取端口号。
            // Returns the port number.
            uint16_t Port() const;

            // 获取套接字地址结构体，用于 socket 通信。
            // Retrieves the socket address structure, used for socket communication.
            void GetSockAddr(struct sockaddr *saddr) const;

            // 判断是否为 IPv6 地址。
            // Determines if the address is IPv6.
            bool IsIpV6() const;

            // 判断是否为广域网 IP。
            // Determines if the address is a WAN IP.
            bool IsWanIp() const;

            // 判断是否为局域网 IP。
            // Determines if the address is a LAN IP.
            bool IsLanIp() const;

            // 判断是否为回环 IP 地址 (127.0.0.1)。
            // Determines if the address is a loopback IP (e.g., 127.0.0.1).
            bool IsLoopbackIp() const;

            // 静态方法：根据主机名解析出 IP 和端口。
            // Static method: Resolves the IP and port from a hostname.
            static void GetIpAndPort(const std::string &host, std::string &ip, std::string &port);

        private:
            // 将字符串形式的 IP 地址转换为 32 位整数（仅适用于 IPv4）。
            // Converts an IP address in string form to a 32-bit integer (IPv4 only).
            uint32_t IPv4(const char *ip) const;

            // 成员变量：存储 IP 地址字符串。
            // Member variable: Stores the IP address as a string.
            std::string addr_;

            // 成员变量：存储端口号字符串。
            // Member variable: Stores the port number as a string.
            std::string port_;

            // 成员变量：标记是否为 IPv6 地址。
            // Member variable: Indicates whether the address is IPv6.
            bool is_ipv6_{false};
        };
    }
}
