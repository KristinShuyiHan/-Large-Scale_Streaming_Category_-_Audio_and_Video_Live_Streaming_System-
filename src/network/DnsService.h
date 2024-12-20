#pragma once  // 防止头文件被重复包含，确保编译时只有一次引用。
// Prevents the header file from being included multiple times.

#include "network/base/InetAddress.h"  // 引入InetAddress类的头文件，处理网络地址相关功能。
// Includes InetAddress for handling network address functionalities.

#include "base/NonCopyable.h"  // 引入NonCopyable基类，防止拷贝和赋值操作。
// Includes NonCopyable to make the class non-copyable.

#include "base/Singleton.h"  // 引入Singleton模板，确保DnsService类是单例模式。
// Includes Singleton template for creating a single instance of DnsService.

#include <unordered_map>  // 使用unordered_map存储主机信息，提供快速查找。
// For unordered_map to store host information with fast lookup.

#include <mutex>  // 使用互斥锁(mutex)确保线程安全。
// For mutex to ensure thread-safe operations.

#include <thread>  // 引入线程库，用于后台执行DNS服务更新任务。
// Includes thread library for background thread execution.

#include <vector>  // 使用vector存储多个InetAddress对象。
// For vector to store multiple InetAddress objects.

#include <string>  // 使用string处理主机名。
// For string operations.

namespace tmms
{
    namespace network
    {
        using InetAddressPtr = std::shared_ptr<InetAddress>; 
        // 定义InetAddressPtr为指向InetAddress对象的智能指针类型，避免手动管理内存。
        // Defines InetAddressPtr as a shared pointer to InetAddress for automatic memory management.

        class DnsService: public base::NonCopyable  // 继承NonCopyable，确保DnsService不能被拷贝或赋值。
// Inherits NonCopyable to prevent copy or assignment of DnsService class.
        {
        public:
            DnsService() = default;  // 默认构造函数，不执行额外操作。
// Default constructor.

            ~DnsService();  // 析构函数，清理资源。
// Destructor to clean up resources.

            void AddHost(const std::string& host);  
            // 添加主机名到DNS服务中。
// Adds a host to the DNS service.

            InetAddressPtr GetHostAddress(const std::string &host, int index);  
            // 获取指定主机的第index个地址。
// Gets the specific address of a host at the given index.

            std::vector<InetAddressPtr> GetHostAddress(const std::string &host);  
            // 获取指定主机的所有地址。
// Retrieves all addresses for the specified host.

            void UpdateHost(const std::string &host, std::vector<InetAddressPtr> &list);  
            // 更新主机的地址列表。
// Updates the address list for a given host.

            std::unordered_map<std::string, std::vector<InetAddressPtr>> GetHosts();  
            // 获取所有主机及其地址信息。
// Returns all host information stored in the service.

            void SetDnsServiceParam(int32_t interval, int32_t sleep, int32_t retry);  
            // 设置DNS服务的参数：更新间隔、休眠时间、重试次数。
// Sets DNS service parameters: update interval, sleep time, and retry count.

            void Start();  // 启动DNS服务线程。
// Starts the DNS service in a separate thread.

            void Stop();  // 停止DNS服务线程。
// Stops the DNS service thread.

            void OnWork();  // 后台线程的主要工作函数，定期更新主机信息。
// Main function that runs in the background thread to update host information.

            static void GetHostInfo(const std::string& host, std::vector<InetAddressPtr>& list);  
            // 静态函数，获取特定主机的信息并填充到地址列表中。
// Static function to fetch host information and populate the address list.

        private:
            std::thread thread_;  // 后台线程对象，用于执行DNS服务。
// Background thread for executing DNS service work.

            bool running_{false};  // 标志DNS服务是否在运行。
// Flag to indicate if the service is running.

            std::mutex lock_;  // 互斥锁，保证hosts_info_在多线程访问时的安全性。
// Mutex to protect access to hosts_info_.

            std::unordered_map<std::string, std::vector<InetAddressPtr>> hosts_info_;  
            // 存储主机名到地址列表的映射。
// Map storing hostnames and their corresponding address lists.

            int32_t retry_{3};  // DNS解析失败的重试次数，默认为3次。
// Number of retries for DNS resolution failures.

            int32_t sleep_{200};  // 每次重试间的休眠时间（毫秒），默认为200ms。
// Sleep time (in ms) between retries.

            int32_t interval_{180 * 1000};  // 主机信息更新间隔时间，默认为180秒（180*1000毫秒）。
// Interval (in ms) to update host information, default is 180 seconds.

        };

        // 定义一个全局单例宏，获取DnsService类的唯一实例。
        // Define a global singleton macro to access the unique instance of DnsService.
        #define sDnsService tmms::base::Singleton<tmms::network::DnsService>::Instance()
    }
}
