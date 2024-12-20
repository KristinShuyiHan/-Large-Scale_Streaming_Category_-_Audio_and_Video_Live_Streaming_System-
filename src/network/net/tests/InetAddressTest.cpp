#include "network/base/InetAddress.h" // 引入自定义的网络地址类头文件
#include <string>   // 引入字符串库，用于处理字符串类型
#include <iostream> // 引入输入输出流库，用于标准输入输出

using namespace tmms::network; // 使用命名空间，简化类名的使用

int main(int argc, const char **argv) // 主函数，接受命令行参数
{
    std::string host; // 定义一个字符串变量 `host`，用于存储用户输入的主机地址

    while (std::cin >> host) // 通过标准输入读取主机名（例如 www.example.com 或 192.168.1.1）
    {
        InetAddress addr(host); // 创建 InetAddress 对象，传入用户输入的主机名
        /*
        InetAddress 是一个自定义类，用于解析网络地址（IP 地址和端口等）。
        它会根据传入的主机名，提取相应的 IP 地址，并提供相关网络信息。
        */

        // 输出解析后的网络信息
        std::cout << "host:" << host << std::endl           // 输出用户输入的主机名
                  << " ip:" << addr.IP() << std::endl       // 调用 `IP()` 方法，输出主机对应的 IP 地址
                  << " port:" << addr.Port() << std::endl   // 调用 `Port()` 方法，输出端口号（默认为 0）
                  << " lan:" << addr.IsLanIp() << std::endl // 调用 `IsLanIp()` 判断是否为局域网 IP 地址（如 192.168.x.x）
                  << " wan:" << addr.IsWanIp() << std::endl // 调用 `IsWanIp()` 判断是否为广域网 IP 地址
                  << " loop:" << addr.IsLoopbackIp() << std::endl; // 调用 `IsLoopbackIp()` 判断是否为回环地址（如 127.0.0.1）

        /*
        示例：
        输入：127.0.0.1
        输出：
        host: 127.0.0.1
         ip: 127.0.0.1
         port: 0
         lan: 0       // 不是局域网地址
         wan: 0       // 不是广域网地址
         loop: 1      // 是回环地址
        */

    }
    return 0; // 程序正常结束，返回 0
}
