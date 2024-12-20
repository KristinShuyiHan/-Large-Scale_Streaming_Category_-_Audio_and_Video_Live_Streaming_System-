#include "network/base/InetAddress.h"  // 引入网络地址类的头文件
#include "network/base/SocketOpt.h"    // 引入Socket操作类的头文件
#include <iostream>                    // 引入标准输入输出流

using namespace tmms::network;  // 使用自定义网络命名空间，简化类名使用

// 测试客户端函数
void TestClient()
{
    // 1. 创建一个非阻塞TCP套接字 (AF_INET表示IPv4协议)
    int sock = SocketOpt::CreateNonblockingTcpSocket(AF_INET);
    if (sock < 0) // 如果创建失败，返回错误
    {
        std::cerr << "socket failed.sock:" << sock << " errno:" << errno << std::endl;
        // 输出错误信息并退出函数
        return;
    }

    // 2. 定义目标服务器的IP地址和端口号 (192.168.1.200:34444)
    InetAddress server("192.168.1.200:34444");

    // 3. 封装套接字操作类 (封装了socket的常用操作)
    SocketOpt opt(sock);
    opt.SetNonBlocking(false); // 将套接字设置为**阻塞模式** (参数false表示阻塞)

    // 4. 连接到服务器
    auto ret = opt.Connect(server);

    // 5. 输出连接结果和状态信息
    std::cout << "Connect ret:" << ret << " errno:" << errno << std::endl
              << " local:" << opt.GetLocalAddr()->ToIpPort() << std::endl  // 获取本地地址和端口
              << " peer:" << opt.GetPeerAddr()->ToIpPort() << std::endl    // 获取对端（服务器）地址和端口
              << std::endl;
}

// 测试服务器函数
void TestServer()
{
    // 1. 创建一个非阻塞TCP套接字 (AF_INET表示IPv4协议)
    int sock = SocketOpt::CreateNonblockingTcpSocket(AF_INET);
    if (sock < 0) // 如果创建失败，返回错误
    {
        std::cerr << "socket failed.sock:" << sock << " errno:" << errno << std::endl;
        // 输出错误信息并退出函数
        return;
    }

    // 2. 定义服务器绑定的IP地址和端口号 (0.0.0.0:34444 表示监听所有网卡的34444端口)
    InetAddress server("0.0.0.0:34444");

    // 3. 封装套接字操作类 (封装了socket的常用操作)
    SocketOpt opt(sock);
    opt.SetNonBlocking(false); // 将套接字设置为**阻塞模式** (参数false表示阻塞)

    // 4. 绑定服务器地址 (将套接字和IP地址绑定)
    opt.BindAddress(server);

    // 5. 监听客户端连接 (将套接字设置为监听状态)
    opt.Listen();

    // 6. 接收客户端的连接请求
    InetAddress addr;            // 用于存储客户端的地址信息
    auto ns = opt.Accept(&addr); // 接受一个连接，并返回新生成的套接字

    // 7. 输出连接接受结果和客户端地址信息
    std::cout << "Accept ret:" << ns << " errno:" << errno << std::endl
              << " addr:" << addr.ToIpPort() << std::endl  // 获取客户端的IP和端口信息
              << std::endl;
}

int main(int argc, const char **argv)
{
    // 主程序执行服务器测试函数
    TestServer();
    return 0; // 程序正常退出
}
