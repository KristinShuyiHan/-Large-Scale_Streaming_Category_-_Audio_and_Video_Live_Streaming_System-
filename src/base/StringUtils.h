#pragma once  
// #pragma once 是一个编译指令，确保头文件只被包含一次，避免重复定义。
// #pragma once ensures the header file is included only once, preventing duplicate definitions.

#include <string>   // 引入C++标准库中的字符串类 std::string  
#include <vector>   // 引入C++标准库中的向量类 std::vector，用于存储分割后的字符串。
// Include C++ standard libraries for string and vector functionality.

namespace tmms    // 定义一个名为 tmms 的命名空间，避免命名冲突。
// Define the `tmms` namespace to avoid naming conflicts.
{
    namespace base  // `base` 子命名空间，表示基础工具类所在的模块。
// Sub-namespace `base`, indicating foundational utility tools.
    {
        using std::string;  // 使用 std::string 作为字符串类型，简化代码编写。
// Bring `std::string` into scope to simplify the usage of the string class.

        // 定义 StringUtils 类，包含字符串处理的静态函数
        // Define the `StringUtils` class containing static string utility functions.
        class StringUtils
        {
        public:
            // 检查字符串 s 是否以 sub 开头
            // Check if the string `s` starts with the substring `sub`.
            static bool StartsWith(const string &s, const string &sub);

            // 检查字符串 s 是否以 sub 结尾
            // Check if the string `s` ends with the substring `sub`.
            static bool EndsWith(const string &s, const string &sub);

            // 从完整路径中提取文件路径（不包含文件名）
            // Extract the file path (excluding the file name) from a full path.
            static std::string FilePath(const std::string &path);

            // 从完整路径中提取文件名和扩展名
            // Extract the file name with its extension from the full path.
            static std::string FileNameExt(const std::string &path);

            // 从完整路径中提取文件名（不包含扩展名）
            // Extract the file name (excluding the extension) from the full path.
            static std::string FileName(const std::string &path);

            // 从完整路径中提取文件扩展名
            // Extract the file extension from the full path.
            static std::string Extension(const std::string &path);

            // 将字符串 s 按照指定的分隔符 delimiter 进行分割，并返回一个字符串向量
            // Split the string `s` using the specified delimiter and return a vector of strings.
            static std::vector<std::string> SplitString(const string &s, const string &delimiter);

            // 使用有限状态机（FSM）方法将字符串 s 按单字符分隔符 delimiter 进行分割
            // Split the string `s` using a single character delimiter with a Finite State Machine (FSM) approach.
            static std::vector<std::string> SplitStringWithFSM(const string &s, const char delimiter);
        };
    }
}



// 细节分析与功能举例
// StartsWith 和 EndsWith

// 功能：检查字符串是否以某个子字符串开头或结尾。
// 举例：
// cpp
// Copy code
// std::string filename = "example.mp4";
// bool starts = StringUtils::StartsWith(filename, "ex"); // 返回 true
// bool ends = StringUtils::EndsWith(filename, ".mp4");   // 返回 true
// FilePath、FileNameExt、FileName 和 Extension

// 功能：处理文件路径，提取文件路径、文件名和扩展名。
// 示例：
// cpp
// Copy code
// std::string fullPath = "/home/user/videos/example.mp4";

// std::string path = StringUtils::FilePath(fullPath);        // "/home/user/videos/"
// std::string fileNameExt = StringUtils::FileNameExt(fullPath); // "example.mp4"
// std::string fileName = StringUtils::FileName(fullPath);    // "example"
// std::string extension = StringUtils::Extension(fullPath);  // ".mp4"
// SplitString 和 SplitStringWithFSM

// 功能：将字符串按分隔符拆分成多个部分。
// 区别：
// SplitString：适合简单的分隔符（如 ","）。
// SplitStringWithFSM：使用有限状态机，效率更高，适合单字符分隔符（如 ','）。
// 示例：
// cpp
// Copy code
// std::string input = "video1,video2,video3";
// auto result1 = StringUtils::SplitString(input, ",");
// // result1: {"video1", "video2", "video3"}

// auto result2 = StringUtils::SplitStringWithFSM(input, ',');
// // result2: {"video1", "video2", "video3"}
// 在大型流媒体项目中的作用
// 路径管理与文件处理
// 在直播系统中，可能需要频繁处理视频文件路径、日志文件路径、配置文件路径等：

// 提取文件名用于显示：FileName()。
// 获取文件后缀进行格式判断：Extension()（如 .mp4、.flv）。
// 示例：
// 当上传直播流时，系统提取文件名 video123.mp4 并存储到数据库。

// 字符串分割与解析

// 分割 RTMP 协议地址、日志行、配置文件内容。
// SplitStringWithFSM 高效处理大规模日志文件中的字符串分割。
// 示例：

// cpp
// Copy code
// std::string rtmpUrl = "rtmp://server/live/stream123";
// auto parts = StringUtils::SplitString(rtmpUrl, "/");
// // parts: {"rtmp:", "", "server", "live", "stream123"}
// 高效工具类复用
// 通过 StringUtils，项目中的不同模块可以复用统一的字符串处理逻辑，避免代码重复，提升项目的可维护性。

// 总结
// StringUtils.h 是一个封装了字符串操作的工具类，提供了文件路径解析、字符串分割等常用功能。在C++大型流媒体项目中，它起到辅助字符串处理、路径管理和数据解析的作用，是底层模块中非常关键的工具组件。