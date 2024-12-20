// 这段代码主要用于实现 RTMP（Real-Time Messaging Protocol）握手机制，确保客户端和服务器之间的安全连接，尤其适用于大型流媒体系统，如千万级并发直播平台中的实时通信模块。



#include "RtmpHandShake.h" // 引入 RTMP 握手的头文件
#include "mmedia/base/MMediaLog.h" // 用于日志记录功能
#include "base/TTime.h" // 时间管理相关工具
#include <cstdint> // 提供固定宽度的整数类型，如 uint8_t
#include <random> // 随机数生成器，用于生成随机数据


#if OPENSSL_VERSION_NUMBER > 0x10100000L
// 如果 OpenSSL 版本大于 1.1.0，使用新的 HMAC 接口
#define HMAC_setup(ctx, key, len) ctx = HMAC_CTX_new(); HMAC_Init_ex(ctx, key, len, EVP_sha256(), 0)
#define HMAC_crunch(ctx, buf, len) HMAC_Update(ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen) HMAC_Final(ctx, dig, &dlen); HMAC_CTX_free(ctx)
#else
// 否则，使用旧版 HMAC 接口
#define HMAC_setup(ctx, key, len) HMAC_CTX_init(&ctx); HMAC_Init_ex(&ctx, key, len, EVP_sha256(), 0)
#define HMAC_crunch(ctx, buf, len) HMAC_Update(&ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen) HMAC_Final(&ctx, dig, &dlen); HMAC_CTX_cleanup(&ctx)
#endif


namespace
{
    static const unsigned char rtmp_server_ver[4] = { 0x0D, 0x0E, 0x0A, 0x0D }; // RTMP 服务器版本号
    static const unsigned char rtmp_client_ver[4] = { 0x0C, 0x00, 0x0D, 0x0E }; // RTMP 客户端版本号


#define PLAYER_KEY_OPEN_PART_LEN 30   ///< 客户端部分密钥长度
/** 客户端密钥 */
static const uint8_t rtmp_player_key[] = {
    'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
    'F', 'l', 'a', 's', 'h', ' ', 'P', 'l', 'a', 'y', 'e', 'r', ' ', '0', '0', '1',
    // 额外的 32 字节随机数（部分密钥）
    0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02,
    0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8,
    0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};


    #define SERVER_KEY_OPEN_PART_LEN 36   ///< length of partial key used for first server digest signing
    /** Key used for RTMP server digest signing */
    static const uint8_t rtmp_server_key[] = {
        'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
        'F', 'l', 'a', 's', 'h', ' ', 'M', 'e', 'd', 'i', 'a', ' ',
        'S', 'e', 'r', 'v', 'e', 'r', ' ', '0', '0', '1',

        0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02,
        0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8,
        0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
    };

    void CalculateDigest(const uint8_t *src, int len, int gap,const uint8_t *key, int keylen, uint8_t *dst)
{
    uint32_t digestLen = 0;
    #if OPENSSL_VERSION_NUMBER > 0x10100000L    
    HMAC_CTX *ctx; // 新版本 OpenSSL 上下文指针
    #else
    HMAC_CTX ctx; // 旧版本 OpenSSL 上下文
    #endif    
    HMAC_setup(ctx, key, keylen); // 设置 HMAC 上下文，初始化密钥
    
    if (gap <= 0)
    {
        HMAC_crunch(ctx, src, len); // 如果 gap 为 0，直接计算整个数据的 HMAC
    }        
    else
    {                     
        HMAC_crunch(ctx, src, gap); // 计算数据的前半部分
        HMAC_crunch(ctx, src + gap + SHA256_DIGEST_LENGTH, len - gap - SHA256_DIGEST_LENGTH); // 跳过指定 gap 区域
    }
    HMAC_finish(ctx, dst, digestLen); // 完成 HMAC 计算，输出签名到 dst
}

bool VerifyDigest(uint8_t *buf, int digest_pos,const uint8_t *key, size_t keyLen)
{
    uint8_t digest[SHA256_DIGEST_LENGTH];
    CalculateDigest(buf, 1536, digest_pos, key, keyLen, digest); // 计算期望的签名

    return memcmp(&buf[digest_pos], digest, SHA256_DIGEST_LENGTH) == 0; // 比较两个签名是否一致
}


   int32_t GetDigestOffset(const uint8_t *buf, int off, int mod_val)
{
    uint32_t offset = 0;
    const uint8_t *ptr = reinterpret_cast<const uint8_t*>(buf + off);
    uint32_t res;

    offset = ptr[0] + ptr[1] + ptr[2] + ptr[3]; // 计算字节和作为偏移量
    res = (offset % mod_val) + (off + 4); // 取模后偏移量
    return res;
}


}

