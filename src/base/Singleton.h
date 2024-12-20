// 文件路径: /Users/shuyihan/Downloads/tmms/src/base/Singleton.h
// 包含必要的头文件，RtmpServer 可能是 RTMP 流媒体服务器的定义，MMediaLog 用于日志输出。
// Include necessary header files. RtmpServer likely defines the RTMP streaming server, 
// and MMediaLog is used for logging.
#include "RtmpServer.h" 
#include "mmedia/base/MMediaLog.h"

#pragma once
// #pragma once 确保此头文件在编译时只被包含一次。
// Ensures this header file is included only once during compilation.

#include "NonCopyable.h" 
// 引入 NonCopyable 头文件，用于防止类被拷贝。
// NonCopyable prevents copy construction and assignment operations.

#include <pthread.h>
// 引入 pthread 库，提供线程同步机制。
// Includes pthread library for thread synchronization.

namespace tmms // 命名空间: tmms
{
    namespace base // 子命名空间: base
    {
        // 使用模板定义一个 Singleton 类，T 是需要单例化的类。
        // A Singleton class template. T represents the class we want to make a singleton.
        template <typename T>
        class Singleton : public NonCopyable // 继承 NonCopyable，禁止拷贝构造和赋值操作。
        {
        public:
            Singleton() = delete; // 删除构造函数，禁止外部创建实例。
            ~Singleton() = delete; // 删除析构函数，禁止外部销毁实例。

            // 返回单例实例的指针，确保只初始化一次。
            // Static function to return the singleton instance. Ensures initialization happens only once.
            static T*& Instance()
            {
                pthread_once(&ponce_, &Singleton::init);
                // pthread_once 确保 init 函数在多线程环境下只执行一次。
                // pthread_once ensures the init function runs only once in a multi-threaded environment.

                return value_; 
                // 返回单例实例的指针。
                // Returns the pointer to the singleton instance.
            }

        private:
            // 初始化单例实例的函数。
            // Static function to initialize the singleton instance.
            static void init()
            {
                if (!value_) // 如果 value_ 为空，则创建一个新的 T 类型实例。
                {
                    value_ = new T();
                    // Dynamically allocate memory for the singleton instance.
                }
            }

            static pthread_once_t ponce_; // 线程同步变量，确保 init 只调用一次。
            // Static pthread_once_t variable to guarantee single initialization.

            static T* value_; // 静态指针，保存单例实例。
            // Static pointer to hold the singleton instance.
        };

        // 初始化静态变量 pthread_once_t。PTHREAD_ONCE_INIT 是初始化常量。
        // Initialize the static pthread_once_t variable with PTHREAD_ONCE_INIT.
        template <typename T>
        pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

        // 初始化静态指针 value_，设置为 nullptr。
        // Initialize the static singleton instance pointer to nullptr.
        template <typename T>
        T* Singleton<T>::value_ = nullptr;
    }
}
