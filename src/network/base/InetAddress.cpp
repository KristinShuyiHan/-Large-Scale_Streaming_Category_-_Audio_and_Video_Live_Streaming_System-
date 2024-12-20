#include "InetAddress.h"
#include "Network.h"
#include <cstring>  // 用于字符串操作，如 memset
#include <sstream>  // 用于字符串流操作
#include <iostream> // 输出调试用

using namespace tmms::network;

// 获取 IP 和端口号的方法
void InetAddress::GetIpAndPort(const std::string &host, std::string &ip, std::string &port)
{
    auto pos = host.find_first_of(':', 0); // 查找字符串中第一个 `:` 的位置
    if (pos != std::string::npos) // 如果找到了 `:`
    {
        ip = host.substr(0, pos);      // 提取 `:` 前的部分作为 IP 地址
        port = host.substr(pos + 1);   // 提取 `:` 后的部分作为端口号
    }
    else // 如果没有 `:`，整个 host 是 IP 地址
    {
        ip = host;
    }
    // 举例:
    // 输入: "127.0.0.1:8080"，输出: ip="127.0.0.1"，port="8080"
}

// 构造函数：用于初始化 InetAddress 对象的 IP 地址、端口号和是否为 IPv6 的标志
// Constructor to initialize InetAddress object with IP address, port, and IPv6 flag
InetAddress::InetAddress(const std::string &ip, uint16_t port, bool bv6)
    : addr_(ip),                        // 使用传入的 ip 参数初始化 addr_  (Initialize addr_ with the provided IP)
      port_(std::to_string(port)),      // 将 port 转换为字符串，初始化 port_ (Convert port to string and initialize port_)
      is_ipv6_(bv6)                     // 使用传入的 bv6 布尔值初始化 is_ipv6_ (Initialize is_ipv6_ with the provided bv6 boolean)
{
    // 例子：输入参数 ip="127.0.0.1", port=8080, bv6=false
    // Example: Input ip="127.0.0.1", port=8080, bv6=false
    // 初始化结果：
    // addr_ = "127.0.0.1"    -> IP 地址直接赋值
    // port_ = "8080"         -> 通过 std::to_string 转换端口号为字符串
    // is_ipv6_ = false       -> 标记地址是否为 IPv6

    // 构造函数体为空，因为所有成员变量的初始化都已在成员初始化列表完成
    // The function body is empty as all member variables are initialized in the initializer list
}


// 构造函数，输入 `host` 字符串并解析 IP 和端口
InetAddress::InetAddress(const std::string &host, bool is_v6)
{
    GetIpAndPort(host, addr_, port_); // 解析 IP 和端口号
    is_ipv6_ = is_v6;                 // 设置是否为 IPv6
}

// 设置主机地址，解析 IP 和端口
void InetAddress::SetHost(const std::string &host)
{
    GetIpAndPort(host, addr_, port_);
}

// 设置 IP 地址
void InetAddress::SetAddr(const std::string &addr)
{
    addr_ = addr; // 直接设置 addr_
}

// 设置端口号
void InetAddress::SetPort(uint16_t port)
{
    port_ = std::to_string(port); // 将端口号转为字符串
}

// 设置是否为 IPv6
void InetAddress::SetIsIPV6(bool is_v6)
{
    is_ipv6_ = is_v6;
}

// 获取 IP 地址
const std::string &InetAddress::IP() const
{
    return addr_;
}

// 将 IPv4 地址字符串转换为无符号 32 位整数表示
uint32_t InetAddress::IPv4(const char *ip) const
{
    struct sockaddr_in addr_in;
    memset(&addr_in, 0x00, sizeof(struct sockaddr_in)); // 初始化为 0
    addr_in.sin_family = AF_INET;                      // 设置为 IPv4 协议族
    addr_in.sin_port = 0;
    if (::inet_pton(AF_INET, ip, &addr_in.sin_addr) < 0) // 将字符串转换为网络地址
    {
        NETWORK_ERROR << "ipv4 ip:" << ip << " convert failed.";
    }
    return ntohl(addr_in.sin_addr.s_addr); // 返回主机字节序的整数表示
}

// 将当前的 IP 地址转换为整数形式（IPv4）
uint32_t InetAddress::IPv4() const
{
    return IPv4(addr_.c_str()); // 调用上面的 IPv4 方法
}

// 将 IP 和端口组合成字符串格式 "IP:端口"
std::string InetAddress::ToIpPort() const
{
    std::stringstream ss;
    ss << addr_ << ":" << port_;
    return ss.str();
    // 举例: addr_="127.0.0.1", port_="8080"，输出: "127.0.0.1:8080"
}

// 将字符串端口号转换为整数
uint16_t InetAddress::Port() const
{
    return std::atoi(port_.c_str()); // 将字符串转换为整数
}

// 将当前地址信息填充到 sockaddr 结构中
void InetAddress::GetSockAddr(struct sockaddr *saddr) const
{
    if (is_ipv6_) // 如果是 IPv6
    {
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)saddr;
        memset(addr_in6, 0x00, sizeof(struct sockaddr_in6));
        addr_in6->sin6_family = AF_INET6;
        addr_in6->sin6_port = htons(std::atoi(port_.c_str())); // 设置端口号
        ::inet_pton(AF_INET6, addr_.c_str(), &addr_in6->sin6_addr);
        return;
    }

    // 如果是 IPv4
    struct sockaddr_in *addr_in = (struct sockaddr_in *)saddr;
    memset(addr_in, 0x00, sizeof(struct sockaddr_in));
    addr_in->sin_family = AF_INET;
    addr_in->sin_port = htons(std::atoi(port_.c_str()));
    ::inet_pton(AF_INET, addr_.c_str(), &addr_in->sin_addr);
}

// 判断是否为 IPv6 地址
bool InetAddress::IsIpV6() const
{
    return is_ipv6_;
}

// 判断是否为外网 IP
bool InetAddress::IsWanIp() const
{
    // 定义私有 IP 范围 (A, B, C 类)
    uint32_t a_start = IPv4("10.0.0.0");
    uint32_t a_end = IPv4("10.255.255.255");
    uint32_t b_start = IPv4("172.16.0.0");
    uint32_t b_end = IPv4("172.31.255.255");
    uint32_t c_start = IPv4("192.168.0.0");
    uint32_t c_end = IPv4("192.168.255.255");
    uint32_t ip = IPv4();

    // 检查是否属于私有 IP 范围
    bool is_a = ip >= a_start && ip <= a_end;
    bool is_b = ip >= b_start && ip <= b_end;
    bool is_c = ip >= c_start && ip <= c_end;

    return !is_a && !is_b && !is_c && ip != INADDR_LOOPBACK;
}

// 判断是否为局域网 IP
bool InetAddress::IsLanIp() const
{
    // 与 IsWanIp 类似，只判断是否为 A/B/C 类私有 IP
    return !IsWanIp();
}

// 判断是否为回环地址 (127.0.0.1)
bool InetAddress::IsLoopbackIp() const
{
    return addr_ == "127.0.0.1";
}
