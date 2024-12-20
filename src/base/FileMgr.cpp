#include "FileMgr.h"          // 包含 FileMgr 类的定义头文件，用于日志管理。
#include "TTime.h"            // 包含 TTime 类，用于获取当前系统时间。
#include "StringUtils.h"      // 包含 StringUtils 类，用于字符串操作（例如提取文件名和路径）。
#include <sstream>            // 引入 sstream 头文件，用于拼接字符串流。




// 这段代码主要实现了 日志管理器 功能，核心内容包括：

// 日志按时间切分：根据天、小时或分钟变化，动态生成新的日志文件。
// 线程安全：通过 std::lock_guard 保护共享数据，确保多线程访问安全。
// 文件管理：使用 std::shared_ptr 管理日志对象，减少内存管理的负担。
// 工具类复用：使用 TTime 获取时间，StringUtils 进行字符串处理，封装性和可读性更强。
// 通过这段代码，日志文件可以高效地按时间切分，便于管理和分析。






using namespace tmms::base;   // 使用 tmms::base 命名空间，避免每次都写完整的前缀。
namespace
{
    static tmms::base::FileLogPtr file_log_nullptr;  // 定义一个静态空指针，表示无效的文件日志。
}

void FileMgr::OnCheck()
{
    bool day_change{false};        // 是否发生天变化
    bool hour_change{false};       // 是否发生小时变化
    bool minute_change{false};     // 是否发生分钟变化

    int year = 0, month = 0, day = -1, hour = -1, minute = 0, second = 0;
    TTime::Now(year, month, day, hour, minute, second);  // 获取当前时间，拆分成年、月、日、时、分、秒。
    if (last_day_ == -1)
    {
        last_day_ = day;    // 初始化上次记录的时间为当前时间
        last_hour_ = hour;
        last_minute_ = minute;
        last_year_ = year;
        last_month_ = month;
    }

    if (last_day_ != day) { day_change = true; }     // 判断天是否发生变化
    if (last_hour_ != hour) { hour_change = true; }  // 判断小时是否发生变化
    if (last_minute_ != minute) { minute_change = true; }  // 判断分钟是否发生变化

        if (!day_change && !hour_change && !minute_change)
    {
        return;  // 如果时间没有变化，直接返回，不做任何处理。
    }

        std::lock_guard<std::mutex> lk(lock_);   // 使用互斥锁，保护 logs_ 映射，确保多线程安全。

            for (auto &l : logs_)   // 遍历所有日志文件
    {
        if (minute_change && l.second->GetRotateType() == kRotateMinute)
        {
            RotateMinutes(l.second);  // 如果日志设置为按分钟切分，调用 RotateMinutes。
        }
        if (hour_change && l.second->GetRotateType() == kRotateHour)
        {
            RotateHours(l.second);    // 如果日志设置为按小时切分，调用 RotateHours。
        }
        if (day_change && l.second->GetRotateType() == kRotateDay)
        {
            RotateDays(l.second);     // 如果日志设置为按天切分，调用 RotateDays。
        }
    }

    last_day_ = day;      // 更新最后一次的时间记录
    last_hour_ = hour;
    last_year_ = year;
    last_month_ = month;
    last_minute_ = minute;
}

FileLogPtr FileMgr::GetFileLog(const std::string &file_name)
{
    std::lock_guard<std::mutex> lk(lock_);  // 加锁保证线程安全。
    auto iter = logs_.find(file_name);      // 在 logs_ 中查找指定日志文件。
    if (iter != logs_.end())
    {
        return iter->second;  // 如果找到，返回对应的日志对象。
    }
    FileLogPtr log = std::make_shared<FileLog>();  // 创建一个新的日志对象。
    if (!log->Open(file_name))  // 尝试打开文件，如果失败，返回空指针。
    {
        return file_log_nullptr;
    }

    logs_.emplace(file_name, log);  // 将新创建的日志对象添加到 logs_ 容器中。
    return log;
}
void FileMgr::RotateDays(const FileLogPtr &file)
{
    if (file->FileSize() > 0)  // 如果文件大小大于 0，则进行切分。
    {
        char buf[128] = {0};
        sprintf(buf, "_%04d-%02d-%02d", last_year_, last_month_, last_day_); // 格式化时间字符串。
        std::string file_path = file->FilePath();
        std::string path = StringUtils::FilePath(file_path);
        std::string file_name = StringUtils::FileName(file_path);
        std::string file_ext = StringUtils::Extension(file_path);

        std::ostringstream ss;
        ss << path << file_name << buf << "." << file_ext;  // 拼接新的文件名。
        file->Rotate(ss.str());  // 调用 Rotate 方法重命名文件。
    }
}





