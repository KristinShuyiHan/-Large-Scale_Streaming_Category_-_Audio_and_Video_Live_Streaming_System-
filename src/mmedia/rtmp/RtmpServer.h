#pragma once
// **中文**: #pragma once 是一个预处理指令，确保头文件只被编译一次，防止重复包含导致错误。
// **English**: `#pragma once` is a preprocessor directive to ensure the header file is included only once during compilation, preventing duplicate inclusion issues.

#include "mmedia/rtmp/RtmpHandler.h"
// **中文**: 包含处理 RTMP 消息的类定义，用于处理 RTMP 协议中的数据传输和控制。
// **English**: Includes the definition for `RtmpHandler` which handles RTMP message parsing, processing, and control logic.

#include "network/net/TcpConnection.h"
// **中文**: 包含 `TcpConnection` 类的定义，它用于管理 TCP 连接，提供读写数据的接口。
// **English**: Includes the definition of `TcpConnection`, which manages TCP connections and provides interfaces for reading and writing data.

#include "network/TcpServer.h"
// **中文**: 包含 `TcpServer` 类的定义，它是一个基础的 TCP 服务器类，提供了创建服务器和管理客户端连接的功能。
// **English**: Includes the definition of `TcpServer`, a base class that provides functionality for creating a server and managing client connections.

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;
        // **中文**: 使用 `tmms::network` 命名空间中的所有类和函数，减少代码中的冗余命名空间前缀。
        // **English**: Brings all classes and functions from the `tmms::network` namespace into the current scope, reducing the need for explicit prefixes.

        // **RtmpServer 类定义**
        class RtmpServer : public TcpServer
        {
        public:
            // **中文**: 构造函数，初始化 RTMP 服务器。参数包括事件循环、服务器监听地址和 RTMP 处理器。
            // **English**: Constructor to initialize the RTMP server. Parameters include the event loop, server listen address, and optional RTMP handler.
            RtmpServer(EventLoop *loop, const InetAddress &local, RtmpHandler *handler = nullptr);

            // **中文**: 析构函数，释放服务器资源。
            // **English**: Destructor to release server resources.
            ~RtmpServer();

            // **中文**: 启动服务器，重写基类的 `Start()` 方法。
            // **English**: Starts the server, overriding the `Start()` method from the base class.
            void Start() override;

            // **中文**: 停止服务器，重写基类的 `Stop()` 方法。
            // **English**: Stops the server, overriding the `Stop()` method from the base class.

        private:
            // **中文**: 新连接事件回调函数，当有新的客户端连接时调用。
            // **English**: Callback function for new connections, called when a new client connection is established.
            void OnNewConnection(const TcpConnectionPtr &conn);

            // **中文**: 连接销毁事件回调函数，当连接关闭时调用。
            // **English**: Callback function for destroyed connections, called when a connection is closed.
            void OnDestroyed(const TcpConnectionPtr &conn);

            // **中文**: 消息接收事件回调函数，当服务器接收到数据时调用。
            // **English**: Callback function for message reception, called when data is received from the client.
            void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf);

            // **中文**: 数据写入完成事件回调函数，当数据发送完成时调用。
            // **English**: Callback function for write completion, called when data has been successfully sent to the client.
            void OnWriteComplete(const ConnectionPtr &con);

            // **中文**: 连接激活事件回调函数，当连接处于活动状态时调用。
            // **English**: Callback function for active connections, called when the connection is active.
            void OnActive(const ConnectionPtr &conn);

            // **中文**: 指向 RTMP 处理器的指针，用于处理 RTMP 消息。
            // **English**: Pointer to an RTMP handler that processes RTMP messages.
            RtmpHandler *rtmp_handler_{nullptr};
        };
    }
}




### **中英文双语 Inline Comment 解释代码**  
**文件路径**: `/Users/shuyihan/Downloads/tmms/src/mmedia/rtmp/RtmpServer.h`  
**项目背景**: 代码可能在一个 **C++大型流媒体项目** 中用来处理 **RTMP协议** 的服务端连接和消息传输，属于直播系统的核心部分，负责处理主播推流和观众拉流的连接。

---

```cpp
#pragma once
// **中文**: #pragma once 是一个预处理指令，确保头文件只被编译一次，防止重复包含导致错误。
// **English**: `#pragma once` is a preprocessor directive to ensure the header file is included only once during compilation, preventing duplicate inclusion issues.

#include "mmedia/rtmp/RtmpHandler.h"
// **中文**: 包含处理 RTMP 消息的类定义，用于处理 RTMP 协议中的数据传输和控制。
// **English**: Includes the definition for `RtmpHandler` which handles RTMP message parsing, processing, and control logic.

#include "network/net/TcpConnection.h"
// **中文**: 包含 `TcpConnection` 类的定义，它用于管理 TCP 连接，提供读写数据的接口。
// **English**: Includes the definition of `TcpConnection`, which manages TCP connections and provides interfaces for reading and writing data.

