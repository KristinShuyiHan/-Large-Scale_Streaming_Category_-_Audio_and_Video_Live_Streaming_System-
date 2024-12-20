#include "LogStream.h"       // 引入LogStream类的头文件  
#include "TTime.h"           // 引入时间相关工具类，提供ISO时间字符串  
#include <string.h>          // C风格字符串处理函数，如strrchr  
#include <unistd.h>          // 提供系统调用，如syscall  
#include <sys/syscall.h>     // 包含SYS_gettid用于获取线程ID  
#include <iostream>          // 标准输入输出流库

using namespace tmms::base;

// 声明全局Logger指针，用于记录日志输出的目标
// Declaring a global Logger pointer to handle log output destination.
Logger *tmms::base::g_logger = nullptr;  

// 声明一个线程局部变量，存储线程ID，每个线程拥有独立的thread_id值。
// Static thread-local variable to store the thread ID. Each thread will have its own value.
static thread_local pid_t thread_id = 0;  

// 定义一个常量字符串数组，存储日志级别对应的字符串
// Constant array holding string representations of log levels.
const char *log_string[] = {  
    " TRACE ",  // 详细跟踪信息  
    " DEBUG ",  // 调试信息  
    " INFO ",   // 通用信息  
    " WARN ",   // 警告信息  
    " ERROR "   // 错误信息  
};

// 构造函数: 初始化LogStream对象，设置日志的格式
// Constructor: Initializes LogStream and formats the log message.
LogStream::LogStream(Logger *loger, const char* file, int line, LogLevel l, const char *func)  
: logger_(loger)  // 初始化Logger指针
{  
    // 提取文件名：strrchr用于查找最后一个'/'位置，获取文件名  
    // Extract file name by finding the last occurrence of '/' in the path.
    const char *file_name = strrchr(file, '/');  
    if(file_name)  
    {  
        file_name = file_name + 1; // 跳过'/'字符，指向文件名  
    }  
    else  
    {  
        file_name = file; // 如果没有'/'，直接使用完整路径作为文件名  
    }  

    // 添加当前时间，使用ISOTime()函数输出时间戳  
    // Append the current time using the ISOTime() function.
    stream_ << TTime::ISOTime() << " ";  

    // 如果thread_id尚未设置，调用系统调用syscall获取当前线程ID  
    // Fetch the thread ID if not already set, using the system call `SYS_gettid`.  
    if(thread_id == 0)  
    {  
        thread_id = static_cast<pid_t>(::syscall(SYS_gettid));  
    }  
    stream_ << thread_id;  // 添加线程ID到日志流  
    stream_ << log_string[l];  // 根据日志级别添加对应的字符串  

    // 添加文件名、行号到日志流，例如 [main.cpp:25]  
    // Append file name and line number, e.g., `[main.cpp:25]`.
    stream_ << "[" << file_name << ":" << line << "]";  

    // 如果提供了函数名，添加函数名到日志，例如 [main()]  
    // If the function name is provided, append it to the log, e.g., `[main()]`.  
    if(func)  
    {  
        stream_ << "[" << func << "]";  
    }  
}

// 析构函数: 完成日志的最终输出
// Destructor: Finalizes and outputs the log message.
LogStream::~LogStream()  
{  
    stream_ << "\n";  // 添加换行符以结束日志行  
    if(logger_)  // 如果logger_对象存在，将日志写入logger  
    {  
        logger_->Write(stream_.str());  // 调用Logger的Write方法输出日志  
    }  
    else  // 如果logger_为空，直接输出到控制台  
    {  
        std::cout << stream_.str() << std::endl;  // 输出到标准输出流  
    }  
}
