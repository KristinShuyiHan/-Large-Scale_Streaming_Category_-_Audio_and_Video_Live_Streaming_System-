#include "network/net/Acceptor.h"         // 引入 Acceptor 类头文件，处理客户端的连接接受
#include "network/net/EventLoop.h"        // 引入 EventLoop 类头文件，管理事件循环
#include "network/net/EventLoopThread.h"  // 引入 EventLoopThread 类头文件，提供事件循环线程
#include "network/TcpServer.h"            // 引入 TcpServer 类头文件，处理 TCP 服务端逻辑
#include "network/TestContext.h"          // 引入 TestContext 类头文件，存储测试上下文数据
#include <iostream>                       // 标准输入输出库

using namespace tmms::network;            // 使用 tmms::network 命名空间
EventLoopThread eventloop_thread;         // 创建一个事件循环线程对象
std::thread th;                           // 定义一个标准线程对象
using TestContextPtr = std::shared_ptr<TestContext>; // 定义 TestContext 的共享指针类型别名

// 定义一个简单的 HTTP 响应消息字符串
const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, const char **argv) {
    // 启动事件循环线程
    eventloop_thread.Run();
    EventLoop *loop = eventloop_thread.Loop(); // 获取事件循环对象的指针

    if (loop) {
        // 配置服务器监听的 IP 和端口
        InetAddress listen("192.168.1.200:34444"); 
        TcpServer server(loop, listen); // 创建 TCP 服务器对象

        // 设置消息回调函数 - 当服务器接收到消息时触发
        server.SetMessageCallback([](const TcpConnectionPtr &con, MsgBuffer &buf) {
            // 获取连接的上下文对象，用于解析消息
            TestContextPtr context = con->GetContext<TestContext>(kNormalContext);
            context->ParseMessage(buf); // 调用解析消息方法
        });

        // 设置新连接回调函数 - 当有新客户端连接时触发
        server.SetNewConnectionCallback([&loop](const TcpConnectionPtr &con) {
            // 为新连接创建一个上下文对象
            TestContextPtr context = std::make_shared<TestContext>(con);

            // 设置测试消息回调函数，用于输出消息内容
            context->SetTestMessageCallback([](const TcpConnectionPtr &con, const std::string &msg) {
                std::cout << "message:" << msg << std::endl; // 输出收到的消息
            });

            // 将上下文对象设置到连接的上下文中
            con->SetContext(kNormalContext, context);

            // 设置写完成回调函数 - 当写操作完成时触发
            con->SetWriteCompleteCallback([&loop](const TcpConnectionPtr &con) {
                std::cout << "write complete host:" << con->PeerAddr().ToIpPort() << std::endl; // 输出对端地址
                // con->ForceClose(); // 强制关闭连接（被注释）
            });
        });

        server.Start(); // 启动服务器监听

        // 主线程持续运行，模拟服务器的运行状态
        while (1) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒暂停一次，保持主线程不退出
        }
    }

    return 0;
}




### File Path: `/Users/shuyihan/Downloads/tmms/src/network/net/tests/TcpServerTest.cpp`

---

### 中英文双语代码解析及 Inline 注释

以下代码演示了一个**基于 C++ 的 TCP 服务器**，用于处理客户端连接和消息，具有事件回调机制。代码包含 **EventLoop**、**TcpServer** 和 **TestContext** 等组件，适合用于大型流媒体项目的底层网络模块中，例如千万级直播系统的网络通信处理。

---

#### **代码及详细解释**

```cpp
#include "network/net/Acceptor.h"         // 引入 Acceptor 类头文件，处理客户端的连接接受
#include "network/net/EventLoop.h"        // 引入 EventLoop 类头文件，管理事件循环
#include "network/net/EventLoopThread.h"  // 引入 EventLoopThread 类头文件，提供事件循环线程
#include "network/TcpServer.h"            // 引入 TcpServer 类头文件，处理 TCP 服务端逻辑
#include "network/TestContext.h"          // 引入 TestContext 类头文件，存储测试上下文数据
#include <iostream>                       // 标准输入输出库

using namespace tmms::network;            // 使用 tmms::network 命名空间
EventLoopThread eventloop_thread;         // 创建一个事件循环线程对象
std::thread th;                           // 定义一个标准线程对象
using TestContextPtr = std::shared_ptr<TestContext>; // 定义 TestContext 的共享指针类型别名

// 定义一个简单的 HTTP 响应消息字符串
const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, const char **argv) {
    // 启动事件循环线程
    eventloop_thread.Run();
    EventLoop *loop = eventloop_thread.Loop(); // 获取事件循环对象的指针

    if (loop) {
        // 配置服务器监听的 IP 和端口
        InetAddress listen("192.168.1.200:34444"); 
        TcpServer server(loop, listen); // 创建 TCP 服务器对象

        // 设置消息回调函数 - 当服务器接收到消息时触发
        server.SetMessageCallback([](const TcpConnectionPtr &con, MsgBuffer &buf) {
            // 获取连接的上下文对象，用于解析消息
            TestContextPtr context = con->GetContext<TestContext>(kNormalContext);
            context->ParseMessage(buf); // 调用解析消息方法
        });

        // 设置新连接回调函数 - 当有新客户端连接时触发
        server.SetNewConnectionCallback([&loop](const TcpConnectionPtr &con) {
            // 为新连接创建一个上下文对象
            TestContextPtr context = std::make_shared<TestContext>(con);

            // 设置测试消息回调函数，用于输出消息内容
            context->SetTestMessageCallback([](const TcpConnectionPtr &con, const std::string &msg) {
                std::cout << "message:" << msg << std::endl; // 输出收到的消息
            });

            // 将上下文对象设置到连接的上下文中
            con->SetContext(kNormalContext, context);

            // 设置写完成回调函数 - 当写操作完成时触发
            con->SetWriteCompleteCallback([&loop](const TcpConnectionPtr &con) {
                std::cout << "write complete host:" << con->PeerAddr().ToIpPort() << std::endl; // 输出对端地址
                // con->ForceClose(); // 强制关闭连接（被注释）
            });
        });

        server.Start(); // 启动服务器监听

        // 主线程持续运行，模拟服务器的运行状态
        while (1) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒暂停一次，保持主线程不退出
        }
    }

    return 0;
}
// ```

