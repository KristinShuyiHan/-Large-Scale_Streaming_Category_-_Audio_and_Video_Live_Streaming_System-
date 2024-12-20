#pragma once  
// Ensures this header file is included only once during compilation to prevent multiple inclusion errors.
// 确保头文件在编译过程中只被包含一次，避免重复包含引发的错误。

#include "Logger.h"  
#include <sstream>  
// Includes the "Logger.h" file and the standard library <sstream> to handle string streams.
// 引入 "Logger.h" 头文件，并包含 <sstream> 标准库用于处理字符串流。

namespace tmms  
{  
    namespace base  
    {  
        extern Logger * g_logger;  
        // Declares an external pointer `g_logger` to a Logger object, making it globally accessible.
        // 声明一个外部指针 `g_logger`，指向 Logger 对象，允许在多个文件中访问它。

        class LogStream  
        {  
        public:  
            LogStream(Logger *loger, const char* file, int line, LogLevel l, const char *func=nullptr);  
            // Constructor: Initializes the log stream with parameters:  
            // Logger pointer, file name, line number, log level, and optional function name.  
            // 构造函数：初始化日志流，传入 Logger 指针、文件名、行号、日志级别和可选的函数名。

            ~LogStream();  
            // Destructor: Likely responsible for sending the constructed log message to the Logger when the LogStream goes out of scope.
            // 析构函数：在 LogStream 对象销毁时，负责将日志消息发送到 Logger。

            template<class T> LogStream & operator<<(const T& value)  
            {  
                stream_ << value; // Appends the value to the internal stream. Allows any type to be logged.
                return *this;     // Returns *this for chained `<<` operations (e.g., log << "value" << 123).
                // 重载运算符 `<<`：将任意类型的值追加到字符串流中，并返回自身支持链式调用。
            }  

        private:  
            std::ostringstream stream_;  // Internal string stream to accumulate log messages.
            // 内部字符串流，用于拼接和存储日志消息。

            Logger *logger_{nullptr};    // Pointer to the logger where the log message will be sent.
            // 指向 Logger 实例的指针，用于接收最终的日志消息。
        };  
    }  
}

#define LOG_TRACE   \  
    if(g_logger && tmms::base::g_logger->GetLogLevel() <= tmms::base::kTrace)   \  
        tmms::base::LogStream(tmms::base::g_logger, __FILE__, __LINE__, tmms::base::kTrace, __func__)  
// LOG_TRACE Macro:  
// Checks if the global logger exists (`g_logger`) and whether the log level is TRACE or higher.  
// If true, creates a LogStream object with the logger pointer, file name (__FILE__), line number (__LINE__),  
// log level (kTrace), and function name (__func__).  
// LOG_TRACE 宏：检查全局 logger 是否存在，以及当前日志级别是否为 TRACE 或更高级别。  
// 如果满足条件，创建一个 LogStream 对象，传入 Logger 指针、文件名、行号、日志级别和函数名。

#define LOG_DEBUG  \  
    if(g_logger && tmms::base::g_logger->GetLogLevel() <= tmms::base::kDebug)   \  
        tmms::base::LogStream(tmms::base::g_logger, __FILE__, __LINE__, tmms::base::kDebug, __func__)  
// LOG_DEBUG Macro: Similar to LOG_TRACE but for DEBUG level logs.  
// LOG_DEBUG 宏：类似于 LOG_TRACE，但用于 DEBUG 级别日志。

#define LOG_INFO   \  
    if(g_logger && tmms::base::g_logger->GetLogLevel() <= tmms::base::kInfo)   \  
        tmms::base::LogStream(tmms::base::g_logger, __FILE__, __LINE__, tmms::base::kInfo)  
// LOG_INFO Macro: Logs messages at INFO level without including the function name.  
// LOG_INFO 宏：在 INFO 级别记录日志，不包括函数名。

#define LOG_WARN   \  
    tmms::base::LogStream(tmms::base::g_logger, __FILE__, __LINE__, tmms::base::kWarn)  
// LOG_WARN Macro: Directly creates a WARN level log. No log level checks.  
// LOG_WARN 宏：直接创建 WARN 级别的日志，不检查日志级别。

#define LOG_ERROR  tmms::base::LogStream(tmms::base::g_logger, __FILE__, __LINE__, tmms::base::kError)  
// LOG_ERROR Macro: Directly logs messages at ERROR level.  
// LOG_ERROR 宏：直接记录 ERROR 级别的日志。

