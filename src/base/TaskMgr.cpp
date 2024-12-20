#include "TaskMgr.h"   // 引入 TaskMgr 类的头文件，包含 TaskMgr 的类声明。
                       // Includes "TaskMgr.h" header file where TaskMgr class is declared.

#include "TTime.h"     // 引入 TTime 类的头文件，提供时间相关功能。
                       // Includes "TTime.h" header file for time-related functions (e.g., NowMS()).

using namespace tmms::base;  // 使用 tmms::base 命名空间，避免重复写前缀。
                             // Use the namespace `tmms::base` to simplify code (no need to write "tmms::base" repeatedly).


// OnWork() 遍历所有的任务，如果某个任务的执行时间到了，就执行它。如果任务执行后仍然不符合时间要求（例如，单次任务），就从列表中删除它。

void TaskMgr::OnWork()
{
    std::lock_guard<std::mutex> lk(lock_);  // 上锁：lock_guard 会自动锁定 lock_ 互斥量，离开作用域时自动解锁。
                                           // std::lock_guard is a RAII-based lock. It locks `lock_` to ensure thread safety 
                                           // and automatically unlocks it when going out of scope.

    int64_t now = TTime::NowMS();           // 调用 TTime::NowMS() 获取当前时间，单位是毫秒。
                                           // Use `TTime::NowMS()` to get the current timestamp in milliseconds.

    for(auto iter = tasks_.begin(); iter != tasks_.end(); )  // 遍历 tasks_ 集合。
                                           // Loop through all tasks in `tasks_` using an iterator.

    {
        if((*iter)->When() < now)          // 如果当前任务的触发时间小于当前时间，说明需要执行。
                                           // Check if the task's scheduled time (`When()`) is earlier than `now`.

        {
            (*iter)->Run();                // 调用任务的 Run() 方法，执行任务。
                                           // Execute the task by calling its `Run()` method.

            if((*iter)->When() < now)      // 再次检查任务的触发时间，如果还是小于当前时间，说明任务已完成，需要删除。
                                           // After running, check again if the task's time is still less than `now`. 
                                           // This indicates a one-time task that is done.

            {
                iter = tasks_.erase(iter); // 从任务集合中删除当前任务，并将迭代器更新为下一个有效位置。
                                           // Remove the task using `erase()` and update the iterator to the next valid position.

                continue;                  // 跳过当前循环，继续处理下一个任务。
                                           // Skip the rest of the loop body and continue with the next task.
            }
        }
        iter++;                            // 如果任务不需要删除，则将迭代器移动到下一个任务。
                                           // Move the iterator to the next task.
    }
}




// Add() 函数向任务列表中添加任务，先检查是否已有该任务，如果存在就返回 false，否则将任务加入列表并返回 true

bool TaskMgr::Add(TaskPtr &task)           // 添加一个任务，参数是 TaskPtr 的引用。
                                           // Function to add a task. The parameter `task` is a reference to `TaskPtr`.

{
    std::lock_guard<std::mutex> lk(lock_); // 上锁，确保任务添加过程的线程安全。
                                           // Lock the mutex `lock_` to make sure adding is thread-safe.

    auto iter = tasks_.find(task);         // 在任务集合中查找是否已存在该任务。
                                           // Check if the task already exists in the `tasks_` container.

    if(iter != tasks_.end())               // 如果找到相同任务，返回 false，表示添加失败。
                                           // If the task is found, return `false` because it's already in the container.

    {
        return false;
    }
    tasks_.emplace(task);                  // 使用 emplace() 将任务添加到 tasks_ 集合中。
                                           // Use `emplace()` to add the task to the `tasks_` container efficiently.

    return true;                           // 返回 true，表示任务添加成功。
                                           // Return `true` indicating the task was successfully added.
}


bool TaskMgr::Del(TaskPtr &task)           // 删除任务，参数是 TaskPtr 的引用。
                                           // Function to delete a task. The parameter `task` is a reference to `TaskPtr`.

{
    std::lock_guard<std::mutex> lk(lock_); // 上锁，确保删除过程的线程安全。
                                           // Lock the mutex `lock_` to make sure deletion is thread-safe.

    tasks_.erase(task);                    // 从任务集合中删除指定任务。
                                           // Use `erase()` to remove the task from the `tasks_` container.

    return true;                           // 返回 true，表示任务删除成功。
                                           // Return `true` indicating the task was successfully deleted.
}
