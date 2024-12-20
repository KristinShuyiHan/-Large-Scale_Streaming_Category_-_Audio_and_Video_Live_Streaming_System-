#include "FileLog.h"        // 包含 FileLog 类的声明文件，定义了日志文件的操作接口。
// 引入必要的系统头文件：  
#include <fcntl.h>          // 提供文件操作相关函数（如 open）。  
#include <unistd.h>         // 提供低级 I/O 操作函数（如 write, close, lseek）。  
#include <sys/stat.h>       // 用于设置文件的权限模式。  
#include <sys/types.h>      // 提供类型定义（如 pid_t）。  
#include <iostream>         // 用于输入输出操作。  

using namespace tmms::base; // 使用 tmms::base 命名空间，避免重复书写。

// -----------------------  Open 方法 -----------------------
bool FileLog::Open(const std::string &filePath)  // 打开日志文件的方法，参数为文件路径。  
{
    file_path_ = filePath;  // 将文件路径存储到类成员变量 `file_path_` 中。  
    // 使用系统调用 open 打开文件：
    // O_CREAT：如果文件不存在，则创建文件  
    // O_APPEND：每次写入数据都追加到文件末尾  
    // O_WRONLY：只写模式  
    // DEFFILEMODE：默认文件权限（比如 644 权限）。  
    int fd = ::open(file_path_.c_str(), O_CREAT | O_APPEND | O_WRONLY, DEFFILEMODE);
    if (fd < 0)  // 如果返回的文件描述符 fd 小于 0，说明打开文件失败。
    {
        std::cout << "open file log error.path:" << filePath << std::endl; // 输出错误信息。
        return false;  // 返回 false，表示打开失败。  
    }
    fd_ = fd;  // 将打开的文件描述符存储到类的成员变量 `fd_` 中。  
    return true;  // 返回 true，表示成功打开文件。  
}

// -----------------------  WriteLog 方法 -----------------------
size_t FileLog::WriteLog(const std::string &msg)  // 写入日志内容的方法，参数为字符串 `msg`。  
{
    int fd = fd_ == -1 ? 1 : fd_;  // 如果 fd_ 未初始化（-1），默认使用标准输出文件描述符（1）。  
    // 调用系统函数 write 将日志消息写入文件：  
    // 参数：fd - 文件描述符，msg.data() - 字符串数据指针，msg.size() - 数据大小。  
    return ::write(fd, msg.data(), msg.size());
}

// -----------------------  Rotate 方法 -----------------------
void FileLog::Rotate(const std::string &file)  // 进行日志文件的切换/重命名操作。  
{
    if (file_path_.empty())  // 如果当前文件路径为空，则直接返回，不进行操作。
    {
        return;
    }
    // 调用 rename 系统函数，将当前日志文件重命名为指定文件名：  
    // 参数：file_path_ - 旧文件名，file - 新文件名。  
    int ret = ::rename(file_path_.c_str(), file.c_str());
    if (ret != 0)  // 如果重命名失败，输出错误信息。
    {
        std::cerr << "rename failed. old:" << file_path_ << " new:" << file;
        return;
    }
    // 重命名完成后，重新打开原始日志文件（用于继续写入新的日志数据）。  
    int fd = ::open(file_path_.c_str(), O_CREAT | O_APPEND | O_WRONLY, DEFFILEMODE);
    if (fd < 0)  // 如果打开失败，输出错误信息。  
    {
        std::cout << "open file log error.path:" << file << std::endl;
        return;
    }
    // 使用 dup2 将新的文件描述符替换旧的文件描述符 `fd_`，实现文件切换。
    ::dup2(fd, fd_);
    close(fd);  // 关闭临时打开的文件描述符。
}

// -----------------------  SetRotate 方法 -----------------------
void FileLog::SetRotate(RotateType type)  // 设置日志文件的切换类型（例如按时间、大小等）。  
{
    rotate_type_ = type;  // 将传入的 `type` 存储到成员变量 `rotate_type_` 中。  
}

// -----------------------  GetRotateType 方法 -----------------------
RotateType FileLog::GetRotateType() const  // 获取当前的日志文件切换类型。  
{
    return rotate_type_;  // 返回成员变量 `rotate_type_` 的值。  
}

// -----------------------  FileSize 方法 -----------------------
int64_t FileLog::FileSize() const  // 获取当前日志文件的大小。  
{
    // 使用 lseek64 系统调用移动文件指针到文件末尾，并返回文件大小。  
    return ::lseek64(fd_, 0, SEEK_END);  
}

// -----------------------  FilePath 方法 -----------------------
std::string FileLog::FilePath() const  // 获取当前日志文件的路径。  
{
    return file_path_;  // 返回成员变量 `file_path_` 的值。  
}
