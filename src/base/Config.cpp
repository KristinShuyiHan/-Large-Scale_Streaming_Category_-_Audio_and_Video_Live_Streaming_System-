#include "Config.h"          // 包含Config类的定义文件。
#include "LogStream.h"       // 提供日志输出功能。
#include "DomainInfo.h"      // 包含域信息相关的类。
#include "AppInfo.h"         // 包含应用程序信息类。
#include <fstream>           // 提供文件输入输出操作。
#include <sys/stat.h>        // 用于获取文件或目录的状态信息。
#include <unistd.h>          // 提供POSIX系统API，比如`stat`函数。
#include <dirent.h>          // 用于遍历目录中的文件和子目录。

using namespace tmms::base;  // 简化命名空间引用。

namespace 
{
    static ServiceInfoPtr service_info_nullptr; // 定义一个静态空指针，用来返回未找到服务时的默认值。
}

bool Config::LoadConfig(const std::string &file)
{
    LOG_DEBUG << "config file:" << file; // 输出加载的配置文件路径。
    Json::Value root;                    // 根节点，存储JSON文件内容。
    Json::CharReaderBuilder reader;      // JSON解析器。
    std::ifstream in(file);              // 打开文件作为输入流。
    std::string err;                     // 用于保存解析错误信息。

    auto ok = Json::parseFromStream(reader, in, &root, &err); // 解析JSON文件内容。
    if (!ok) // 如果解析失败
    {
        LOG_ERROR << "config file:" << file << " parse error.err:" << err; // 输出错误信息。
        return false; // 返回false表示加载失败。
    }

    // 解析"name"字段，配置文件的名称
    Json::Value nameObj = root["name"];
    if (!nameObj.isNull()) 
    {
        name_ = nameObj.asString(); // 将"name"字段转换为字符串并赋值给name_。
    }

    // 解析"cpu_start"字段，表示CPU启动参数
    Json::Value cpusObj = root["cpu_start"];
    if (!cpusObj.isNull()) 
    {
        cpu_start_ = cpusObj.asInt(); // 将"cpu_start"字段转换为整数。
    }

    // 解析"cpus"字段，表示CPU核心数量
    Json::Value cpus1Obj = root["cpus"];
    if (!cpus1Obj.isNull()) 
    {
        cpus_ = cpus1Obj.asInt(); // 将"cpus"字段赋值给cpus_。
    }

    // 解析"threads"字段，表示线程数量
    Json::Value threadsObj = root["threads"];
    if (!threadsObj.isNull()) 
    {
        thread_nums_ = threadsObj.asInt(); // 将"threads"字段赋值给线程数量变量。
    }

    // 解析"Log"字段，加载日志配置信息
    Json::Value logObj = root["log"];
    if (!logObj.isNull()) 
    {
        ParseLogInfo(logObj); // 调用ParseLogInfo函数解析日志配置。
    }

    // 解析"services"字段，加载服务配置信息
    if (!ParseServiceInfo(root["services"])) 
    {
        return false; // 如果服务解析失败，返回false。
    }

    // 解析"directory"字段，加载目录信息
    ParseDirectory(root["directory"]);
    return true; // 加载成功返回true。
}

bool Config::ParseLogInfo(const Json::Value &root)
{
    log_info_ = std::make_shared<LogInfo>(); // 创建一个新的LogInfo对象并使用智能指针管理。

    // 解析"level"字段，设置日志级别
    Json::Value levelObj = root["level"];
    if (!levelObj.isNull()) 
    {
        std::string level = levelObj.asString();
        if (level == "TRACE") log_info_->level = kTrace;
        else if (level == "DEBUG") log_info_->level = kDebug;
        else if (level == "INFO") log_info_->level = kInfo;
        else if (level == "WARN") log_info_->level = kWarn;
        else if (level == "ERROR") log_info_->level = kError;
    }

    // 解析"path"字段，设置日志文件路径
    Json::Value pathObj = root["path"];
    if (!pathObj.isNull()) 
    {
        log_info_->path = pathObj.asString(); // 赋值日志文件路径。
    }

    // 解析"name"字段，设置日志名称
    Json::Value nameObj = root["name"];
    if (!nameObj.isNull()) 
    {
        log_info_->name = nameObj.asString(); // 赋值日志名称。
    }

    // 解析"rotate"字段，设置日志分割类型（天或小时）
    Json::Value rtObj = root["rotate"];
    if (!rtObj.isNull()) 
    {
        std::string rt = rtObj.asString();
        if (rt == "DAY") log_info_->rotate_type = kRotateDay;
        else if (rt == "HOUR") log_info_->rotate_type = kRotateHour;
    }
    return true; // 日志解析成功。
}

bool Config::ParseServiceInfo(const Json::Value &serviceObj)
{
    if (serviceObj.isNull()) 
    {
        LOG_ERROR << " config no service section!";
        return false; // 如果"services"字段为空，返回false。
    }

    if (!serviceObj.isArray()) 
    {
        LOG_ERROR << " service section type is not array!";
        return false; // 如果"services"不是数组类型，返回false。
    }

    // 遍历"services"数组，解析每个服务的配置信息
    for (auto const &s : serviceObj) 
    {
        ServiceInfoPtr sinfo = std::make_shared<ServiceInfo>();

        // 获取服务的地址、端口、协议和传输类型
        sinfo->addr = s.get("addr", "0.0.0.0").asString();
        sinfo->port = s.get("port", "0").asInt();
        sinfo->protocol = s.get("protocol", "rtmp").asString();
        sinfo->transport = s.get("transport", "tcp").asString();

        // 输出服务配置信息
        LOG_INFO << "service info addr:" << sinfo->addr
                 << " port:" << sinfo->port
                 << " protocol:" << sinfo->protocol
                 << " transport:" << sinfo->transport;

        services_.emplace_back(sinfo); // 将服务信息添加到services_向量中。
    }
    return true;
}
