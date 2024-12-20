#include "Logger.h"  // 引入头文件，声明了 Logger 类和相关函数。
                      // Include the header file, which declares the Logger class and its related functions.

#include <iostream>   // 引入 iostream 库，用于输出到控制台。
                      // Include the iostream library for console output.

using namespace tmms::base;  // 使用命名空间 tmms::base，简化代码书写。
                             // Use the namespace `tmms::base` to simplify code writing.

/// @brief Logger 类的构造函数，接收一个 FileLog 指针来初始化日志对象。
/// @brief Constructor of the Logger class, initializes the log object using a `FileLog` pointer.
Logger::Logger(const FileLogPtr &log)  // `const FileLogPtr &log` 表示传入的日志对象是一个共享指针引用，不能被修改。
: log_(log)                            // 使用初始化列表将传入的日志对象赋值给成员变量 `log_`。
{                                     
    // 空函数体，构造时只完成成员变量初始化。
    // Empty function body, initialization is done in the initializer list.
}

/// @brief 设置日志的级别。
/// @brief Set the log level of the Logger.
void Logger::SetLogLevel(const LogLevel &level)  
{                                    
    level_ = level;  // 将传入的日志级别 `level` 赋值给成员变量 `level_`。
                     // Assign the incoming log level `level` to the member variable `level_`.
}

/// @brief 获取当前的日志级别。
/// @brief Retrieve the current log level.
LogLevel Logger::GetLogLevel() const  
{
    return level_;  // 返回当前日志级别 `level_`。
                    // Return the current log level `level_`.
}

/// @brief 将日志信息写入到日志文件或控制台。
/// @brief Write the log message to a file or print it to the console.
void Logger::Write(const std::string &msg)  // `const std::string &msg` 表示传入的日志信息是常量字符串引用，避免拷贝。
{
    if (log_)  // 判断成员变量 `log_` 是否存在，即是否有有效的日志对象。
               // Check if the member variable `log_` is valid (non-null).
    {
        log_->WriteLog(msg);  // 调用 `FileLog` 类的 WriteLog 函数，将日志写入文件。
                              // Call the `WriteLog` function of the `FileLog` class to write the log message to a file.
    }
    else  // 如果没有传入有效的日志对象，则将日志信息输出到控制台。
          // If there is no valid log object, print the log message to the console.
    {
        std::cout << msg;  // 使用 `std::cout` 将日志信息输出到标准输出（控制台）。
                           // Use `std::cout` to output the log message to the console.
    }
}
