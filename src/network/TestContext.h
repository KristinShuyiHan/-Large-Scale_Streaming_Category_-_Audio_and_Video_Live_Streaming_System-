#pragma once  
// **中文**: 防止头文件被重复包含，保证编译过程中只会包含一次这个头文件。
// **英文**: Ensures the header file is included only once during compilation to avoid redefinition errors.
#include "network/net/TcpConnection.h"  
#include <string>
#include <functional>
#include <memory>  
// **中文**: 引入所需的头文件，提供基础功能支持。
// `TcpConnection.h`: 用于TCP连接类的声明，代表与客户端/服务端的网络连接。
// `<string>`: 提供字符串类 std::string。
// `<functional>`: 提供 `std::function`，用于存储可调用的回调函数。
// `<memory>`: 提供智能指针的支持，比如 std::shared_ptr。
// **英文**: Include required headers:
// - `TcpConnection.h`: Declares the TCP connection class, representing client-server connections.
// - `<string>`: Provides the std::string class for string manipulation.
// - `<functional>`: Enables std::function to store callback functions.
// - `<memory>`: Enables smart pointers like std::shared_ptr for memory management.
namespace tmms  
{  
    namespace network  
    {  
        using TestMessageCallback = std::function<void(const TcpConnectionPtr&, const std::string&)>;  
        // **中文**: 定义一个类型别名 `TestMessageCallback`，表示回调函数的类型。
        // 参数：1. `TcpConnectionPtr` (TCP连接对象智能指针)  
        //       2. `const std::string&` (接收到的消息字符串)  
        // 用途：当接收到完整消息时，触发这个回调函数进行消息处理。
        // **英文**: Defines a type alias `TestMessageCallback` for callback functions.
        // Parameters: 
        //   1. `TcpConnectionPtr` (a shared pointer to a TCP connection object).
        //   2. `const std::string&` (a received message as a string).
        // Purpose: Trigger this callback function when a complete message is received.
class TestContext  
{  
    enum  
    {  
        kTestContextHeader = 1,  
        kTestContextBody = 2,  
    };  
    // **中文**: 定义状态枚举值，用于解析消息时区分当前处理的是消息头还是消息体。
    // - `kTestContextHeader` (1): 当前解析的是消息头部。
    // - `kTestContextBody` (2): 当前解析的是消息体内容。
    // **英文**: Defines enum values for the parsing state:
    // - `kTestContextHeader` (1): Parsing the message header.
    // - `kTestContextBody` (2): Parsing the message body.
public:  
    TestContext(const TcpConnectionPtr &con);  
    ~TestContext() = default;  
    // **中文**: 构造函数，接收一个TCP连接对象的智能指针并初始化。  
    // `~TestContext()`：默认析构函数，用于释放资源（当前没有显式资源释放需求）。  
    // **英文**: Constructor that accepts a shared pointer to a TCP connection object and initializes the class.  
    // `~TestContext()` is the default destructor for resource cleanup (no explicit resource needs here).
    int ParseMessage(MsgBuffer & buf);  
    // **中文**: 用于解析从网络缓冲区接收到的数据。  
    // 参数：`MsgBuffer &buf`（消息缓冲区对象）。  
    // 返回值：  
    // - 解析成功返回消息状态码（如0或1）。  
    // - 解析失败时返回错误码。  
    // **英文**: Parses the data received from the network buffer.  
    // Parameter: `MsgBuffer &buf` (a message buffer object).  
    // Return Value:  
    // - Returns a status code on success (e.g., 0 or 1).  
    // - Returns an error code on failure.
    void SetTestMessageCallback(const TestMessageCallback &cb);  
    void SetTestMessageCallback(TestMessageCallback &&cb);  
    // **中文**: 设置回调函数，处理解析后的消息。  
    // - `const TestMessageCallback &cb`: 传入左值回调函数。  
    // - `TestMessageCallback &&cb`: 传入右值回调函数（支持移动语义，提高性能）。  
    // **英文**: Sets the callback function to process parsed messages.  
    // - `const TestMessageCallback &cb`: Accepts an lvalue callback function.  
    // - `TestMessageCallback &&cb`: Accepts an rvalue callback function for move semantics (improves performance).
private:  
    TcpConnectionPtr connection_;  
    // **中文**: 保存与客户端的TCP连接对象的智能指针，用于网络通信。  
    // **英文**: Holds a shared pointer to the TCP connection object for network communication.

    int state_{kTestContextHeader};  
    // **中文**: 当前解析状态，初始化为 `kTestContextHeader`（解析消息头部）。  
    // **英文**: Current parsing state, initialized to `kTestContextHeader` (parsing the message header).

    int32_t message_length_{0};  
    // **中文**: 存储当前消息的长度，默认初始化为0。  
    // **英文**: Stores the length of the current message, initialized to 0.

    std::string message_body_;  
    // **中文**: 用于存储接收到的消息体内容。  
    // **英文**: Holds the content of the received message body.

    TestMessageCallback cb_;  
    // **中文**: 回调函数，用于处理解析后的消息内容。  
    // **英文**: Callback function to process the parsed message content.
};
    };
}


// 代码功能总结
// TestContext 类在大型C++流媒体项目中的作用是：

// 网络消息解析

// 分阶段解析网络数据流：先解析消息头（获取消息长度等信息），然后解析消息体。
// 回调函数执行

// 使用 TestMessageCallback 回调，在消息解析完成后，将消息内容传递给外部处理逻辑。
// 与 TcpConnection 集成

// 通过 TcpConnectionPtr 维护和客户端的连接，并从网络缓冲区读取数据。
// 在大型流媒体项目中的作用
// 在一个大型流媒体项目中，比如实现一个千万级直播系统，TestContext 的作用包括：

// 底层网络通信处理：

// 解析客户端发送的消息，例如心跳包、视频流控制信息等。
// 通过回调函数处理解析后的数据，如传递给上层业务逻辑（播放、推流）。
// 高效消息管理：

// 使用 state_ 控制解析流程，减少错误和性能开销。
// 提供回调接口，实现消息的异步处理，提高并发性能。
// 模块化设计：

// TestContext 提供了良好的封装，便于与其他模块（如视频流处理模块、用户管理模块）协同工作。
// 举例：

// 场景1：用户发送一条“推流开始”命令，服务器通过 ParseMessage 解析消息，调用回调函数通知业务层启动视频流传输。
// 场景2：实时监控消息内容，例如视频帧的传输状态，解析后触发相应的控制逻辑。
// 总结
// TestContext 类通过状态机和回调函数机制，实现了网络消息的高效解析和处理，特别适用于大规模、高并发的流媒体直播系统。在这种系统中，稳定的消息通信机制是确保低延迟、高性能的核心