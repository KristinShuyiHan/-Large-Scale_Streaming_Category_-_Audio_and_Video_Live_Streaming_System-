#include "TimingWheel.h"  
#include "network/base/Network.h"  
#include <iostream>  

using namespace tmms::network;




// 这段代码实现了一个高效的定时任务管理器（时间轮），通过分层将任务按秒、分钟、小时和天管理起来。
// OnTimer 函数会定时触发，检查当前时间轮中的任务并执行它们。
// RunAfter 和 RunEvery 支持延迟执行和周期性任务，例如 “5 秒后打印消息” 或 “每 10 秒重复打印消息”。

// 构造函数：初始化时间轮，分为四个层级：秒、分、时、天
// Constructor: Initializes the timing wheel with 4 layers (second, minute, hour, day).
TimingWheel::TimingWheel() : wheels_(4) {  
    wheels_[kTimingTypeSecond].resize(60); // 秒级时间轮有 60 个槽位 (0-59 秒)
    wheels_[kTimingTypeMinute].resize(60); // 分钟级时间轮有 60 个槽位 (0-59 分钟)
    wheels_[kTimingTypeHour].resize(24);   // 小时级时间轮有 24 个槽位 (0-23 小时)
    wheels_[kTimingTypeDay].resize(30);    // 天级时间轮有 30 个槽位 (1-30 天)
}

// 析构函数：释放资源（这里没有具体实现）
// Destructor: No specific resource cleanup.
TimingWheel::~TimingWheel() {}

// 插入任务到秒级时间轮的指定槽位
// Inserts a task into the second-level timing wheel.
void TimingWheel::InsertSecondEntry(uint32_t delay, EntryPtr entryPtr) {
    wheels_[kTimingTypeSecond][delay - 1].emplace(entryPtr); // 将任务放到 delay-1 对应的槽位中
}

// 插入任务到分钟级时间轮，延迟时间超过 60 秒时会分解到秒级时间轮
// Inserts a task into the minute-level timing wheel, and recurses remaining seconds.
void TimingWheel::InsertMinuteEntry(uint32_t delay, EntryPtr entryPtr) {
    auto minute = delay / kTimingMinute;  // 计算需要多少分钟
    auto second = delay % kTimingMinute;  // 计算剩余的秒数
    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr]() {
        InsertEntry(second, entryPtr);    // 将剩余秒数的任务插入秒级时间轮
    });
    wheels_[kTimingTypeMinute][minute - 1].emplace(newEntryPtr); // 任务放到分钟级时间轮
}

// 插入任务到小时级时间轮，超过 1 小时时分解到分钟级时间轮
// Inserts a task into the hour-level timing wheel, recursing remaining seconds.
void TimingWheel::InsertHourEntry(uint32_t delay, EntryPtr entryPtr) {
    auto hour = delay / kTimingHour;  // 计算需要多少小时
    auto second = delay % kTimingHour;  // 计算剩余的秒数
    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr]() {
        InsertEntry(second, entryPtr);  // 递归将剩余时间插入秒级时间轮
    });
    wheels_[kTimingTypeHour][hour - 1].emplace(newEntryPtr); // 任务放入小时级时间轮
}

// 插入任务到天级时间轮，超过 1 天时分解到小时级时间轮
// Inserts a task into the day-level timing wheel.
void TimingWheel::InsertDayEntry(uint32_t delay, EntryPtr entryPtr) {
    auto day = delay / kTimingDay;  // 计算需要多少天
    auto second = delay % kTimingDay;  // 计算剩余的秒数
    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr]() {
        InsertEntry(second, entryPtr);  // 将剩余秒数任务插入
    });
    wheels_[kTimingTypeDay][day - 1].emplace(newEntryPtr); // 任务放入天级时间轮
}

// 根据延迟时间将任务插入到对应的时间轮层级
// Inserts an entry into the appropriate timing wheel level based on delay.
void TimingWheel::InsertEntry(uint32_t delay, EntryPtr entryPtr) {
    if (delay <= 0) { // 如果延迟时间为 0，释放任务
        entryPtr.reset();
    }
    if (delay < kTimingMinute) { // 小于 60 秒，放入秒级时间轮
        InsertSecondEntry(delay, entryPtr);
    } else if (delay < kTimingHour) { // 小于 1 小时，放入分钟级时间轮
        InsertMinuteEntry(delay, entryPtr);
    } else if (delay < kTimingDay) {  // 小于 1 天，放入小时级时间轮
        InsertHourEntry(delay, entryPtr);
    } else {  // 超过 1 天
        auto day = delay / kTimingDay; 
        if (day > 30) { // 限制不能超过 30 天
            NETWORK_ERROR << "It does not support > 30 days!";
            return;
        }
        InsertDayEntry(delay, entryPtr); // 插入天级时间轮
    }
}

// 定时器触发函数：每秒触发一次，检查当前时间轮是否有任务要执行
// Timer tick function: Called every second to check for tasks to execute.
void TimingWheel::OnTimer(int64_t now) {
    if (last_ts_ == 0) {
        last_ts_ = now;
    }
    if (now - last_ts_ < 1000) { // 每秒触发一次
        return;
    }
    last_ts_ = now;
    ++tick_;
    PopUp(wheels_[kTimingTypeSecond]); // 执行秒级时间轮的任务
    if (tick_ % kTimingMinute == 0) {
        PopUp(wheels_[kTimingTypeMinute]); // 每分钟触发一次
    } else if (tick_ % kTimingHour == 0) {
        PopUp(wheels_[kTimingTypeHour]); // 每小时触发一次
    } else if (tick_ % kTimingDay == 0) {
        PopUp(wheels_[kTimingTypeDay]); // 每天触发一次
    }
}

// 执行当前时间轮槽位中的所有任务，并清空槽位
// Executes tasks in the current timing wheel slot and clears the slot.
void TimingWheel::PopUp(Wheel &bq) {
    WheelEntry tmp;
    bq.front().swap(tmp); // 取出当前槽位的任务
    bq.pop_front(); // 移除当前槽位
    bq.push_back(WheelEntry()); // 添加一个空槽位到末尾
}

// 执行延迟任务
// Runs a delayed task.
void TimingWheel::RunAfter(double delay, const Func &cb) {
    CallbackEntryPtr cbEntry = std::make_shared<CallbackEntry>([cb]() { cb(); });
    InsertEntry(delay, cbEntry);
}

// 执行周期性任务，每次任务执行完毕后重新插入
// Runs a periodic task by re-inserting it after execution.
void TimingWheel::RunEvery(double interval, const Func &cb) {
    CallbackEntryPtr cbEntry = std::make_shared<CallbackEntry>([this, cb, interval]() {
        cb();
        RunEvery(interval, cb);
    });
    InsertEntry(interval, cbEntry);
}