using namespace tmms::mm;

RtmpHandShake::RtmpHandShake(const TcpConnectionPtr &conn,bool client)
:connection_(conn),is_client_(client)
{
}
// Purpose（作用）：

// 构造函数，初始化握手对象，接收网络连接指针 TcpConnectionPtr 和客户端标志。


void RtmpHandShake::Start()
{
    CreateC1S1(); // 创建握手数据 C1（客户端）和 S1（服务器）
    if (is_client_)
    {
        state_ = kHandShakePostC0C1; // 客户端状态：发送 C1
        SendC1S1();
    }
    else 
    {
        state_ = kHandShakeWaitC0C1; // 服务器状态：等待客户端的 C0 和 C1
    }
}


uint8_t RtmpHandShake::GenRandom()  
{
    // 使用随机数生成器生成一个随机整数
    // Use a random number generator to generate a random integer.
    std::mt19937 mt{std::random_device{}()}; 
    /*
    解析/Syntax Analysis:
    - `std::mt19937` 是一种梅森旋转随机数生成器 (Mersenne Twister Engine)，生成高质量的伪随机数。
      The `std::mt19937` is a Mersenne Twister Engine used for generating high-quality pseudo-random numbers.
    - `std::random_device{}()` 用于提供随机种子，用于初始化随机数生成器。
      `std::random_device{}()` provides a random seed to initialize the random number generator.
    */

    // 生成一个在 [0, 256] 范围内的随机数  
    // Generate a random number in the range [0, 256]
    std::uniform_int_distribution<> rand(0,256); 
    /*
    解析/Syntax Analysis:
    - `std::uniform_int_distribution<>` 创建一个均匀分布的整数生成器。
      `std::uniform_int_distribution<>` creates a uniformly distributed integer generator.
    - 参数 `[0, 256]` 表示随机数的范围是从 0 到 256。
      The range `[0, 256]` means the random number is between 0 and 256.
    */

    // 返回生成的随机数的余数 (范围最终为 0-255)
    // Return the generated random number mod 256 (final range is 0-255)
    return rand(mt)%256; 
}


void RtmpHandShake::CreateC1S1()
{
    for(int i=0;i<kRtmpHandShakePacketSize+1;i++)
    {
        C1S1_[i] = GenRandom();
    }

    C1S1_[0] = '\x03';

    memset(C1S1_+1,0x00,4);

    if(!is_complex_handshake_)
    {
        memset(C1S1_+5,0x00,4);
    }
    else
    {
        auto offset = GetDigestOffset(C1S1_+1,8,728);
        uint8_t * data = C1S1_+1+offset;
        if(is_client_)
        {
            memcpy(C1S1_+5,rtmp_client_ver,4);
            CalculateDigest(C1S1_+1,kRtmpHandShakePacketSize,offset,rtmp_player_key,PLAYER_KEY_OPEN_PART_LEN,data);
        }
        else 
        {
            memcpy(C1S1_+5,rtmp_server_ver,4);
            CalculateDigest(C1S1_+1,kRtmpHandShakePacketSize,offset,rtmp_server_key,SERVER_KEY_OPEN_PART_LEN,data);            
        }
        memcpy(digest_,data,SHA256_DIGEST_LENGTH);
    }
}
void RtmpHandShake::CreateC1S1()
{
    // 遍历C1S1_缓冲区，生成随机数据并填充
    // Iterate through the C1S1_ buffer, generate random data, and fill it
    for(int i=0;i<kRtmpHandShakePacketSize+1;i++)
    {
        C1S1_[i] = GenRandom();
    }

    // 设置C1S1的第一个字节为 '\x03'，表示RTMP协议版本
    // Set the first byte of C1S1 to '\x03', representing the RTMP protocol version
    C1S1_[0] = '\x03';

    // 接下来4个字节清零，表示时间戳 (握手时可忽略)
    // The next 4 bytes are set to zero, representing the timestamp (ignored during handshake)
    memset(C1S1_+1,0x00,4);

    // 如果不使用复杂握手，继续清零后4个字节
    // If not using complex handshake, clear the next 4 bytes
    if(!is_complex_handshake_)
    {
        memset(C1S1_+5,0x00,4);
    }
    else
    {
        // 计算摘要 (Digest) 的偏移值
        // Calculate the offset for the digest
        auto offset = GetDigestOffset(C1S1_+1,8,728);  
        uint8_t * data = C1S1_+1+offset;

        if(is_client_)
        {
            // 如果是客户端，设置版本信息并计算摘要
            // If it is a client, set version info and calculate the digest
            memcpy(C1S1_+5,rtmp_client_ver,4);  
            CalculateDigest(C1S1_+1,kRtmpHandShakePacketSize,offset,rtmp_player_key,PLAYER_KEY_OPEN_PART_LEN,data);
        }
        else 
        {
            // 如果是服务端，设置版本信息并计算摘要
            // If it is a server, set version info and calculate the digest
            memcpy(C1S1_+5,rtmp_server_ver,4);
            CalculateDigest(C1S1_+1,kRtmpHandShakePacketSize,offset,rtmp_server_key,SERVER_KEY_OPEN_PART_LEN,data);            
        }

        // 复制摘要数据到digest_缓冲区
        // Copy the digest data into the digest_ buffer
        memcpy(digest_,data,SHA256_DIGEST_LENGTH);
    }
}

