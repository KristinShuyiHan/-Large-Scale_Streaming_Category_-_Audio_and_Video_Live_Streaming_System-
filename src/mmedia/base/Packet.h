#pragma once  
// 防止头文件被重复包含，确保编译时只会引入一次该文件。  
// Prevents multiple inclusions of this file during compilation. 

#include <string>    // 引入字符串类
#include <memory>    // 引入智能指针 (std::shared_ptr)
#include <cstring>   // 用于内存操作
#include <cstdint>   // 提供固定宽度的整数类型，例如 int32_t, uint64_t

namespace tmms    // 定义一个名为 `tmms` 的顶级命名空间 (顶层模块名)
{
    namespace mm  // 定义子命名空间 `mm`，表示 multimedia (多媒体)
    {
        // 定义枚举，用于标识不同的 packet 类型 (音视频和元数据包)
        enum
        {
            kPacketTypeVideo = 1,     // 视频数据包
            kPacketTypeAudio = 2,     // 音频数据包
            kPacketTypeMeta = 4,      // 元数据包
            kPacketTypeMeta3 = 8,     // 特定元数据包类型
            kFrameTypeKeyFrame = 16,  // 视频关键帧 (比如I帧)
            kFrameTypeIDR = 32,       // 视频IDR帧 (Intra-coded Picture)
            kPacketTypeUnknowed = 255 // 未知类型的包
        };

        class Packet;   // 提前声明 `Packet` 类
        using PacketPtr = std::shared_ptr<Packet>;  
        // 定义智能指针别名，便于管理 `Packet` 对象的生命周期  
        // `std::shared_ptr` 自动管理对象内存，防止内存泄漏。

#pragma pack(push) 
#pragma pack(1)  
        // 修改结构体字节对齐方式为 1 字节对齐 (确保内存紧凑，提高传输效率)
        class Packet
        {
        public:
            // 构造函数，初始化包的容量 (Capacity)
            Packet(int32_t size)
                : capacity_(size)
            {
            }
            ~Packet() {} // 空析构函数

            // 工厂方法：创建一个新的 Packet 实例
            static PacketPtr NewPacket(int32_t size);

            // 判断是否是视频包
            bool IsVideo() const
            {
                return (type_ & kPacketTypeVideo) == kPacketTypeVideo;
            }

            // 判断是否是关键帧
            bool IsKeyFrame() const
            {
                return ((type_ & kPacketTypeVideo) == kPacketTypeVideo)
                        && (type_ & kFrameTypeKeyFrame) == kFrameTypeKeyFrame;
            }

            // 判断是否是音频包
            bool IsAudio() const
            {
                return type_ == kPacketTypeAudio;
            }

            // 判断是否是元数据包
            bool IsMeta() const
            {
                return type_ == kPacketTypeMeta;
            }

            // 判断是否是Meta3类型的包
            bool IsMeta3() const
            {
                return type_ == kPacketTypeMeta3;
            }

            // 返回包的当前大小
            inline int32_t PacketSize() const
            {
                return size_;
            }

            // 计算包剩余的可用空间 (容量 - 已使用大小)
            inline int Space() const
            {
                return capacity_ - size_;
            }

            // 设置包的大小
            inline void SetPacketSize(size_t len)
            {
                size_ = len;
            }

            // 更新包的大小 (增加 len)
            inline void UpdatePacketSize(size_t len)
            {
                size_ += len;
            }

            // 设置索引值
            void SetIndex(int32_t index)
            {
                index_ = index;
            }

            // 获取索引值
            int32_t Index() const
            {
                return index_;
            }

            // 设置包的类型 (音频、视频等)
            void SetPacketType(int32_t type)
            {
                type_ = type;
            }

            // 获取包的类型
            int32_t PacketType() const
            {
                return type_;
            }

            // 设置时间戳
            void SetTimeStamp(uint64_t timestamp)
            {
                timestamp_ = timestamp;
            }

            // 获取时间戳
            uint64_t TimeStamp() const
            {
                return timestamp_;
            }

            // 获取数据区域的指针  
            // 数据区域紧跟在 Packet 对象之后  
            inline char *Data()
            {
                return (char *)this + sizeof(Packet);
            }

            // 获取扩展字段，并转换为指定类型的智能指针
            template <typename T>
            inline std::shared_ptr<T> Ext() const
            {
                return std::static_pointer_cast<T>(ext_);
            }

            // 设置扩展字段 (void类型的共享指针)
            inline void SetExt(const std::shared_ptr<void> &ext)
            {
                ext_ = ext;
            }

