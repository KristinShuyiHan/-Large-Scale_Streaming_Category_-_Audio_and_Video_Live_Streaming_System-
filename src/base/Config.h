#pragma once 
// 防止头文件被多次包含 (Include guard)  
// Ensures this header file is included only once during compilation to avoid duplicate definitions.

#include "json/json.h"            // 引入 JSON 库，用于解析 JSON 文件 (For JSON parsing).
#include "NonCopyable.h"          // 自定义的基类，用于阻止拷贝操作 (Prevents copying of derived classes).
#include "Singleton.h"            // 单例模式相关的头文件 (Singleton pattern utility).
#include "Logger.h"               // 日志管理类的头文件 (Logger class for logging system).
#include "FileLog.h"              // 文件日志类，用于日志文件的写入 (Handles writing logs to files).
#include <string>                 // 引入 std::string (C++ string class).
#include <cstdint>                // 定义固定大小的整数类型 (For fixed-width integer types like uint16_t).
#include <memory>                 // 引入智能指针 std::shared_ptr (Smart pointers for memory management).
#include <mutex>                  // 引入互斥锁 std::mutex (For thread safety).
#include <unordered_map>          // 引入哈希表存储 (Key-value map for fast lookups).

namespace tmms                    // 定义命名空间 tmms (Namespace for the project).
{
    namespace base                // 命名空间 base，表示基础组件 (Sub-namespace for base components).
    {
        using std::string;        // 简化 string 类型的使用 (Alias for std::string).

        // 定义一个结构体 LogInfo，存储日志信息 (Log information).
        struct LogInfo
        {
            LogLevel level;                    // 日志级别，例如 INFO, ERROR (Logging severity level).
            std::string path;                 // 日志文件的路径 (Log file path).
            std::string name;                 // 日志文件的名称 (Log file name).
            RotateType rotate_type{kRotateNone}; // 日志的滚动类型，默认无滚动 (Log rotation type, default: none).
        };

        using LogInfoPtr = std::shared_ptr<LogInfo>; 
        // 定义智能指针类型，管理 LogInfo 的内存 (Smart pointer to manage LogInfo objects).

        // 定义一个结构体 ServiceInfo，存储服务配置信息 (Service configuration info).
        struct ServiceInfo
        {
            string addr;          // 服务地址，例如 127.0.0.1 (Service address, e.g., localhost).
            uint16_t port;        // 服务端口号，例如 8080 (Port number, e.g., 8080).
            string protocol;      // 协议类型，例如 HTTP、TCP (Protocol type, e.g., HTTP, TCP).
            string transport;     // 传输类型，例如 REST、RPC (Transport type, e.g., REST, RPC).
        };

        using ServiceInfoPtr = std::shared_ptr<ServiceInfo>;
        // 智能指针类型，管理 ServiceInfo 的内存 (Smart pointer to manage ServiceInfo objects).

        // 前向声明 (Forward declarations)  
        // 只声明类，不定义，减少依赖项 (Forward declaration to reduce compilation dependencies).
        class DomainInfo;         
        class AppInfo;

        using DomainInfoPtr = std::shared_ptr<DomainInfo>; // 智能指针，管理 DomainInfo (Smart pointer for DomainInfo).
        using AppInfoPtr = std::shared_ptr<AppInfo>;       // 智能指针，管理 AppInfo (Smart pointer for AppInfo).

        // 定义 Config 类，处理配置信息 (Configuration management class).
        class Config
        {
        public:
            Config() = default;     // 默认构造函数 (Default constructor).
            ~Config() = default;    // 默认析构函数 (Default destructor).

            bool LoadConfig(const std::string &file);
            // 加载配置文件 (Load configuration from a file).

            LogInfoPtr& GetLogInfo();
            // 获取日志信息 (Get logging configuration).

            const std::vector<ServiceInfoPtr> & GetServiceInfos();
            // 获取所有服务信息 (Get all service configurations).

            const ServiceInfoPtr &GetServiceInfo(const string &protocol,const string &transport);
            // 根据协议和传输方式获取特定服务信息 (Get specific service info based on protocol and transport).

            bool ParseServiceInfo(const Json::Value &serviceObj);
            // 解析 JSON 中的服务信息 (Parse service info from JSON object).

            AppInfoPtr GetAppInfo(const string &domain,const string &app);
            // 获取应用信息 (Retrieve application info based on domain and app name).

            DomainInfoPtr GetDomainInfo(const string &domain);
            // 获取域名信息 (Retrieve domain info).

            // 配置相关的变量 (Configuration variables).
            std::string name_;          // 名称 (Config name).
            int32_t cpu_start_{0};      // CPU 起始编号 (Starting CPU number).
            int32_t thread_nums_{1};    // 线程数量，默认 1 (Number of threads, default 1).
            int32_t cpus_{1};           // CPU 核数，默认 1 (Number of CPUs, default 1).

        private:
            bool ParseDirectory(const Json::Value &root);
            bool ParseDomainPath(const string &path);
            bool ParseDomainFile(const string &file);
            bool ParseLogInfo(const Json::Value & root);

            LogInfoPtr log_info_; // 日志信息的智能指针 (Pointer to log info).
            std::vector<ServiceInfoPtr> services_; // 服务列表 (List of services).
            std::unordered_map<std::string,DomainInfoPtr> domaininfos_; // 域名信息映射表 (Mapping of domain info).
            std::mutex lock_; // 互斥锁，保证线程安全 (Mutex for thread safety).
        };

        using ConfigPtr = std::shared_ptr<Config>; 
        // 智能指针类型，管理 Config 对象 (Smart pointer to manage Config objects).

        // 定义 ConfigMgr 类，管理 Config 实例 (Config manager using singleton pattern).
        class ConfigMgr:public NonCopyable
        {
        public:
            ConfigMgr() = default;    // 默认构造函数 (Default constructor).
            ~ConfigMgr() = default;   // 默认析构函数 (Default destructor).

            bool LoadConfig(const std::string &file);
            // 加载配置文件 (Load configuration file).

            ConfigPtr GetConfig();
            // 获取 Config 实例 (Retrieve the configuration instance).

        private:
            ConfigPtr config_; // Config 实例的智能指针 (Pointer to Config instance).
            std::mutex lock_;  // 互斥锁，保证线程安全 (Mutex for thread safety).
        };

        #define sConfigMgr tmms::base::Singleton<tmms::base::ConfigMgr>::Instance()
        // 使用单例模式获取 ConfigMgr 实例 (Macro to access ConfigMgr instance using Singleton pattern).
    }
}
