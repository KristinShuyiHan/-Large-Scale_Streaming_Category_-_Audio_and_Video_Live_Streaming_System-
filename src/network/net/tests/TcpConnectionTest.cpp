#include "network/net/Acceptor.h" 
#include "network/net/EventLoop.h" 
#include "network/net/EventLoopThread.h" 
#include "network/net/TcpConnection.h" 
#include <iostream>

// 引入网络相关类和标准输入输出库，用于实现网络服务器功能。
// Includes necessary network headers and the standard I/O library for building a server.

using namespace tmms::network;
EventLoopThread eventloop_thread;
// 创建一个独立的事件循环线程，用于管理事件循环。
// Creates a separate thread for handling the event loop.

std::thread th;
// 定义一个标准线程，但暂时没有使用。
// Defines a standard thread, though it’s not used in this code.

const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
// 定义一个固定的 HTTP 响应，用于向客户端发送简单的 HTTP 响应。
// Defines a fixed HTTP response to send back to the client.

int main(int argc, const char **argv) {
    eventloop_thread.Run();
    // 启动事件循环线程，使其开始运行事件循环。
    // Starts the event loop thread, enabling it to process events.

    EventLoop *loop = eventloop_thread.Loop();
    // 获取事件循环对象，用于注册和管理事件。
    // Retrieves the event loop object for managing events.

    if (loop) { // 检查事件循环是否有效 // Checks if the event loop is valid.
        std::vector<TcpConnectionPtr> list;
        // 定义一个向量，用于存储所有活动的 TCP 连接。
        // Defines a vector to store all active TCP connections.

        InetAddress server("192.168.1.200:34444");
        // 创建一个 InetAddress 对象，指定服务器监听的地址和端口。
        // Creates an InetAddress object specifying the server’s listening address and port.

        std::shared_ptr<Acceptor> acceptor = std::make_shared<Acceptor>(loop, server);
        // 创建一个 Acceptor 对象，用于接受新的连接，并绑定到事件循环。
        // Creates an Acceptor object to handle new connections, bound to the event loop.

        acceptor->SetAcceptCallback([&loop, &server, &list](int fd, const InetAddress &addr) {
            // 为 acceptor 设置接受连接的回调函数，当有新连接时触发。
            // Sets a callback for accepting connections, triggered when a new connection is made.

            std::cout << "host:" << addr.ToIpPort() << std::endl;
            // 打印连接的客户端地址和端口。
            // Prints the connected client’s address and port.

            TcpConnectionPtr connection = std::make_shared<TcpConnection>(loop, fd, server, addr);
            // 创建一个新的 TcpConnection 对象，表示新的客户端连接。
            // Creates a new TcpConnection object representing the client connection.

            connection->SetRecvMsgCallback([](const TcpConnectionPtr &con, MsgBuffer &buf) {
                // 设置接收消息的回调，当有数据到达时触发。
                // Sets a callback for handling received messages, triggered when data arrives.

                std::cout << "recv msg:" << buf.Peek() << std::endl;
                // 打印收到的数据内容。
                // Prints the received message content.

                buf.RetrieveAll();
                // 清空接收缓冲区，表示消息已处理。
                // Clears the buffer, indicating the message is processed.

                con->Send(http_response, strlen(http_response));
                // 发送 HTTP 响应给客户端。
                // Sends the HTTP response to the client.
            });

            connection->SetWriteCompleteCallback([&loop](const TcpConnectionPtr &con) {
                // 设置写入完成回调，当数据成功发送后触发。
                // Sets a callback for write completion, triggered after data is successfully sent.

                std::cout << "write complete host:" << con->PeerAddr().ToIpPort() << std::endl;
                // 打印写入完成的客户端地址。
                // Prints the client address for which the write is complete.

                loop->DelEvent(con);
                // 从事件循环中移除该连接。
                // Removes the connection from the event loop.

                con->ForceClose();
                // 强制关闭连接。
                // Forcefully closes the connection.
            });

            list.push_back(connection);
            // 将连接对象加入活动连接列表。
            // Adds the connection object to the active connection list.

            loop->AddEvent(connection);
            // 将连接添加到事件循环中进行管理。
            // Adds the connection to the event loop for management.
        });

        acceptor->Start();
        // 启动 acceptor 开始监听新的连接。
        // Starts the acceptor to listen for new connections.

        while (1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // 主线程进入无限循环，每隔 1 秒休眠一次。
            // Main thread enters an infinite loop, sleeping for 1 second at a time.
        }
    }
    return 0;
    // 结束程序，返回 0 表示成功。
    // Ends the program, returning 0 to indicate success.
}