// ---

// ### **核心功能解释 (中文 + 英文)**

// #### **1. 网络通信核心组件**

// - **`EventLoopThread`**  
//   - **中文**：事件循环线程类，用于运行和管理 EventLoop，使得事件循环可以在独立的线程中执行。  
//   - **英文**：Manages an EventLoop in a separate thread, enabling non-blocking event-driven operations.

// - **`TcpServer`**  
//   - **中文**：用于监听客户端连接，管理 TCP 连接，处理消息的服务器类。  
//   - **英文**：A class that listens for client connections, manages TCP connections, and handles incoming messages.

// - **`TestContext`**  
//   - **中文**：存储每个 TCP 连接的上下文信息，包含消息解析和回调函数。  
//   - **英文**：Stores context for each TCP connection, including message parsing and callback functions.

// #### **2. 回调函数的设置**

// - **`SetMessageCallback`**:  
//   - **中文**：当服务器接收到客户端消息时调用。  
//     - 示例：当客户端发送数据时，`ParseMessage` 会解析接收到的 `MsgBuffer`。  
//   - **英文**: Called when the server receives a message from the client.  
//     - Example: Invokes `ParseMessage` to parse the incoming message buffer.

// - **`SetNewConnectionCallback`**:  
//   - **中文**：当有新连接建立时调用。  
//     - 示例：为新连接分配 `TestContext` 并设置消息和写完成回调。  
//   - **英文**: Triggered when a new connection is established.  
//     - Example: Assigns a `TestContext` and sets callbacks for message handling and write completion.

// - **`SetWriteCompleteCallback`**:  
//   - **中文**：当数据发送完成时调用，通常用于日志记录或资源释放。  
//     - 示例：输出消息发送完成的主机地址。  
//   - **英文**: Invoked when a write operation completes, typically for logging or resource cleanup.

// ---

// ### **代码在大型流媒体项目中的作用**

// 在一个 **C++大型流媒体项目-从底层到应用层的千万级直播系统** 中，这段代码可以作为**底层网络通信模块**的一部分，主要作用包括：

// 1. **处理客户端连接和消息**  
//    - 接收来自客户端的消息，例如用户的推流数据、播放请求等。  
//    - 举例：  
//      - 用户推流：消息解析后，服务器可以将流媒体数据分发到其他客户端。  
//      - 观众观看请求：服务器返回视频流数据。

// 2. **事件驱动与回调机制**  
//    - 通过回调函数机制（如 `SetMessageCallback`、`SetNewConnectionCallback`），实现事件驱动式消息处理，减少阻塞，提高系统的并发性能。

// 3. **扩展性和高性能**  
//    - 使用 **EventLoop** 和 **多线程模型**，可以高效地处理大量并发连接。  
//    - 举例：  
//      - 一个直播平台的服务器，需要同时处理成千上万的客户端连接。

// 4. **上下文管理与消息解析**  
//    - 使用 **`TestContext`** 来管理每个连接的状态和数据。  
//    - 举例：  
//      - 在直播系统中，每个客户端连接都有自己的上下文，包括用户信息、当前播放状态、数据缓存等。

// 5. **HTTP 支持 (示例)**  
//    - 简单的 HTTP 响应字符串，可以扩展为支持 HTTP-FLV 流媒体传输，适用于低延迟直播场景。

// ---

// ### **总结 (Summary)**

// 1. **功能概述**：  
//    这段代码实现了一个支持回调机制的 **TCP 服务器**，用于处理客户端消息、管理连接状态，并具备上下文存储能力。

// 2. **应用场景**：  
//    在大型流媒体系统中，它可用于**实时音视频流传输**的底层网络通信模块，处理客户端的推流请求、播放请求、连接管理等任务。

// 3. **可扩展性**：  
//    - 基于 `EventLoop` 的非阻塞事件驱动设计，适合高并发场景。  
//    - 通过上下文管理和回调机制，轻松扩展复杂的业务逻辑。

// 4. **代码优势**：  
//    - 高效：事件循环 + 多线程。  
//    - 易维护：通过上下文和回调封装了每个连接的状态。  
//    - 灵活：可扩展支持 HTTP、WebSocket 等协议，适合多种实时网络应用。

// **英文总结**:  
// This code builds a **TCP server** using an event-driven, callback-based design with `EventLoop` and `TcpServer`. It is highly efficient and scalable for handling concurrent client connections, making it ideal for the network layer of a **large-scale streaming platform**. It supports message parsing, connection context management, and write completion notifications, ensuring smooth and organized handling of incoming client requests and responses.