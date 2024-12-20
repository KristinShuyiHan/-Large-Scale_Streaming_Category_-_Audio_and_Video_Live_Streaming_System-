#pragma once  
// 防止头文件被多次包含，作用等同于 #ifndef/#define/#endif 宏定义保护。
// Prevents multiple inclusions of the same header file. It acts as a safeguard similar to `#ifndef/#define/#endif`.

#include <cstdint>  
// 包含用于固定宽度整数类型的头文件，比如 int64_t。
// Includes fixed-width integer types like `int64_t`.

#include <functional>  
// 包含 std::function 类模板，它可以存储、调用任何可调用对象（函数、lambda、仿函数等）。
// Includes `std::function` to store and call callable objects such as functions, lambdas, or functors.

#include <memory>  
// 包含智能指针的头文件，比如 std::shared_ptr 和 std::unique_ptr。
// Includes smart pointers like `std::shared_ptr` and `std::unique_ptr`.

namespace tmms  // 定义命名空间 tmms，表示这是 TMMS 项目的一部分。
// Define namespace `tmms`, indicating it belongs to the TMMS project.
{
    namespace base  // 在 base 子命名空间下，表示基础组件的代码。
// Under the `base` namespace, indicating foundational components of the project.
    {
        class Task;  
        // 预声明 Task 类，避免头文件循环依赖。
// Forward declaration of the `Task` class to avoid circular header dependencies.

        using TaskPtr = std::shared_ptr<Task>;  
        // 定义一个类型别名 TaskPtr，它是一个共享指针，指向 Task 对象。
// Define a type alias `TaskPtr`, which is a shared pointer to a `Task` object.

        using TaskCallback = std::function<void (const TaskPtr &)>;  
        // 定义一个类型别名 TaskCallback，它是一个函数对象，接受一个 TaskPtr 共享指针作为参数。
// Define a type alias `TaskCallback`, a function object that takes a `TaskPtr` as a parameter.

        class Task : public std::enable_shared_from_this<Task>  
        // 定义 Task 类，并继承 std::enable_shared_from_this<Task>，允许通过成员函数创建 shared_ptr。
// Define the `Task` class and inherit `std::enable_shared_from_this<Task>`, enabling shared_ptr creation from member functions.
        {
        public:
            Task(const TaskCallback &cb, int64_t interval);  
            // 构造函数，接收一个常量引用的 TaskCallback 和一个 int64_t 类型的时间间隔。
// Constructor that accepts a `TaskCallback` (constant reference) and an `int64_t` interval.

            Task(const TaskCallback &&cb, int64_t interval);  
            // 构造函数，接收一个右值引用的 TaskCallback 和一个 int64_t 类型的时间间隔。
// Constructor that accepts an rvalue reference `TaskCallback` and an `int64_t` interval.

            void Run();  
            // 执行任务回调函数。
// Execute the task's callback function.

            void Restart();  
            // 重新启动任务，通常会重置任务的触发时间。
// Restart the task, typically resetting its trigger time.

            int64_t When() const  
            // 获取任务的触发时间戳。
// Retrieve the task's trigger timestamp.
            {
                return when_;  
                // 返回任务触发时间成员变量。
// Return the `when_` member variable.
            }

        private:
            int64_t interval_{0};  
            // 任务执行的时间间隔，单位通常是毫秒。
// Interval between task executions, typically in milliseconds.

            int64_t when_{0};  
            // 任务的触发时间戳。
// The task's trigger timestamp.

            TaskCallback cb_;  
            // 存储任务的回调函数。
// Store the task's callback function.
        };
    }
}
