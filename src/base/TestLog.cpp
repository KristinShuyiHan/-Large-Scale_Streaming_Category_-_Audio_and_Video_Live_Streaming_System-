#include "base/LogStream.h"   // 用于日志输出的流封装 (LogStream for logging streams)
#include "base/Logger.h"      // 日志管理类，用于日志系统 (Logger for managing logs)
#include "FileLog.h"          // 文件日志管理 (Handles file-based logging)
#include "FileMgr.h"          // 文件管理器 (Manages log files)
#include "TTime.h"            // 时间工具类，提供时间戳 (Time utility for timestamps)
#include "TaskMgr.h"          // 定时任务管理器 (Manages scheduled tasks)
#include <thread>             // C++标准库的多线程支持 (C++ threading support)

using namespace tmms::base;
std::thread t;

void TestLog()
{
    t = std::thread([](){
        while(true) // 无限循环，持续执行日志输出
        {
            LOG_INFO << " test info!!!now:" << TTime::NowMS();
            // 输出一条INFO级别的日志，附加当前时间戳。
            // Outputs INFO log with the current timestamp.

            LOG_DEBUG << " test debug!!! now:" << TTime::NowMS(); 
            // 输出一条DEBUG级别的日志，附加当前时间戳。
            // Outputs DEBUG log with the current timestamp.

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // 线程休眠500毫秒，避免日志过于频繁输出。
            // Thread sleeps for 500 milliseconds to limit log frequency.
        }
    });    
}


int main(int argc, const char **argv) 
{
    // Step 1: Create a file log instance named "test.log"  
    // 第一步：创建一个名为 "test.log" 的文件日志实例  
    FileLogPtr log = sFileMgr->GetFileLog("test.log");
    // Example: "test.log" is where logs will be stored in the file system.

    // Step 2: Set log file rotation policy to rotate every minute  
    // 第二步：设置日志文件的轮转策略，每分钟轮转一次  
    log->SetRotate(kRotateMinute);
    // kRotateMinute 是一个枚举值或常量，表示日志文件每分钟轮换。
    // Example: "test.log" becomes "test_1.log", "test_2.log" after every minute.

    // Step 3: Initialize the global logger and bind it to the file log  
    // 第三步：初始化全局日志记录器，并将其绑定到文件日志  
    tmms::base::g_logger = new Logger(log);  
    // g_logger 是一个全局指针，用于记录日志数据。

    // Step 4: Set the logging level to 'Trace' (the most detailed log level)  
    // 第四步：设置日志级别为 "kTrace"，这是最详细的日志级别  
    tmms::base::g_logger->SetLogLevel(kTrace);
    // kTrace 表示日志的级别，详细记录所有日志信息。

    // Step 5: Create a task to check and manage file logs every 1000ms (1 second)  
    // 第五步：创建一个任务，每 1000 毫秒（1 秒）检查和管理文件日志  
    TaskPtr task4 = std::make_shared<Task>(
        [](const TaskPtr &task) // Lambda function: the task's logic  
        {  
            sFileMgr->OnCheck(); // Call OnCheck() to manage file rotation and cleanup  
            // 调用 OnCheck() 管理日志文件轮转和清理  
            task->Restart(); // Restart the task for continuous execution  
            // 重新启动任务，使任务可以持续执行  
        },  
        1000); // Set task interval to 1000ms  
        // Example: Every second, this task will trigger `OnCheck` and restart itself.

    // Step 6: Add the created task to the task manager  
    // 第六步：将任务添加到任务管理器中  
    sTaskMgr->Add(task4);
    // sTaskMgr 是任务管理器，用于管理和调度所有的定时任务。

    // Step 7: Run a test log function  
    // 第七步：执行一个测试日志函数  
    TestLog();  
    // TestLog() 可能是一个函数，用于生成测试日志信息，验证日志系统是否正常工作。

    // Step 8: Infinite loop to keep running tasks and log operations  
    // 第八步：进入无限循环，持续运行任务管理器和日志操作  
    while (1)  
    {  
        sTaskMgr->OnWork(); // Execute pending tasks in the task manager  
        // 调用任务管理器的 OnWork()，执行所有待处理的任务  

        std::this_thread::sleep_for(std::chrono::milliseconds(50));  
        // Pause the loop for 50 milliseconds to avoid high CPU usage  
        // 线程暂停 50 毫秒，防止 CPU 占用率过高  
    }

    // Step 9: Return 0 indicating successful program termination  
    // 第九步：程序正常退出，返回 0  
    return 0;  
}
