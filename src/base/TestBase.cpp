#include "TTime.h" // 包含TTime时间工具类的头文件
#include "StringUtils.h" // 包含StringUtils字符串工具类的头文件
#include <iostream> // 标准输入输出库
#include <thread>   // 用于线程操作
#include <chrono>   // 用于时间相关的操作

using namespace tmms::base; // 使用 tmms::base 命名空间，简化类调用

// **Function 1: TestTTime**
// This function tests time-related utilities, printing current time and milliseconds.
void TestTTime()
{
    // 输出当前时间戳，当前秒级时间，毫秒级时间
    std::cout << "now:" << tmms::base::TTime::Now() // 调用TTime类的Now()方法获取当前时间戳
              << " time:" << time(NULL) // 使用标准库time(NULL)获取当前的秒级时间
              << " now ms:" << tmms::base::TTime::NowMS() // 获取当前的毫秒级时间戳
              << std::endl;
}

// **Function 2: TestString1**
// This function tests the FSM-based string splitting utility (SplitStringWithFSM).
void TestString1()
{
    // 定义多个字符串测试用例
    const std::string str2 = "";   // 空字符串
    const std::string str3 = "a";  // 单字符字符串
    const std::string str4 = "aa;ab;ac;ad;ae;"; // 包含分隔符的字符串
    const std::string str5 = ";;;;;"; // 仅有分隔符的字符串
    char de = ';'; // 分隔符定义

    // **Test Case 1: SplitStringWithFSM with str4**
    std::vector<std::string> list = tmms::base::StringUtils::SplitStringWithFSM(str4, de);
    std::cout << "delimiter:" << de << " str4:" << str4 << " result:" << list.size() << std::endl;
    for (auto v : list) { std::cout << v << std::endl; } // 输出分割结果

    // **Test Case 2: SplitStringWithFSM with str5**
    list = tmms::base::StringUtils::SplitStringWithFSM(str5, de);
    std::cout << "delimiter:" << de << " str5:" << str5 << " result:" << list.size() << std::endl;
    for (auto v : list) { std::cout << v << std::endl; }

    // **Test Case 3: SplitStringWithFSM with str3**
    list = tmms::base::StringUtils::SplitStringWithFSM(str3, de);
    std::cout << "delimiter:" << de << " str3:" << str3 << " result:" << list.size() << std::endl;
    for (auto v : list) { std::cout << v << std::endl; }

    // **Test Case 4: SplitStringWithFSM with str2**
    list = tmms::base::StringUtils::SplitStringWithFSM(str2, de);
    std::cout << "delimiter:" << de << " str2:" << str2 << " result:" << list.size() << std::endl;
    for (auto v : list) { std::cout << v << std::endl; }
}

// **Function 3: TestString**
// This function tests multiple string utilities, including prefix, suffix checks, and basic splitting.
void TestString()
{
    // 多个测试字符串
    const std::string str = "abcdadcb";
    const std::string str1 = "aaaaaa";
    const std::string str2 = "";
    const std::string str3 = "a";
    const std::string str4 = "aa;ab;ac;ad;ae;";
    const std::string str5 = ";;;;;";
    const std::string start = "abc";
    const std::string start1 = "abca";
    const std::string start2 = "";
    const std::string de = ";";

    // 测试StartsWith函数 - 检查字符串是否以指定前缀开始
    std::cout << "start:" << start << " str:" << str << " result:" 
              << tmms::base::StringUtils::StartsWith(str, start) << std::endl;

    // 测试EndsWith函数 - 检查字符串是否以指定后缀结束
    std::cout << "end:" << start << " str:" << str << " result:" 
              << tmms::base::StringUtils::EndsWith(str, start) << std::endl;

    // 测试SplitString函数 - 基于普通分隔符的字符串分割
    std::vector<std::string> list = tmms::base::StringUtils::SplitString(str4, de);
    std::cout << "delimiter:" << de << " str4:" << str4 << " result:" << list.size() << std::endl;
    for (auto v : list) { std::cout << v << std::endl; }
}

int main(int argc, const char **argv)
{
    TestString1(); // 调用字符串测试函数
    return 0;
}



// SplitString 与 SplitStringWithFSM 的区别
// SplitString:

// 基于简单的字符分割。每次遇到分隔符时拆分字符串，适用于小规模、简单场景。
// Example: "aa;ab;ac;" 用 ; 分割，输出结果为 ["aa", "ab", "ac"]。
// SplitStringWithFSM (Finite State Machine):

// 使用有限状态机实现字符串分割，适合更复杂的场景，如跳过特定字符或多字符分隔符。
// 更高效，尤其在处理大字符串时。
// 代码在大型流媒体项目中的作用
// 时间戳处理 (TTime):

// 应用层：精确记录直播流的时间戳，进行音视频同步、延迟统计。
// 底层：性能监控，如统计网络请求、任务执行时间。
// 字符串处理 (StringUtils):

// 配置解析：将直播配置文件（如IP列表、推流信息）拆分成易读的格式。
// 日志处理：将日志内容分割为独立条目，便于分析。
// 数据清洗：拆分或格式化字符串数据，用于视频流元数据处理。
// 任务调度 (TestTask):

// 虽然注释掉，但代码展示了如何使用异步任务调度，适用于定时任务如心跳包发送、流媒体重试逻辑。