#ifndef RAPIDRPC_NET_IO_THREAD_H
#define RAPIDRPC_NET_IO_THREAD_H

#include "rapidrpc/net/eventloop.h"

#include <thread>
#include <semaphore.h>

namespace rapidrpc {

class IOThread {
public:
    IOThread();
    ~IOThread();

    /**
     * @brief 获取线程的 EventLoop 对象
     * @note 用于获取该线程的 EventLoop 对象，用于添加事件，例如常规套接字事件，定时任务，停止线程loop等
     */
    EventLoop *getEventLoop();

    /**
     * @brief 开启 loop
     * @note 使用信号量，唤醒该线程执行 loop
     */
    void start();

    /**
     * @brief 等待线程结束
     * @note 正常情况下，会一直阻塞，除非调用 getEventLoop()->stop() 退出 loop
     */
    void join();

private:
    static void *Main(void *arg);

private:
    pid_t m_tid{-1};        // 线程ID
    std::thread m_thread{}; // 线程对象

    EventLoop *m_event_loop{nullptr}; // 事件循环, 当前线程的 EventLoop

    /**
     * @brief 用于线程同步的信号量, C++需要 20版本才支持
     */
    sem_t m_init_semaphore;  // 信号量， 完成 EventLoop 和 tid 初始化
    sem_t m_start_semaphore; // 启动 loop
};

} // namespace rapidrpc

#endif // !RAPIDRPC_NET_IO_THREAD_H