// 文件路径: /Users/shuyihan/Downloads/tmms/src/mmedia/rtmp/RtmpHandShake.cpp

// 函数: 发送C1/S1包，这是RTMP握手的第一步
void RtmpHandShake::SendC1S1()
{
    // 将C1S1_数据 (1537字节) 发送给对端，通过connection_发送
    connection_->Send((const char*)C1S1_, 1537);
    // 这里 C1S1_ 是一个预先准备好的数据包，包含随机数据和协议信息
}

// 函数: 创建C2/S2包，RTMP握手的响应阶段
void RtmpHandShake::CreateC2S2(const char *data, int bytes, int offset)
{
    // 初始化C2S2_数组，填充随机数据，确保握手的安全性
    for(int i = 0; i < kRtmpHandShakePacketSize; i++)  
    {
        C2S2_[i] = GenRandom(); // 调用GenRandom()生成随机字节
    }

    // 将data的前8字节拷贝到C2S2_的前8个字节位置，用于回应客户端的握手请求
    memcpy(C2S2_, data, 8);

    // 获取当前时间戳，将其拷贝到C2S2_的第4个字节位置，确保握手包时间信息
    auto timestamp = tmms::base::TTime::Now(); // 获取当前时间戳
    char *t = (char*)&timestamp;              // 将时间戳转换成字节数组
    C2S2_[3] = t[0];                          // 存储时间戳的低位字节
    C2S2_[2] = t[1];
    C2S2_[1] = t[2];
    C2S2_[0] = t[3];                          // 存储时间戳的高位字节

    // 如果握手使用复杂模式
    if (is_complex_handshake_)
    {
        uint8_t digest[32]; // 32字节的摘要
        if (is_client_)     // 如果是客户端，使用客户端密钥生成摘要
        {
            CalculateDigest((const uint8_t*)(data + offset), 32, 0, rtmp_player_key, sizeof(rtmp_player_key), digest);
        }
        else // 如果是服务器端，使用服务器密钥生成摘要
        {
            CalculateDigest((const uint8_t*)(data + offset), 32, 0, rtmp_server_key, sizeof(rtmp_server_key), digest);
        }
        // 将摘要签名插入到C2S2_的后32字节，完成安全验证
        CalculateDigest(C2S2_, kRtmpHandShakePacketSize - 32, 0, digest, 32, &C2S2_[kRtmpHandShakePacketSize - 32]);
    }
}

// 函数: 发送C2/S2包，响应握手请求
void RtmpHandShake::SendC2S2()
{
    // 将C2S2_ (1536字节) 发送给对端，完成握手响应阶段
    connection_->Send((const char*)C2S2_, kRtmpHandShakePacketSize);
}

// 函数: 校验接收到的C2/S2包
bool RtmpHandShake::CheckC2S2(const char *data, int bytes)
{
    // 这里只是一个示例实现，当前直接返回true，表示校验通过
    return true;
}


// 函数: 完成RTMP握手的状态机逻辑，根据状态不同执行不同握手阶段
int32_t RtmpHandShake::HandShake(MsgBuffer &buf)
{
    switch (state_)  // 根据当前握手状态执行不同逻辑
    {
        case kHandShakeWaitC0C1: // 等待接收C0C1包
        {
            // 如果数据不足1537字节，无法读取C0C1，返回1等待数据
            if (buf.ReadableBytes() < 1537)
            {
                return 1;
            }

            // 记录接收C0C1包的日志
            RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ", Recv C0C1.\n";

            // 校验C1S1的合法性
            auto offset = CheckC1S1(buf.Peek(), 1537);
            if (offset >= 0)  // 如果校验通过
            {
                CreateC2S2(buf.Peek() + 1, 1536, offset); // 创建C2S2包
                buf.Retrieve(1537); // 从buffer中移除已经读取的C0C1数据
                state_ = kHandShakePostS0S1; // 更新状态
                SendC1S1(); // 发送C1S1包
            }
            else  // 校验失败，记录日志并返回-1
            {
                RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ", check C0C1 failed.\n";
                return -1;
            }
            break;
        }

        case kHandShakeWaitC2: // 等待接收C2包
        {
            if (buf.ReadableBytes() < 1536) // 数据不足1536字节，等待更多数据
            {
                return 1;
            }
            RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ", Recv C2.\n";
            if (CheckC2S2(buf.Peek(), 1536)) // 校验C2S2数据包
            {
                buf.Retrieve(1536); // 移除C2数据
                RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ", handshake done.\n";
                state_ = kHandShakeDone; // 握手完成
                return 0;
            }
            else  // 校验失败
            {
                RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ", check C2 failed.\n";
                return -1;
            }
            break;
        }

        // 其他状态类似，处理S0S1和S2阶段的逻辑
        case kHandShakeWaitS0S1: 
        case kHandShakeWaitS2: 
        {
            // 逻辑同样是接收和校验数据包，并发送相应响应
            break;
        }
    }
    return 1; // 返回1，表示握手未完成，需要继续等待
}

