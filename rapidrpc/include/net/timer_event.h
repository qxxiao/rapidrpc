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

    void setCancled(bool value) {
        m_is_cancled = value;
    }

    bool isCancled() const {
        return m_is_cancled;
    }

    bool isRepeat() const {
        return m_is_repeat;
    }

    std::function<void()> getTask() const {
        return m_task;
    }

    void resetArriveTime();

private:
    int64_t m_arrive_time;    // 到达时间点/时间戳
    int64_t m_interval;       // 间隔时间
    bool m_is_repeat{false};  // 是否重复
    bool m_is_cancled{false}; // 是否取消

    std::function<void()> m_task; // 任务
};
} // namespace rapidrpc

#endif // RAPIDRPC_NET_TIMER_EVENT_H