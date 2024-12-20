#pragma once  
// This ensures the file is included only once during compilation.  
// 它确保头文件在编译时只会被包含一次，避免重复包含引起的错误。

#include "Task.h"  
// Includes the definition of the `Task` class or related declarations.  
// 包含 Task 类或相关声明的头文件。

#include "Singleton.h"  
// Includes the Singleton template to manage the TaskMgr as a single instance.  
// 包含 Singleton 模板，用于管理 TaskMgr 为单例类。

#include <unordered_set>  
// Provides the unordered_set container for storing unique elements.  
// 引入 unordered_set 容器，用于存储唯一的任务对象。

#include <mutex>  
// Provides the `std::mutex` for thread-safe operations.  
// 引入 mutex 类，用于实现线程安全的互斥锁。

namespace tmms  
{  
    namespace base  
    {  
        class TaskMgr: public NonCopyable  
        {  
        public:  
            TaskMgr()=default;  
            // Default constructor. No custom logic is required here.  
            // 默认构造函数，直接使用编译器生成的构造函数。

            ~TaskMgr()=default;  
            // Default destructor. Nothing special needs to be cleaned up.  
            // 默认析构函数，直接使用编译器生成的析构函数。

            void OnWork();  
            // Member function `OnWork` executes tasks. Implementation not shown here.  
            // 成员函数 OnWork 用于执行任务，具体实现未在此展示。

            bool Add(TaskPtr &task);  
            // Adds a task to the task manager. Returns true if successful.  
            // 添加任务到任务管理器，成功时返回 true。

            bool Del(TaskPtr &task);  
            // Removes a task from the task manager. Returns true if successful.  
            // 从任务管理器中删除任务，成功时返回 true。

        private:  
            std::unordered_set<TaskPtr> tasks_;  
            // `unordered_set` is used to store unique task pointers.  
            // It prevents duplicate tasks from being added.  
            // unordered_set 容器用于存储唯一的任务指针，防止重复任务。

            std::mutex lock_;  
            // `mutex` is used to make the class thread-safe.  
            // Ensures that tasks can be added or removed safely in a multi-threaded environment.  
            // mutex 用于保证线程安全，确保在多线程环境下安全地添加或删除任务。  
        };  

    } // namespace base  

    #define sTaskMgr tmms::base::Singleton<tmms::base::TaskMgr>::Instance()  
    // Macro `sTaskMgr` creates a single instance of `TaskMgr` using the Singleton pattern.  
    // This allows global access to a single `TaskMgr` instance.  
    // 宏定义 sTaskMgr 使用 Singleton 模式创建 TaskMgr 的唯一实例，提供全局访问入口。

} // namespace tmms  
