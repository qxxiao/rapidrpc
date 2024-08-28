
#include "rapidrpc/net/io_thread.h"
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/util.h"

namespace rapidrpc {

/**
 * @brief 子线程对象，并启动一个 EventLoop
 * @note 注意生命周期，如果析构，会停止 EventLoop，并等待线程结束
 */
IOThread::IOThread() {
    int rt = sem_init(&m_init_semaphore, 0, 0); // 初始化信号量
    if (rt != 0) {
        ERRORLOG("Failed to init m_init_semaphore");
        return;
    }
    rt = sem_init(&m_start_semaphore, 0, 0);
    if (rt != 0) {
        ERRORLOG("Failed to init m_start_semaphore")
    }

    // 创建一个新的线程
    m_thread = std::thread(Main, this); // move assignment

    // wait for thread init
    rt = sem_wait(&m_init_semaphore);
    if (rt != 0) {
        ERRORLOG("Failed to wait semaphore");
        return;
    }
    DEBUGLOG("IOThread created, tid: %d", m_tid);
}

IOThread::~IOThread() {
    m_event_loop->stop();
    sem_destroy(&m_init_semaphore);
    sem_destroy(&m_start_semaphore);

    // 等待线程结束
    if (m_thread.joinable()) {
        m_thread.join();
    }

    if (m_event_loop) {
        delete m_event_loop;
        m_event_loop = nullptr;
    }
}

void *IOThread::Main(void *arg) {
    // 线程入口函数
    IOThread *thread = static_cast<IOThread *>(arg);

    // 创建一个 EventLoop 对象
    thread->m_event_loop = new EventLoop();
    thread->m_tid = getThreadId(); // 在线程内完成赋值

    // ! 唤醒主线程
    sem_post(&thread->m_init_semaphore);

    // ! 等待 start 信号量，进入事件循环
    sem_wait(&thread->m_start_semaphore);
    DEBUGLOG("IOThread start Eventloop::loop(), tid: %d", thread->m_tid);
    thread->m_event_loop->loop();
    DEBUGLOG("IOThread exit Eventloop::loop(), tid: %d", thread->m_tid);

    return nullptr;
}

EventLoop *IOThread::getEventLoop() {
    return m_event_loop;
}

void IOThread::start() {
    sem_post(&m_start_semaphore);
}

void IOThread::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

} // namespace rapidrpc