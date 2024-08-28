/**
 * 定时任务，用于定时执行任务
 */

#ifndef RAPIDRPC_NET_TIMER_EVENT_H
#define RAPIDRPC_NET_TIMER_EVENT_H

#include <functional>
#include <memory>
// #include <chrono>

namespace rapidrpc {

class TimerEvent {
public:
    // typedef std::shared_ptr<TimerEvent> s_ptr;
    using s_ptr = std::shared_ptr<TimerEvent>;

    TimerEvent(int interval, bool is_repeat, std::function<void()> cb);

    ~TimerEvent();

    /**
     * @brief 获取到达时间
     */
    int64_t getArriveTime() const {
        return m_arrive_time;
    }

    void setCanceled(bool value) {
        m_is_canceled = value;
        // clear task, avoid capture variable in lambda function cannot be released/circular reference, memory leak
        m_task = nullptr;
    }

    bool isCanceled() const {
        return m_is_canceled;
    }

    bool isRepeat() const {
        return m_is_repeat;
    }

    std::function<void()> getTask() const {
        return m_task;
    }

    void resetArriveTime();

private:
    int64_t m_arrive_time;     // 到达时间点/时间戳(ms), 可用于重复任务
    int64_t m_interval;        // 间隔时间
    bool m_is_repeat{false};   // 是否重复
    bool m_is_canceled{false}; // 是否取消

    std::function<void()> m_task; // 任务
};
} // namespace rapidrpc

#endif // RAPIDRPC_NET_TIMER_EVENT_H