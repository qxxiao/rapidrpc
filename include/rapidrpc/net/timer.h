/**
 * 定时器, 是一个定时任务的集合。同时它是 FdEvent，用于被 EventLoop 监听。
 * 主要目的是，管理所有的定时任务，当定时任务到期时，触发回调函数。
 */

#ifndef RAPIDRPC_NET_TIMER_H
#define RAPIDRPC_NET_TIMER_H

#include "rapidrpc/net/fd_event.h"
#include "rapidrpc/net/timer_event.h"

#include <map>
#include <mutex>

namespace rapidrpc {

class Timer: public FdEvent {

public:
    Timer();
    ~Timer();

    /**
     * @brief 添加定时任务
     * @param event: 定时任务
     */
    void addTimerEvent(TimerEvent::s_ptr event);

    /**
     * @brief 移除定时任务
     * @param event: 定时任务
     */
    void deleteTimerEvent(TimerEvent::s_ptr event);

    /**
     * @brief 定时器到期时触发, 定时器文件可读时的回调函数
     */
    void onTimer();

private:
    /**
     * @brief 重置定时器
     */
    void resetTimer();

private:
    // 定时任务集合
    std::mutex m_mutex;
    std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events;
};

} // namespace rapidrpc

#endif // !RAPIDRPC_NET_TIMER_H