        private:
            int32_t type_{kPacketTypeUnknowed};  // 包类型 (默认为未知)
            uint32_t size_{0};                   // 当前包的大小
            int32_t index_{-1};                  // 包的索引值
            uint64_t timestamp_{0};              // 时间戳 (用于同步音视频数据)
            uint32_t capacity_{0};               // 包的总容量 (最大数据大小)
            std::shared_ptr<void> ext_;          // 扩展字段，用于存储额外数据
        };
#pragma pack()  // 恢复默认字节对齐方式
    }
}
// ### **代码功能及其在直播流媒体系统中的实际应用**

// ---

// ### **1. `Packet` 类的作用**  
// `Packet` 类是直播流媒体系统中的**数据单元**，用于表示视频、音频或元数据包。  
// - 它封装了**大小**、**类型**、**时间戳**以及指向实际数据的指针。  
// - **示例**：一个视频帧（I帧或关键帧）会被视为类型为 `kPacketTypeVideo` 的数据包。

// ---

// ### **2. 在直播流媒体系统中的应用场景**  
// - **网络层**：  
//   `Packet` 类可用于流媒体协议（如 **RTMP** 实时消息协议 或 **HLS** HTTP直播协议）中，负责高效地管理和传输多媒体数据包。  

// - **处理层**：  
//   - **关键帧检测** (`IsKeyFrame()`):  
//     该函数用于识别视频关键帧，这对于**快速视频定位（拖动进度条）**等任务至关重要。  
//   - **音频/视频分类** (`IsAudio()` 和 `IsVideo()`):  
//     用于区分音频和视频数据包，并进行相应处理。  

// - **应用层**：  
//   - 提供时间戳 (`TimeStamp()`) 和索引 (`Index()`) 用于**音视频同步**和数据包正确排序。  
//   - **示例**：同步播放器上的音频和视频数据，确保播放时不会出现延迟或错位。

// ---

// ### **3. 现实类比**  
// 可以把 **Packet** 看作一个**装载数据的集装箱**：  
// - **视频帧**: 类型为 `kPacketTypeVideo` 的 `Packet`。  
// - **音频帧**: 类型为 `kPacketTypeAudio` 的 `Packet`。  
// - **元数据**: 类型为 `kPacketTypeMeta` 的 `Packet`。  

// 系统读取数据包的类型，根据所载的数据进行处理，就像根据集装箱的货物决定如何运输一样。

// ---

// ### **4. 为什么这种设计有效？**  
// - **内存布局紧凑**：  
//   使用 `#pragma pack(1)` 确保结构体在内存中**紧凑排列**，减少空间浪费，优化实时流媒体传输的带宽。  
// - **可扩展性**：  
//   使用 `ext_` 智能指针允许动态附加额外数据（例如自定义头信息或统计信息），而无需修改核心类。  
//   - **示例**：为数据包添加丢包率或网络延迟的统计信息。  
// - **类型安全**：  
//   通过 `IsVideo()`、`IsAudio()` 和 `IsKeyFrame()` 等辅助函数，提供了简单可读的方法来判断数据包的类型。  
//   - **示例**：确保视频数据包与音频数据包的处理逻辑不同。

// ---

// ### **5. 在直播流媒体系统中的集成路径**  
// 在一个**高并发直播流媒体平台**（需要处理百万级用户）中，`Packet` 类的作用包括：  
// - **编码器/打包器**：  
//   将原始多媒体数据生成数据包，例如 H.264 视频编码器生成 `kPacketTypeVideo` 类型的数据包。  
// - **流媒体服务器**：  
//   通过网络将数据包传输给观众，使用协议如 RTMP 或 HLS。  
// - **播放器**：  
//   消费数据包，通过时间戳和索引重建音频/视频数据并同步播放。  
// - **实时处理**：  
//   快速检查和分类数据包（例如识别关键帧以便快速解码视频）。  

// ---

// ### **文件路径**  
// **`/Users/shuyihan/Downloads/tmms/src/mmedia/base/Packet.h`**  

// ---

// ### **总结**  
// `Packet` 类是处理直播流媒体系统中**多媒体数据**的核心组件。它提供了一种结构化的方式来管理音频、视频和元数据包，同时保持了灵活性，允许扩展其他功能。在一个**大型直播系统**中，它可以无缝集成到**网络层**、**处理层**和**应用层**，确保音视频数据的**实时传输**、**同步播放** 和**高效管理**，为数百万用户提供流畅的直播体验。