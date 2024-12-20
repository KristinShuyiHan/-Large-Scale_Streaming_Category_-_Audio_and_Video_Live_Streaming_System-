#include "TaskMgr.h"  
#include "TTime.h"  
#include <iostream>  
#include <thread>  
#include <chrono>

// 引入必要的头文件
// Includes necessary header files
// - "TaskMgr.h"：任务管理器，用于添加和调度定时任务。
//   "TaskMgr.h" - Manages tasks by scheduling and handling them.
// - "TTime.h"：提供时间相关功能，比如获取当前时间戳。
//   "TTime.h" - Provides time utilities like retrieving the current timestamp.
// - <iostream>：用于输出信息到控制台。
//   <iostream> - Enables console output.
// - <thread> 和 <chrono>：提供线程和时间相关功能，但这里未直接使用。
//   <thread> and <chrono> - For threads and time utilities (not directly used here).

using namespace tmms::base;  
// 使用 tmms::base 命名空间，简化代码中类和函数的调用。
// Using the `tmms::base` namespace to simplify the use of classes and functions.

void TestTask() // 定义 TestTask 函数，创建并添加定时任务。  
{  
    // 1. 定义任务 task1：每 1000 毫秒（1 秒）执行一次。
    // Define task1: Executes every 1000 milliseconds (1 second).
    TaskPtr task1 = std::make_shared<Task>(  
        [](const TaskPtr &task) // Lambda 表达式，任务的执行逻辑。  
        {  
            std::cout << "task1 interval:" << 1000 
                      << " now:" << TTime::NowMS() << std::endl;  
            // 输出任务名称、任务间隔（1000 毫秒）和当前时间戳。
            // Outputs task interval (1000 ms) and current timestamp.  
        },  
        1000); // 1000 表示任务执行的时间间隔。  
               // 1000 specifies the time interval for task execution.

    // 2. 定义任务 task2：每 1000 毫秒执行，执行后重启任务。
    // Define task2: Executes every 1000 milliseconds and restarts after execution.
    TaskPtr task2 = std::make_shared<Task>(  
        [](const TaskPtr &task)  
        {  
            std::cout << "task2 interval:" << 1000 
                      << " now:" << TTime::NowMS() << std::endl;  
            task->Restart(); // 重启任务，确保再次执行。  
                             // Restarts the task to execute again.  
        },  
        1000);  

    // 3. 定义任务 task3：每 500 毫秒执行，执行后重启任务。
    // Define task3: Executes every 500 milliseconds and restarts after execution.
    TaskPtr task3 = std::make_shared<Task>(  
        [](const TaskPtr &task)  
        {  
            std::cout << "task3 interval:" << 500 
                      << " now:" << TTime::NowMS() << std::endl;  
            task->Restart(); // 重启任务，间隔时间保持 500 毫秒。
                             // Restarts the task to maintain a 500 ms interval.  
        },  
        500);  

    // 4. 定义任务 task4：每 30000 毫秒（30 秒）执行，执行后重启任务。
    // Define task4: Executes every 30000 milliseconds (30 seconds) and restarts.
    TaskPtr task4 = std::make_shared<Task>(  
        [](const TaskPtr &task)  
        {  
            std::cout << "task4 interval:" << 30000 
                      << " now:" << TTime::NowMS() << std::endl;  
            task->Restart(); // 重启任务，间隔时间保持 30 秒。
                             // Restarts the task to maintain a 30-second interval.  
        },  
        30000);  

    // 将任务添加到任务管理器中
    // Add tasks to the task manager for scheduling and execution.
    sTaskMgr->Add(task1); // 添加 task1
                          // Adds task1 to the task manager.  
    sTaskMgr->Add(task2); // 添加 task2
                          // Adds task2 to the task manager.  
    sTaskMgr->Add(task3); // 添加 task3
                          // Adds task3 to the task manager.  
    sTaskMgr->Add(task4); // 添加 task4
                          // Adds task4 to the task manager.  
}

int main(int argc, const char **agrc) // 主函数，程序的入口点  
{  
    TestTask(); // 调用 `TestTask` 函数，初始化和添加任务  
    while(1) // 无限循环，程序会一直运行  
    {  
        sTaskMgr->OnWork(); // 执行任务管理器的 `OnWork` 成员函数，处理任务逻辑  
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
        // 当前线程休眠 50 毫秒，降低 CPU 占用率，避免资源浪费
    }  
    return 0; // 返回 0 表示程序正常结束  
}
