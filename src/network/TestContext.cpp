// 引入头文件，包含 TestContext 类的声明及标准输入输出库。
// Include TestContext header file and standard I/O library.
#include "TestContext.h" 
#include <iostream> 

// 使用命名空间 tmms::network
// Use the namespace tmms::network for clarity and scope management.
using namespace tmms::network;

// TestContext 构造函数，初始化时传入一个 TcpConnectionPtr 智能指针。
// Constructor for TestContext. It accepts a TcpConnectionPtr as input.
TestContext::TestContext(const TcpConnectionPtr &con)
:connection_(con) // 将传入的连接指针赋值给类的成员变量 connection_
{
    // 构造函数没有其他逻辑，仅做初始化。
    // No additional logic; initializes the connection_ member variable.
}

// ParseMessage 方法用于解析从网络缓冲区 (MsgBuffer) 接收到的数据。
// The ParseMessage method parses incoming data from the network buffer (MsgBuffer).
int TestContext::ParseMessage(MsgBuffer & buf)
{
    // 循环读取缓冲区的数据，直到没有可读字节。
    // Continuously read from the buffer while there are readable bytes.
    while(buf.ReadableBytes() > 0)
    {
        // 如果当前状态为读取消息头 (Header)。
        // If the current state is reading the message header.
        if(state_ == kTestContextHeader)
        {
            // 检查缓冲区是否至少有 4 字节 (表示头部的长度)。
            // Check if there are at least 4 bytes (header size) to read.
            if(buf.ReadableBytes() >= 4)
            {
                // 读取消息的长度 (4 字节整数)。
                // Read the message length (4-byte integer).
                message_length_ = buf.ReadInt32();

                // 打印读取的消息长度，便于调试。
                // Print the message length for debugging.
                std::cout << "message_length_:" << message_length_ << std::endl;

                // 设置状态为读取消息体 (Body)。
                // Change the state to reading the message body.
                state_ = kTestContextBody;
                continue; // 跳过当前循环，重新检查缓冲区。
            }
            else 
            {
                // 如果可读字节不足 4 字节，说明数据不完整，等待下一次数据到达。
                // If there are fewer than 4 bytes, wait for more data.
                return 1;
            }
        }
        // 如果当前状态为读取消息体 (Body)。
        // If the current state is reading the message body.
        else if(state_ == kTestContextBody)
        {
            // 检查缓冲区是否有足够的数据来读取消息体。
            // Check if the buffer has enough data for the message body.
            if(buf.ReadableBytes() >= message_length_)
            {
                // 临时字符串 tmp 用于存储消息体数据。
                // Temporary string 'tmp' to store the message body data.
                std::string tmp;
                tmp.assign(buf.Peek(), message_length_); // 从缓冲区读取消息体。
                
                // 将临时字符串追加到消息体变量中。
                // Append the temporary string to the message_body_ variable.
                message_body_.append(tmp);

                // 从缓冲区中移除已读取的数据。
                // Remove the consumed bytes from the buffer.
                buf.Retrieve(message_length_);
                
                // 重置消息长度。
                // Reset the message length.
                message_length_ = 0;

                // 如果回调函数已设置，则调用回调函数，传递消息体内容。
                // If a callback function is set, invoke it with the message body.
                if(cb_)
                {
                    cb_(connection_, message_body_); // 执行回调，将消息体传递给业务逻辑。
                    message_body_.clear(); // 清空消息体，准备下一次消息解析。
                }
                
                // 将状态重置为读取消息头 (Header)。
                // Reset the state to reading the message header.
                state_ = kTestContextHeader;
            }
        }
    }
    // 返回 1 表示解析完成，数据可能不完整但未发生错误。
    // Return 1 indicating that parsing is complete; partial data may remain.
    return 1;
}

// 设置消息回调函数的常引用版本。
// Set the callback function for message handling (constant reference version).
void TestContext::SetTestMessageCallback(const TestMessageCallback &cb)
{
    cb_ = cb; // 将传入的回调函数赋值给成员变量 cb_。
}

// 设置消息回调函数的右值引用版本，使用 std::move 移动赋值以提高性能。
// Set the callback function using rvalue reference (std::move for efficiency).
void TestContext::SetTestMessageCallback(TestMessageCallback &&cb)
{
    cb_ = std::move(cb); // 移动传入的回调函数到成员变量 cb_。
}


// TCP的 客户端往服务器发一个消息，来解析这个消息
// 项目场景：在一个 C++大型流媒体项目 中，这个类可能位于 网络层，用于接收客户端或服务器发送的数据包，对消息进行 头部与消息体的解析，并在解析完成后触发回调，传递完整的消息到上层业务处理逻辑。
// 代码功能总结与解析
// 解析网络消息（消息头与消息体）：

// 代码将数据分成 消息头 (4 字节) 和 消息体。
// 消息头存储消息体的长度，解析完成后，进入下一个状态读取消息体。
// 状态机设计：
// kTestContextHeader: 读取消息头。
// kTestContextBody: 读取消息体。
// 回调机制：

// 提供了 SetTestMessageCallback 方法，允许业务层注册回调函数。
// 当解析完整的消息体后，调用回调函数，将消息传递给业务逻辑。
// 代码应用场景：
// 在 C++大型流媒体项目 中，这段代码可以用于：

// 底层网络通信模块：解析客户端发送的 控制命令、媒体数据或心跳包。
// 消息分发机制：通过回调机制将完整的消息体交给上层 业务逻辑层 处理。
// 高性能直播系统：解析音视频流数据的分片，并进行后续的播放或转码操作。
// 代码语法解析与举例
// buf.ReadInt32()

// 语法：从缓冲区读取 4 字节整数（假设缓冲区格式为网络字节序）。
// 例子：
// 如果缓冲区包含 0x0000000A，则 ReadInt32() 返回 10。
// 状态机设计

// 通过 state_ 控制解析逻辑，两个状态：
// kTestContextHeader：等待读取消息头。
// kTestContextBody：等待读取消息体。
// 例子：
// 如果接收到数据流 \x00\x00\x00\x05hello：
// 读取消息头（长度为 5）。
// 读取消息体（hello），触发回调。
// cb_ 回调函数

// 语法：使用函数对象（std::function）存储回调函数。
// 例子：
// cpp
// Copy code
// TestContext context(connection);
// context.SetTestMessageCallback([](const TcpConnectionPtr& conn, const std::string& message) {
//     std::cout << "Received message: " << message << std::endl;
// });
// 代码在项目中的作用总结
// 在一个 C++大型流媒体项目 中，这段代码的主要作用是：

// 底层网络数据解析：

// 将网络传输的数据按消息头和消息体进行分割，保证数据完整性。
// 高效消息传递：

// 通过 回调机制 将解析后的消息体传递给业务层，减少耦合度。
// 流媒体控制：

// 可用于解析 控制指令、心跳包 或 媒体数据分片，如解析直播流中的视频关键帧。
