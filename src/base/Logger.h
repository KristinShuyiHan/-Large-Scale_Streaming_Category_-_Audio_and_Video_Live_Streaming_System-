#pragma once  
// **#pragma once**: This ensures the header file is included only once during compilation  
// 防止头文件被多次包含，确保编译时只包含一次。

#include "NonCopyable.h"  
// **Include NonCopyable.h**: This ensures the class cannot be copied or assigned  
// 包含 NonCopyable.h，保证类不能被复制或赋值。

#include "FileLog.h"  
// **Include FileLog.h**: Provides the definition for FileLog-related functionality  
// 包含 FileLog.h，提供日志文件写入相关功能的定义。

#include <string>  
// **Include string**: Required to use `std::string` for log messages  
// 包含 `<string>`，用于使用 `std::string` 类型来存储日志消息。

namespace tmms  
{  
    namespace base  
    {  
        // **Define LogLevel Enum**: Enum to represent different logging levels  
        // 定义枚举 LogLevel：表示不同的日志级别。  
        enum LogLevel  
        {  
            kTrace,      // **kTrace**: Logs detailed trace/debugging information  
                         // 记录详细的调试信息。  

            kDebug,      // **kDebug**: Logs debugging-level information  
                         // 记录调试级别的信息。  

            kInfo,       // **kInfo**: Logs informational messages  
                         // 记录信息级别的消息。  

            kWarn,       // **kWarn**: Logs warning messages  
                         // 记录警告级别的消息。  

            kError,      // **kError**: Logs error messages  
                         // 记录错误级别的消息。  

            kMaxNumOfLogLevel // **kMaxNumOfLogLevel**: Represents the maximum number of log levels  
                             // 表示日志级别的最大数量。  
        };  

        // **Define Logger Class**: A class to handle logging functionality  
        // 定义 `Logger` 类：用于处理日志记录功能。  
        class Logger : public NonCopyable  
        {  
        public:  
            // **Constructor**: Takes a FileLog pointer to set up the logger  
            // 构造函数：接收一个 `FileLog` 智能指针用于初始化日志器。  
            Logger(const FileLogPtr &log);

            // **Destructor**: Default implementation  
            // 析构函数：使用默认实现。  
            ~Logger() = default;  

            // **SetLogLevel**: Sets the current logging level  
            // **参数 level**: LogLevel 枚举值，表示日志级别。  
            // 设置当前的日志级别。  
            void SetLogLevel(const LogLevel &level);  

            // **GetLogLevel**: Returns the current logging level  
            // 返回当前的日志级别。  
            LogLevel GetLogLevel() const;  

            // **Write**: Logs a message to the associated file  
            // **参数 msg**: 字符串，表示要写入日志的消息。  
            // 将日志消息写入关联的日志文件。  
            void Write(const std::string &msg);  

        private:  
            LogLevel level_{kDebug};  
            // **level_**: Stores the current logging level, initialized to `kDebug`  
            // level_：存储当前日志级别，默认初始化为 `kDebug`。  

            FileLogPtr log_;  
            // **log_**: A shared pointer to the FileLog class for file-based logging  
            // log_：一个 `FileLog` 智能指针，用于日志写入文件。  
        };  
    }  
}
