#include "StringUtils.h"

using namespace tmms::base;

bool StringUtils::StartsWith(const string &s, const string &sub)
{
    if(sub.empty()) // 如果子字符串为空，返回true，因为空字符串是任何字符串的前缀。
    {
        return true;
    }
    if(s.empty()) // 如果原字符串为空，返回false。
    {
        return false;
    }
    auto len = s.size();    // 获取原字符串的长度
    auto slen = sub.size(); // 获取子字符串的长度
    if(len < slen) // 如果子字符串长度大于原字符串，必定不匹配。
    {
        return false;
    }
    return s.compare(0, slen, sub) == 0; // 从索引0开始比较，长度为slen，看是否与子字符串相同
}

bool StringUtils::EndsWith(const string &s, const string &sub)
{
    if(sub.empty()) // 如果子字符串为空，返回true。
    {
        return true;
    }
    if(s.empty()) // 如果原字符串为空，返回false。
    {
        return false;
    }
    auto len = s.size();    // 获取原字符串的长度
    auto slen = sub.size(); // 获取子字符串的长度
    if(len < slen) // 如果子字符串长度大于原字符串，必定不匹配。
    {
        return false;
    }
    return s.compare(len - slen, slen, sub) == 0; // 从末尾向前slen长度比较
}


std::string StringUtils::FilePath(const std::string &path)
{
    auto pos = path.find_last_of("/\\"); // 找到路径中最后一个"/"或"\"的位置
    if(pos != std::string::npos) // 如果找到分隔符，截取从开始到分隔符的字符串
    {
        return path.substr(0, pos);
    }
    else // 如果找不到分隔符，返回当前目录"./"
    {
        return "./";
    }
}

std::string StringUtils::FileNameExt(const std::string &path)
{
    auto pos = path.find_last_of("/\\"); // 找到最后的文件路径分隔符
    if(pos != std::string::npos)
    {
        if(pos + 1 < path.size()) // 如果分隔符后还有内容，截取文件名及扩展名
        {
            return path.substr(pos + 1);
        }
    }
    return path; // 如果没有分隔符，直接返回原字符串
}

std::string StringUtils::FileName(const std::string &path)
{
    string file_name = FileNameExt(path); // 调用上一个函数，先获取完整文件名
    auto pos = file_name.find_last_of("."); // 找到文件名中的最后一个"."
    if(pos != std::string::npos)
    {
        if(pos != 0) // 如果"."不在开头，截取去掉扩展名的部分
        {
            return file_name.substr(0, pos);
        }
    }
    return file_name; // 如果没有"."，直接返回文件名
}

std::string StringUtils::Extension(const std::string &path)
{
    string file_name = FileNameExt(path); // 先获取文件名及扩展名
    auto pos = file_name.find_last_of("."); // 找到最后一个"."
    if(pos != std::string::npos)
    {
        if(pos != 0 && pos + 1 < file_name.size()) // 确保"."后还有字符
        {
            return file_name.substr(pos + 1);
        }
    }
    return std::string(); // 如果没有"."，返回空字符串
}


std::vector<std::string> StringUtils::SplitString(const string &s,const string &delimiter)
{
    if(delimiter.empty()) // 如果分隔符为空，返回空结果
    {
        return std::vector<std::string>{};
    }
    std::vector<std::string> result;
    size_t last = 0; // 起始位置
    size_t next = 0;
    while((next = s.find(delimiter, last)) != std::string::npos) // 循环查找分隔符
    {
        if(next > last) // 如果有内容，截取
        {
            result.emplace_back(s.substr(last, next - last));
        }
        else 
        {
            result.emplace_back("");
        }
        last = next + delimiter.size(); // 更新起始位置
    }
    if(last < s.size()) // 添加最后一段内容
    {
        result.emplace_back(s.substr(last));
    }
    return result;
}


std::vector<std::string> StringUtils::SplitStringWithFSM(const string &s,const char delimiter)
{
    enum
    {
        kStateInit = 0,
        kStateNormal = 1,
        kStateDelimiter = 2,
        kStateEnd = 3,
    };
    std::vector<std::string> result;
    int state = kStateInit;
    std::string tmp;
    state = kStateNormal;
    for(int pos = 0; pos < s.size();) // 遍历字符串
    {
        if(state == kStateNormal)
        {
            if(s.at(pos) == delimiter) // 遇到分隔符，切换状态
            {
                state = kStateDelimiter;
                continue;
            }
            tmp.push_back(s.at(pos));
            pos ++;
        }
        else if(state == kStateDelimiter) // 分隔符状态，保存之前的内容
        {
            result.push_back(tmp);
            tmp.clear();
            state = kStateNormal;
            pos++;
        }
    }
    if(!tmp.empty()) // 保存最后一段内容
    {
        result.push_back(tmp);
    }
    return result;
}


// 路径解析：文件路径、文件名提取用于存储日志、配置、视频片段等。
// 输入预处理：分割字符串处理如配置文件、流媒体数据流中的分隔数据。
// 高效工具类：通过封装字符串操作，简化代码，减少重复工作。