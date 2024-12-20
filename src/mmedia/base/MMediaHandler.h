#pragma once
// **#pragma once** 是一个编译指令，确保头文件只会被编译器包含一次，避免重复定义。
// "Ensures the header file is only included once during compilation to prevent redefinition issues."

#include "base/NonCopyable.h"
// 引入 **NonCopyable** 基类，确保 MMediaHandler 类的对象不能被复制。
// "Includes the NonCopyable base class to prevent copying of MMediaHandler objects."

#include "network/net/TcpConnection.h"
// 引入 TcpConnection 类，用于管理网络 TCP 连接。
// "Includes TcpConnection class for managing TCP network connections."

#include "Packet.h"
// 引入 Packet 类，用于表示网络传输的数据包。
// "Includes Packet class to represent network data packets."

#include <memory>
// 引入 **std::shared_ptr** 和 **std::unique_ptr** 等智能指针功能。
// "Includes <memory> for smart pointers like std::shared_ptr and std::unique_ptr."

namespace tmms
{
    namespace mm
    {
        // 使用命名空间 network
        using namespace network;

        // 定义一个抽象类 MMediaHandler，继承自 NonCopyable，表示此类不能被复制。
        // "Define an abstract class MMediaHandler that inherits from NonCopyable to disable copying."

        class MMediaHandler: public base::NonCopyable
        {
        public:
            // **虚函数 (Virtual Functions):**
            // 所有的函数都是纯虚函数，表示 MMediaHandler 只是一个接口，具体实现由子类提供。
            // "All functions are pure virtual, making MMediaHandler an interface where subclasses provide implementations."

            // 1. **OnNewConnection** - 当有新的 TCP 连接建立时调用。
            // 参数: const TcpConnectionPtr &conn - 指向新连接的共享指针。
            // "Called when a new TCP connection is established."
            // Parameter: const TcpConnectionPtr &conn - Shared pointer to the new connection.
            virtual void OnNewConnection(const TcpConnectionPtr &conn) = 0;

            // 2. **OnConnectionDestroy** - 当连接销毁时调用。
            // 参数: const TcpConnectionPtr &conn - 指向被销毁连接的共享指针。
            // "Called when a connection is destroyed."
            // Parameter: const TcpConnectionPtr &conn - Shared pointer to the destroyed connection.
            virtual void OnConnectionDestroy(const TcpConnectionPtr &conn) = 0;

            // 3. **OnRecv (重载函数)** - 当接收到数据包时调用。
            // (1) 传入一个左值引用的 PacketPtr。
            // 参数: const TcpConnectionPtr &conn - TCP 连接的共享指针。
            //       const PacketPtr &data - 数据包的共享指针。
            // "Called when a data packet is received (lvalue reference version)."
            virtual void OnRecv(const TcpConnectionPtr &conn, const PacketPtr &data) = 0;

            // (2) 传入一个右值引用的 PacketPtr，用于**数据移动操作**，减少拷贝开销。
            // 参数: const TcpConnectionPtr &conn - TCP 连接的共享指针。
            //       PacketPtr &&data - 数据包的右值引用。
            // "Called when a data packet is received (rvalue reference version for move semantics)."
            virtual void OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data) = 0;

            // 4. **OnActive** - 当连接变为活动状态时调用。
            // 参数: const ConnectionPtr &conn - 连接的共享指针。
            // "Called when a connection becomes active."
            virtual void OnActive(const ConnectionPtr &conn) = 0;
        };        
    }
}



// MMediaHandler 在大型流媒体项目中的作用
// 背景：在一个 C++ 实现的底层到应用层千万级直播系统中，MMediaHandler 主要负责网络数据传输层与业务逻辑模块的解耦，提供了一个统一的回调接口来管理网络连接及数据。

// 作用：

// 数据包管理：通过 OnRecv 接收网络层数据包，将数据解析后发送到业务模块进行处理（如视频编码）。
// 连接生命周期管理：通过 OnNewConnection 和 OnConnectionDestroy 处理用户连接的创建与释放。
// 状态管理：通过 OnActive 实现网络连接的心跳检查和健康状态维护。
// 性能优化：重载 OnRecv 使用右值引用，有效减少数据拷贝，提升性能。
// 总结
// MMediaHandler 是一个基类，定义了多个纯虚函数，专门用于管理网络连接的回调事件。
// 模块化设计：通过回调接口，网络模块与业务模块分离，提升了代码的可维护性和可扩展性。
// 性能与安全：通过不可拷贝继承与右值引用，确保了资源安全和性能优化。
// 实际应用：在直播系统中，可以用于处理用户连接、接收数据包、心跳检测等核心任务。
// By using MMediaHandler, developers can focus on具体的业务逻辑实现（如视频推流、用户管理等），而无需关心网络底层的细节。