#include "network/TcpServer.h"
// **中文**: 包含 `TcpServer` 类的定义，它是一个基础的 TCP 服务器类，提供了创建服务器和管理客户端连接的功能。
// **English**: Includes the definition of `TcpServer`, a base class that provides functionality for creating a server and managing client connections.

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;
        // **中文**: 使用 `tmms::network` 命名空间中的所有类和函数，减少代码中的冗余命名空间前缀。
        // **English**: Brings all classes and functions from the `tmms::network` namespace into the current scope, reducing the need for explicit prefixes.

        // **RtmpServer 类定义**
        class RtmpServer : public TcpServer
        {
        public:
            // **中文**: 构造函数，初始化 RTMP 服务器。参数包括事件循环、服务器监听地址和 RTMP 处理器。
            // **English**: Constructor to initialize the RTMP server. Parameters include the event loop, server listen address, and optional RTMP handler.
            RtmpServer(EventLoop *loop, const InetAddress &local, RtmpHandler *handler = nullptr);

            // **中文**: 析构函数，释放服务器资源。
            // **English**: Destructor to release server resources.
            ~RtmpServer();

            // **中文**: 启动服务器，重写基类的 `Start()` 方法。
            // **English**: Starts the server, overriding the `Start()` method from the base class.
            void Start() override;

            // **中文**: 停止服务器，重写基类的 `Stop()` 方法。
            // **English**: Stops the server, overriding the `Stop()` method from the base class.

        private:
            // **中文**: 新连接事件回调函数，当有新的客户端连接时调用。
            // **English**: Callback function for new connections, called when a new client connection is established.
            void OnNewConnection(const TcpConnectionPtr &conn);

            // **中文**: 连接销毁事件回调函数，当连接关闭时调用。
            // **English**: Callback function for destroyed connections, called when a connection is closed.
            void OnDestroyed(const TcpConnectionPtr &conn);

            // **中文**: 消息接收事件回调函数，当服务器接收到数据时调用。
            // **English**: Callback function for message reception, called when data is received from the client.
            void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf);

            // **中文**: 数据写入完成事件回调函数，当数据发送完成时调用。
            // **English**: Callback function for write completion, called when data has been successfully sent to the client.
            void OnWriteComplete(const ConnectionPtr &con);

            // **中文**: 连接激活事件回调函数，当连接处于活动状态时调用。
            // **English**: Callback function for active connections, called when the connection is active.
            void OnActive(const ConnectionPtr &conn);

            // **中文**: 指向 RTMP 处理器的指针，用于处理 RTMP 消息。
            // **English**: Pointer to an RTMP handler that processes RTMP messages.
            RtmpHandler *rtmp_handler_{nullptr};
        };
    }
}
// ```

// ---

// ### **关键技术解释和示例**

// 1. **RTMP 服务器的作用 (RtmpServer)**  
//    - **中文**: RTMP（Real-Time Messaging Protocol）是流媒体直播中常用的协议，负责将主播端推送的音视频流传输到服务器，然后再分发给观众。  
//    - **English**: RTMP (Real-Time Messaging Protocol) is widely used for streaming live audio and video. It enables a client (broadcaster) to push a live stream to the server, which then distributes it to viewers.

//    **示例**:  
//    直播平台中的 `RtmpServer` 用于接收主播推流，例如：  
//    - 主播通过 **OBS** 推流到 RTMP 服务器（地址：`rtmp://your-server/live`）。  
//    - `OnMessage` 函数会接收推流的数据包，将音视频数据解析并传送给业务层。  

// 2. **继承关系**  
//    - **中文**: `RtmpServer` 继承自 `TcpServer`，复用底层 TCP 服务器功能，如监听端口、接受连接和数据传输。  
//    - **English**: `RtmpServer` inherits from `TcpServer`, reusing its core TCP server capabilities, such as listening on a port, accepting connections, and handling data transmission.

// 3. **事件回调机制**  
//    - **中文**: 服务器与客户端之间的事件（如连接、消息、断开连接）通过回调函数进行处理，`RtmpServer` 提供了具体实现。  
//    - **English**: Events between the server and client (e.g., new connections, messages, disconnections) are handled using callback functions, and `RtmpServer` implements these for RTMP-specific behavior.

//    **示例**:
//    - `OnNewConnection`: 当观众或主播与服务器建立连接时，打印日志并初始化必要的资源。  
//      ```cpp
//      void OnNewConnection(const TcpConnectionPtr &conn) {
//          std::cout << "New connection established: " << conn->GetPeerAddress() << std::endl;
//      }
//      ```
//    - `OnMessage`: 当接收到数据时，解析 RTMP 数据包，然后交由 `RtmpHandler` 处理。  
//      ```cpp
//      void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf) {
//          rtmp_handler_->HandleMessage(conn, buf);
//      }
//      ```

// 4. **RTMP Handler (RtmpHandler)**  
//    - **中文**: `RtmpHandler` 是消息处理类，负责解析 RTMP 消息，如音视频流数据、控制消息等。  
//    - **English**: `RtmpHandler` is the message handler class that processes RTMP messages, such as video/audio streams and control commands.

// ---

// ### **在大型流媒体项目中的作用**  
// **项目背景**: 在 **千万级并发直播系统** 中，RTMP 服务器是**推流入口**，起到数据的接收和传输作用。

// 1. **接收主播推流**:  
//    - **OnNewConnection** 接收主播的连接请求。  
//    - **OnMessage** 解析主播发送的 RTMP 数据包，将音视频流发送到 **业务处理模块** 或存储到 **CDN**。

// 2. **管理观众拉流**:  
//    - 同时，RTMP 服务器也可以处理观众的连接请求，并将流数据分发给观众端。

// 3. **高并发场景下的稳定性**:  
//    - 通过继承 `TcpServer`，利用其高性能的网络 IO 处理机制，确保能够稳定地处理大量并发连接。

// 4. **可扩展性**:  
//    - 使用 **RtmpHandler** 进行 RTMP 消息的解析和业务逻辑分发，方便未来扩展新的协议或功能。

// ---

// ### **总结**  
// `RtmpServer` 类是一个 RTMP 协议的服务端实现，继承自 `TcpServer` 并通过回调函数管理连接和消息处理。它在直播系统中承担接收推流、处理 RTMP 数据包、分发数据的重要角色，配合 `RtmpHandler` 解析和转发音视频流。