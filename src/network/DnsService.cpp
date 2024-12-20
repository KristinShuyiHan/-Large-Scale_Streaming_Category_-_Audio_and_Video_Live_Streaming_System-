#include "DnsService.h"
#include <functional>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <cstring> // 提供 memset 和字符串操作 // Includes memset and string manipulation functions.

using namespace tmms::network; // 使用命名空间，减少代码中重复前缀 // Using namespace to simplify code.

namespace
{
    static InetAddressPtr inet_address_null; 
    // 定义一个空指针变量，表示默认返回的空地址。 
    // Defines a null pointer to represent an empty/default address.
}

DnsService::~DnsService()
{
    // 空析构函数，没有需要特殊释放的资源。 
    // Empty destructor, as no special resources need cleanup.
}

void DnsService::AddHost(const std::string& host)
{
    std::lock_guard<std::mutex> lk(lock_); // 使用互斥锁保护共享资源 // Lock to protect shared data (thread safety).
    auto iter = hosts_info_.find(host);   // 查找主机是否已存在 // Check if host already exists.
    if (iter != hosts_info_.end())        // 如果找到，直接返回 // If host exists, return.
    {
        return;
    }
    hosts_info_[host] = std::vector<InetAddressPtr>(); // 初始化为空列表 // Initialize an empty list for the host.
}

InetAddressPtr DnsService::GetHostAddress(const std::string &host, int index)
{
    std::lock_guard<std::mutex> lk(lock_); // 加锁保护共享资源 // Lock to ensure thread safety.
    auto iter = hosts_info_.find(host);    // 查找主机地址列表 // Look up the list of addresses for the host.
    if (iter != hosts_info_.end())         // 如果找到主机 // If the host is found:
    {
        auto list = iter->second;          // 获取地址列表 // Retrieve the address list.
        if (list.size() > 0)               // 如果列表非空 // If the list is not empty:
        {
            return list[index % list.size()]; // 通过索引取模来循环访问列表中的地址 // Use modulo to access the address cyclically.
        }
    }
    return inet_address_null; // 返回默认空地址 // Return the default null address.
}

std::vector<InetAddressPtr> DnsService::GetHostAddress(const std::string &host)
{
    std::lock_guard<std::mutex> lk(lock_); // 加锁 // Lock for thread safety.
    auto iter = hosts_info_.find(host);    // 查找主机 // Look up the host.
    if (iter != hosts_info_.end())         // 如果找到主机 // If host exists:
    {
        return iter->second;               // 返回主机地址列表 // Return the list of addresses.
    }
    return std::vector<InetAddressPtr>();  // 返回空列表 // Return an empty list.
}

void DnsService::UpdateHost(const std::string &host, std::vector<InetAddressPtr> &list)
{
    std::lock_guard<std::mutex> lk(lock_); // 加锁，保护更新操作 // Lock for thread safety.
    hosts_info_[host].swap(list);          // 更新主机地址列表，用新列表替换旧列表 // Swap the list with the new data.
}

std::unordered_map<std::string, std::vector<InetAddressPtr>> DnsService::GetHosts()
{
    std::lock_guard<std::mutex> lk(lock_); // 加锁保护共享数据 // Lock for thread safety.
    return hosts_info_;                    // 返回当前所有主机和它们的地址信息 // Return all hosts and their address lists.
}

void DnsService::SetDnsServiceParam(int32_t interval, int32_t sleep, int32_t retry)
{
    interval_ = interval;  // 设置定时检查的时间间隔 // Set the interval for periodic checks.
    sleep_ = sleep;        // 设置重试之间的休眠时间 // Set sleep duration between retries.
    retry_ = retry;        // 设置重试次数 // Set retry attempts.
}

void DnsService::Start()
{
    running_ = true;  // 标记服务为运行状态 // Mark the service as running.
    thread_ = std::thread(std::bind(&DnsService::OnWork, this)); 
    // 创建线程并绑定 OnWork 函数，用于后台执行 DNS 更新任务。 
    // Start a thread that runs the OnWork function for periodic DNS updates.
}

void DnsService::Stop()
{
    running_ = false;  // 停止运行 // Set running flag to false.
    if (thread_.joinable()) 
    {
        thread_.join(); // 等待线程结束 // Wait for the thread to finish.
    }
}

void DnsService::GetHostInfo(const std::string &host, std::vector<InetAddressPtr> &list)
{
    struct addrinfo ainfo, *res; // 定义 addrinfo 结构体用于 DNS 查询结果 // Struct for DNS query results.
    memset(&ainfo, 0x00, sizeof(ainfo)); // 清空结构体 // Clear the structure.
    ainfo.ai_family = AF_UNSPEC; // 支持 IPv4 和 IPv6 // Support both IPv4 and IPv6.
    ainfo.ai_flags = AI_PASSIVE; // 用于服务器绑定 // Passive socket for server binding.
    ainfo.ai_socktype = SOCK_DGRAM; // 数据报套接字 // Use UDP socket type.

    auto ret = ::getaddrinfo(host.c_str(), nullptr, &ainfo, &res); 
    // 调用 getaddrinfo 获取主机地址信息 // Get address information.
    if (ret == -1 || res == nullptr) // 查询失败直接返回 // Return if no results.
    {
        return;
    }

    struct addrinfo *rp = res;
    for (; rp != nullptr; rp = rp->ai_next) // 遍历查询结果 // Iterate over the results.
    {
        InetAddressPtr peeraddr = std::make_shared<InetAddress>(); // 创建地址对象 // Create an address object.
        if (rp->ai_family == AF_INET) // 处理 IPv4 地址 // Handle IPv4.
        {
            char ip[16] = {0,};
            struct sockaddr_in *saddr = (struct sockaddr_in*)rp->ai_addr;
            ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(saddr->sin_port));
        }
        else if (rp->ai_family == AF_INET6) // 处理 IPv6 地址 // Handle IPv6.
        {
            char ip[INET6_ADDRSTRLEN] = {0,};
            struct sockaddr_in6 *saddr = (struct sockaddr_in6*)rp->ai_addr;
            ::inet_ntop(AF_INET6, &(saddr->sin6_addr), ip, sizeof(ip));
            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(saddr->sin6_port));
            peeraddr->SetIsIPV6(true);
        }
        list.push_back(peeraddr); // 将结果添加到列表 // Add result to the list.
    }
}

void DnsService::OnWork()
{
    while (running_) // 循环直到服务停止 // Loop until service stops.
    {
        auto host_infos = GetHosts(); // 获取当前的主机列表 // Get all current hosts.
        for (auto &host : host_infos) 
        {
            for (int i = 0; i < retry_; i++) // 尝试多次获取主机信息 // Retry multiple times.
            {
                std::vector<InetAddressPtr> list;
                GetHostInfo(host.first, list); // 获取主机地址信息 // Fetch host info.
                if (list.size() > 0) 
                {
                    UpdateHost(host.first, list); // 更新主机信息 // Update host list.
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_)); // 等待重试间隔 // Wait before retry.
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_)); // 等待下一次检查间隔 // Wait for the next interval.
    }
}
