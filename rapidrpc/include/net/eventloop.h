#ifndef RAPIDRPC_NET_EVENTLOOP_H
#define RAPIDRPC_NET_EVENTLOOP_H

#include "net/fd_event.h"
#include <net/wakeup_fd_event.h>
#include <net/timer.h>

#include <sys/types.h>
#include <set>
#include <functional>
#include <queue>
#include <mutex>

namespace rapidrpc {

/**
 * @brief 事件循环类, 基于主从 Reactor 事件模型
 */
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void loop();

    /**
     * @brief 唤醒 EventLoop
     */
    void wakeup();

    /**
     * @brief 停止 EventLoop
     */
    void stop();

    /**
     * @brief 添加一个监听的fd, 对于已经存在的fd，会更新其监听事件。
     * 由 EventLoop 本线程调用，或者主线程调用。
     * @param event: 监听的fd
     * @param is_wakeup: 是否立即唤醒此 EventLoop 线程, 用于快速将 fd 加到该线程的epoll监听集合中
     * @note 对于主从 Reactor模型，主Reactor负责监听新连接，从Reactor负责处理已有连接。夸线程会调用
     * rapidrpc::FdEvent::addTask()
     */
    void addEpollEvent(FdEvent *event);

    /**
     * @brief 删除一个监听的fd。其他和 addEpollEvent 一样。
     * @param event: 监听的fd
     */
    void deleteEpollEvent(FdEvent *event);

    /**
     * @brief 判断当前线程是否是 EventLoop 线程
     * @return true: 是 EventLoop 线程, false: 不是 EventLoop 线程
     */
    bool isInLoopThread();

    // 回调函数, 将任务添加到任务队列，并由当前线程自己执行
    /**
     * @brief 添加任务到队列，注意：当前线程的 EventLoop 循环跳出后才执行队列中的任务
     * @param cb: 任务函数/回调函数
     * @param is_wakeup: 是否立即唤醒此 EventLoop 线程, 将任务加到该线程的任务队列中立即唤醒该 LOOP
     * 线程。主要作用是，主线程添加任务并唤醒该Loop线程去执行任务队列中的任务，例如 迅速添加 fd 到 epoll 中，例如删除
     * fd等等。
     */
    void addTask(std::function<void()> cb, bool is_wakeup = false);

    /**
     * @brief 添加定时任务
     * @param event: 定时任务
     */
    void addTimerEvent(TimerEvent::s_ptr event);

private:
    void handleWakeUp();

    void initWakeupFdEvent(); // add wakeup fd to epoll
    void initTimer();         // add timer fd to epoll

private:
    pid_t m_tid{0}; // 线程id

    int m_epoll_fd{0}; // epoll fd

    // int m_wakeup_fd{0}; // 用于唤醒的 fd
    WakeupFdEvent *m_wakeup_event{nullptr}; // 用于唤醒的 FdEvent，封装 m_wakeup_fd

    bool m_stop_flag{false}; // 是否停止 EventLoop

    std::set<int> m_listen_fds; // 监听的fd集合

    std::mutex m_mutex;
    std::queue<std::function<void()>> m_pending_tasks; // 待处理的任务(当前Loop循环结束后处理)

    Timer *m_timer{nullptr}; // 定时器, 管理定时任务
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_EVENTLOOP_H