void RtmpHandShake::WriteComplete()
{
    // 使用 switch 语句处理不同的握手阶段状态
    // Use a switch statement to handle the various handshake states
    switch(state_)
    {
        // **Case 1**: kHandShakePostS0S1 - 完成 S0 和 S1 数据发送后的状态
        // **Case 1**: kHandShakePostS0S1 - State after sending S0 and S1
        case kHandShakePostS0S1:
        {
            // 记录日志：当前主机完成 S0 和 S1 发送  
            // Log that the host has sent S0 and S1
            RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ",Post S0S1.\n";
            
            // 更新状态为 kHandShakePostS2，表示准备发送 S2 数据  
            // Update state to kHandShakePostS2 to prepare for sending S2
            state_ = kHandShakePostS2;

            // 调用 SendC2S2() 发送客户端 C2 和服务端 S2 握手阶段的确认数据  
            // Call SendC2S2() to send client C2 and server S2 handshake acknowledgment
            SendC2S2();
            break;
        }

        // **Case 2**: kHandShakePostS2 - 完成 S2 数据发送后的状态
        // **Case 2**: kHandShakePostS2 - State after sending S2
        case kHandShakePostS2:
        {
            // 记录日志：当前主机完成 S2 数据发送  
            // Log that the host has sent S2
            RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ",Post S2.\n";

            // 更新状态为 kHandShakeWaitC2，表示等待客户端发送 C2 数据  
            // Update state to kHandShakeWaitC2 to wait for the client to send C2
            state_ = kHandShakeWaitC2;
            break;
        }

        // **Case 3**: kHandShakePostC0C1 - 完成 C0 和 C1 数据发送后的状态
        // **Case 3**: kHandShakePostC0C1 - State after sending C0 and C1
        case kHandShakePostC0C1:
        {
            // 记录日志：当前主机完成 C0 和 C1 数据发送  
            // Log that the host has sent C0 and C1
            RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ",Post C0C1.\n";

            // 更新状态为 kHandShakeWaitS0S1，表示等待服务端发送 S0 和 S1 数据  
            // Update state to kHandShakeWaitS0S1 to wait for the server to send S0 and S1
            state_ = kHandShakeWaitS0S1;
            break;
        }

        // **Case 4**: kHandShakePostC2 - 完成 C2 数据发送后的状态
        // **Case 4**: kHandShakePostC2 - State after sending C2
        case kHandShakePostC2:
        {
            // 记录日志：当前主机完成 C2 数据发送  
            // Log that the host has sent C2
            RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ",Post C2.done\n";

            // 更新状态为 kHandShakeWaitS2，表示等待服务端发送 S2 数据  
            // Update state to kHandShakeWaitS2 to wait for the server to send S2
            state_ = kHandShakeWaitS2;
            break;
        }

        // **Case 5**: kHandShakeDoning - 正在进行握手的状态
        // **Case 5**: kHandShakeDoning - State while handshake is still in progress
        case kHandShakeDoning:
        {
            // 记录日志：当前主机完成握手最终步骤（C2 数据发送完成）  
            // Log that the host has completed the final step (C2 has been sent)
            RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << ",Post C2.done\n";

            // 更新状态为 kHandShakeDone，表示握手已完成  
            // Update state to kHandShakeDone, indicating the handshake is complete
            state_ = kHandShakeDone;
            break;
        }
    }
}
// 在大型流媒体项目中的作用
// RTMP 握手：实现客户端和服务器之间的安全连接，验证数据合法性。
// 性能优化：减少握手时延，提升直播系统的启动速度。
// 安全性：使用随机数和 HMAC 摘要防止数据伪造和中间人攻击。
