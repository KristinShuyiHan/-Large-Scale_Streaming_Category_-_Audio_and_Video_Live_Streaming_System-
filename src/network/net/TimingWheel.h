#pragma once
// 英文: Prevents multiple inclusions of this header file during compilation.  
// 中文: 防止头文件被多次包含，确保编译时不会重复定义。

#include <memory>            // For std::shared_ptr, smart pointer management
#include <functional>        // For std::function, callback functions
#include <cstdint>           // For fixed-width integer types like uint32_t
#include <vector>            // For std::vector, dynamic array storage
#include <unordered_set>     // For std::unordered_set, used for unique entries
#include <deque>             // For std::deque, double-ended queue storage

namespace tmms // Namespace to group related code and avoid naming conflicts
{
    namespace network
    {
        // -------------------- Type Aliases (别名定义) --------------------
        using EntryPtr = std::shared_ptr<void>; // Smart pointer to any type (void pointer)
        // 英文: A shared_ptr that can point to any type, allowing flexible use cases.  
        // 中文: 使用智能指针（shared_ptr）指向任意类型的数据，实现灵活管理。

        using WheelEntry = std::unordered_set<EntryPtr>; // A set to store unique entries
        // 英文: Stores unique entries efficiently using unordered_set.  
        // 中文: 使用unordered_set存储唯一的条目，避免重复。

        using Wheel = std::deque<WheelEntry>; // A deque (double-ended queue) to store time slots
        // 英文: A deque to hold WheelEntry, representing slots of a timing wheel.  
        // 中文: 使用双端队列表示时间轮的时间槽，每个槽中存储不同的条目。

        using Wheels = std::vector<Wheel>; // A vector to represent multiple timing wheels
        // 英文: Represents a collection of timing wheels for seconds, minutes, hours, etc.  
        // 中文: 使用vector存储多个时间轮，比如秒、分钟、小时的分级时间管理。

        using Func = std::function<void()>; // Function type for callbacks
        // 英文: std::function type that represents any callable function with no parameters and void return.  
        // 中文: 表示回调函数类型，可以绑定任意无参数、无返回值的函数。

        // -------------------- Constants (常量定义) --------------------
        const int kTimingMinute = 60;             // 1 minute = 60 seconds
        const int kTimingHour = 60*60;            // 1 hour = 3600 seconds
        const int kTimingDay = 60*60*24;          // 1 day = 86400 seconds

        enum TimingType
        {
            kTimingTypeSecond = 0, // Timing wheel for seconds
            kTimingTypeMinute = 1, // Timing wheel for minutes
            kTimingTypeHour = 2,   // Timing wheel for hours
            kTimingTypeDay = 3,    // Timing wheel for days
        };
        // 英文: Represents different levels of time units used in the timing wheel.  
        // 中文: 定义时间轮的不同时间单位（秒、分钟、小时、天）。

        // -------------------- CallbackEntry 类 --------------------
        class CallbackEntry
        {
        public:
            CallbackEntry(const Func &cb):cb_(cb) 
            {}
            ~CallbackEntry()
            {
                if(cb_) // Check if the callback function is valid
                {
                    cb_(); // Execute the callback when the object is destroyed
                }
            }
        private:
            Func cb_; // Stores the callback function
        };
        using CallbackEntryPtr = std::shared_ptr<CallbackEntry>; 
        // 英文: Smart pointer to manage the lifetime of CallbackEntry objects.  
        // 中文: 使用智能指针管理 CallbackEntry 对象的生命周期，确保资源被释放。

        // -------------------- TimingWheel 类 --------------------
        class TimingWheel
        {
        public:
            TimingWheel();
            ~TimingWheel();

            void InsertEntry(uint32_t delay, EntryPtr entryPtr);
            // 英文: Inserts an entry into the timing wheel with a delay (in seconds).  
            // 中文: 将一个条目插入到时间轮中，延迟 delay 秒后触发。

            void OnTimer(int64_t now);
            // 英文: Called periodically to advance the timing wheel and process entries.  
            // 中文: 定期调用以推动时间轮并处理已到期的条目。

            void PopUp(Wheel &bq);
            // 英文: Processes all entries in the current time slot.  
            // 中文: 处理当前时间槽中的所有条目。

            void RunAfter(double delay, const Func &cb);
            void RunAfter(double delay, Func &&cb);
            // 英文: Schedules a callback function to run after a specified delay.  
            // 中文: 在指定的延迟后运行一个回调函数。

            void RunEvery(double interval, const Func &cb);
            void RunEvery(double interval, Func &&cb);
            // 英文: Schedules a callback function to run repeatedly at a given interval.  
            // 中文: 每隔一定时间间隔重复执行一个回调函数。

        private:
            void InsertSecondEntry(uint32_t delay, EntryPtr entryPtr);
            void InsertMinuteEntry(uint32_t delay, EntryPtr entryPtr);
            void InsertHourEntry(uint32_t delay, EntryPtr entryPtr);
            void InsertDayEntry(uint32_t delay, EntryPtr entryPtr);
            // 英文: Helper functions to insert entries into the respective timing wheels (seconds, minutes, etc.).  
            // 中文: 辅助函数，将条目插入到不同层级的时间轮中（秒、分钟、小时、天）。

            Wheels wheels_; // A vector holding all timing wheels
            // 英文: Stores the hierarchical timing wheels for managing different delays.  
            // 中文: 使用vector存储多个分级时间轮，用于管理不同延迟的事件。

            int64_t last_ts_{0}; // The last processed timestamp
            // 英文: Tracks the last time the timing wheel was updated.  
            // 中文: 记录上一次时间轮处理的时间戳。

            uint64_t tick_{0}; // The current tick count of the timing wheel
            // 英文: Tracks the number of ticks (time slots) the timing wheel has processed.  
            // 中文: 记录时间轮已经处理的时间槽数（tick）。
        };
    }
}
