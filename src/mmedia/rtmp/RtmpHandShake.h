#pragma once
// 避免头文件被重复包含。
// Prevent multiple inclusions of this header file.

#include "network/net/TcpConnection.h"  // 引入TcpConnection类，处理网络连接。
// Include TcpConnection class for handling network connections.

#include <string>     // 字符串支持
#include <memory>     // 智能指针 std::shared_ptr 支持
#include <cstdint>    // 定义固定宽度的整数类型
#include <openssl/sha.h>  // SHA256 加密函数
#include <openssl/hmac.h> // HMAC 加密函数

namespace tmms  // 命名空间 tmms (项目名)
{
    namespace mm  // 子命名空间 mm (多媒体模块)
    {
        using namespace tmms::network;  // 引入网络模块的命名空间

        // 常量: RTMP 握手包的大小 (1536 字节)
        const int kRtmpHandShakePacketSize = 1536;

        // 枚举类型: RTMP 握手状态机的各个状态
        enum RtmpHandShakeState
        {
            kHandShakeInit,           // 握手初始化状态
            kHandShakePostC0C1,       // 客户端发送 C0 和 C1
            kHandShakeWaitS0S1,       // 客户端等待服务器的 S0 和 S1
            kHandShakePostC2,         // 客户端发送 C2
            kHandShakeWaitS2,         // 客户端等待服务器的 S2
            kHandShakeDoning,         // 握手进行中

            kHandShakeWaitC0C1,       // 服务器等待客户端的 C0 和 C1
            kHandShakePostS0S1,       // 服务器发送 S0 和 S1
            kHandShakePostS2,         // 服务器发送 S2
            kHandShakeWaitC2,         // 服务器等待客户端的 C2

            kHandShakeDone            // 握手完成
        };

        // RTMP 握手类
        class RtmpHandShake
        {
        public:
            // 构造函数: 初始化握手对象
            RtmpHandShake(const TcpConnectionPtr &conn, bool client=false);
            // 参数:
            // - conn: 网络连接对象，用于客户端/服务器之间的数据传输
            // - client: 是否为客户端角色，默认 false

            ~RtmpHandShake() = default;  // 默认析构函数

            void Start();  // 开始握手流程

            // 处理握手的核心函数
            int32_t HandShake(MsgBuffer &buf);

            // 当数据写入完成后的回调
            void WriteComplete();

        private:
            uint8_t GenRandom();  // 生成随机数
            void CreateC1S1();    // 创建 C1 和 S1 数据包
            int32_t CheckC1S1(const char *data, int bytes); // 校验 C1 和 S1 数据包
            void SendC1S1();      // 发送 C1 和 S1 数据包
            void CreateC2S2(const char *data, int bytes, int offset); // 创建 C2 和 S2 数据包
            void SendC2S2();      // 发送 C2 和 S2 数据包
            bool CheckC2S2(const char *data, int bytes); // 校验 C2 和 S2 数据包

            // 网络连接对象，用于传输数据
            TcpConnectionPtr connection_;

            bool is_client_{false};  // 是否为客户端角色
            bool is_complex_handshake_{true}; // 是否为复杂握手流程

            // 存储握手数据和摘要
            uint8_t digest_[SHA256_DIGEST_LENGTH]; // SHA256摘要存储
            uint8_t C1S1_[kRtmpHandShakePacketSize+1]; // C1/S1 数据包
            uint8_t C2S2_[kRtmpHandShakePacketSize];   // C2/S2 数据包

            int32_t state_{kHandShakeInit}; // 握手的当前状态
        };

        // 智能指针别名，便于管理 RtmpHandShake 对象的生命周期
        using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;
    }
}



// 代码的核心概念解析 | Key Concepts
// RTMP 握手 (RTMP Handshake)
// RTMP 握手是 RTMP 协议中确保客户端和服务器之间安全、稳定通信的第一步。它通过三次消息交换完成:

// C0/S0: 固定格式，标识协议版本。
// C1/S1: 包含随机数据和时间戳，用于校验和同步。
// C2/S2: 基于 S1 的数据生成，用于完成握手。
// 示例场景:

// 客户端通过 SendC1S1 发送握手请求。
// 服务器通过 CheckC1S1 校验数据，并返回 S1 和 S2 数据包。
// 客户端接收后验证，通过 CheckC2S2 完成握手。
// 状态机 (State Machine)
// 使用枚举 RtmpHandShakeState 定义握手过程的各个状态。

// 客户端状态: 从 kHandShakePostC0C1 开始，逐步接收服务器数据并验证。
// 服务器状态: 从 kHandShakeWaitC0C1 开始，监听客户端数据包并响应。
// 示例:

// cpp
// Copy code
// RtmpHandShakePtr handshake = std::make_shared<RtmpHandShake>(conn, true); // 客户端
// handshake->Start(); // 开始握手流程
// 加密与随机数生成 (Encryption and Random Generation)
// 使用 SHA256 和 HMAC 进行数据校验，确保握手包的完整性。
// 使用 GenRandom 生成随机数据，增加握手的安全性。
// 智能指针 (Smart Pointers)
// 使用 std::shared_ptr 管理 RtmpHandShake 对象，确保资源安全释放，避免内存泄漏。
// 在多线程环境下，便于安全共享和传递对象。
// 在大型 C++ 流媒体项目中的应用 | Application in a Large-Scale Streaming System
// 底层通信模块:
// 该类负责 RTMP 连接的握手部分，为直播系统提供稳定可靠的连接初始化。

// 客户端: 负责推流到服务器（如主播端）。
// 服务器: 负责接收客户端流，并分发给观众端。
// 数据完整性与安全:
// 使用 SHA256 和随机数生成，保证数据包的安全传输，防止篡改。

// 高并发支持:
// 通过状态机和智能指针管理多个并发连接的握手状态，适合大规模直播平台的高并发需求。

// 代码示例:

// cpp
// Copy code
// TcpConnectionPtr connection = std::make_shared<TcpConnection>(socket);
// RtmpHandShakePtr handshake = std::make_shared<RtmpHandShake>(connection, true);
// handshake->Start(); // 开始客户端握手
// 总结 | Summary
// 这段代码实现了 RTMP 协议中的握手逻辑，是 客户端与服务器建立连接的关键步骤。
// 它通过 状态机、SHA256 加密 和 智能指针 提供了一个安全、可维护的握手机制。
// 在大型流媒体系统中，该类位于底层网络模块，为上层音视频传输提供稳定的连接支持。
// This code is a crucial part of an RTMP handshake implementation, ensuring secure and reliable communication for a live streaming system, particularly during connection initialization between the client and server.