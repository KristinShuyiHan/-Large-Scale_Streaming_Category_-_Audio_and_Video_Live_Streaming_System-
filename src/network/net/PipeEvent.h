#pragma once  
// 防止头文件被重复包含，避免编译错误  
// Prevent the header file from being included multiple times, avoiding compilation errors.

#include "Event.h"  
// 包含 "Event.h" 头文件，提供 Event 类的定义  
// Includes the "Event.h" file, which provides the definition for the Event class.

#include <memory>  
// 引入 std::shared_ptr 智能指针，用于自动管理对象生命周期  
// Includes <memory> for std::shared_ptr, which manages object lifetimes automatically.

namespace tmms  
{  
    namespace network  
    {  
        // 定义 PipeEvent 类，继承自 Event 类  
        // Defines the `PipeEvent` class, which inherits from the `Event` class.
        class PipeEvent: public Event  
        {  
        public:  
            // 构造函数：接受一个 EventLoop 指针作为参数  
            // Constructor: Takes a pointer to an EventLoop as an argument.  
            PipeEvent(EventLoop *loop);  

            // 析构函数：释放 PipeEvent 相关资源  
            // Destructor: Cleans up resources associated with PipeEvent.  
            ~PipeEvent();  

            // 重写基类 Event 的 OnRead 方法，当有可读数据时触发  
            // Overrides the OnRead method from the base Event class; triggered when data is readable.  
            void OnRead() override;  

            // 重写基类 Event 的 OnClose 方法，当管道关闭时触发  
            // Overrides the OnClose method from the base Event class; triggered when the pipe is closed.  
            void OnClose() override;  

            // 重写基类 Event 的 OnError 方法，处理错误信息  
            // Overrides the OnError method; handles error messages.  
            void OnError(const std::string &msg) override;  

            // Write 方法：向管道写入数据  
            // Write method: Writes data to the pipe.  
            // 参数 `data`：要写入的数据  
            // Parameter `data`: The data to be written.  
            // 参数 `len`：数据的长度  
            // Parameter `len`: The length of the data.  
            void Write(const char* data, size_t len);  

        private:  
            // 私有成员变量：写入端的文件描述符，初始值为 -1  
            // Private member variable: File descriptor for the write end, initialized to -1.  
            int write_fd_{-1};  
        };  

        // 定义 PipeEventPtr 为 std::shared_ptr<PipeEvent> 类型  
        // Defines `PipeEventPtr` as a `std::shared_ptr<PipeEvent>` type for easier use of smart pointers.  
        using PipeEventPtr = std::shared_ptr<PipeEvent>;  
    }  
}
