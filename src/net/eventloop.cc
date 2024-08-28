#include "rapidrpc/net/eventloop.h"
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/util.h"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>

// micro to add epoll event
// with local variable (it,op,ev,rt)
#define ADD_TO_EPOLL(event, m_epoll_fd)                                                                                \
    auto it = m_listen_fds.find(event->getFd());                                                                       \
    int op = EPOLL_CTL_ADD;                                                                                            \
    if (it != m_listen_fds.end()) {                                                                                    \
        op = EPOLL_CTL_MOD;                                                                                            \
    }                                                                                                                  \
    else {                                                                                                             \
        m_listen_fds.insert(event->getFd());                                                                           \
    }                                                                                                                  \
    epoll_event ev = event->getEpollEvent();                                                                           \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &ev);                                                           \
    if (rt < 0) {                                                                                                      \
        ERRORLOG("Failed to add fd_event[fd=%d] to epoll, error [%s]", event->getFd(), strerror(errno));               \
    }                                                                                                                  \
    else                                                                                                               \
        DEBUGLOG("Add fd_event[fd=%d] to epoll(%s)", event->getFd(), (op == EPOLL_CTL_ADD ? "ADD" : "MOD"));

// micro to delete epoll event
// with local variable (it,rt)
#define DELETE_FROM_EPOLL(event, m_epoll_fd)                                                                           \
    auto it = m_listen_fds.find(event->getFd());                                                                       \
    if (it != m_listen_fds.end()) {                                                                                    \
        int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event->getFd(), nullptr);                                        \
        if (rt < 0) {                                                                                                  \
            ERRORLOG("Failed to delete epoll event, error [%s]", strerror(errno));                                     \
        }                                                                                                              \
        m_listen_fds.erase(it);                                                                                        \
        DEBUGLOG("Delete epoll event, fd [%d]", event->getFd());                                                       \
    }

namespace rapidrpc {

// * 每个线程最多一个 EventLoop, thread local 变量
static thread_local EventLoop *t_current_loop = nullptr;
static int g_epoll_max_timeout = 10000;
static int g_epoll_max_events = 10;

// 1. 添加 m_wakeup_fd 到 epoll 中
// 2. 打印 EventLoop 创建信息
// 3. 初始化，外部 AddEpollEvent
// 4. Loop 循环处理事件
EventLoop::EventLoop() {
    if (t_current_loop) { // CHECK: 每个线程最多一个 EventLoop
        ERRORLOG("Failed to create EventLoop. Another EventLoop exists in this thread");
        exit(-1);
    }

    m_tid = getThreadId();
    m_epoll_fd = epoll_create(100);
    if (m_epoll_fd < 0) {
        ERRORLOG("Failed to create epoll fd, error [%s]", strerror(errno));
        exit(-1);
    }

    initWakeupFdEvent(); // 添加 m_wakeup_fd 到 epoll 中
    initTimer();         // 添加定时器 m_Timer 到 epoll 中

    INFOLOG("EventLoop created in thread %d", m_tid);
    t_current_loop = this;
}

EventLoop::~EventLoop() {
    // TODO : close all listen fds
    close(m_epoll_fd);
    if (m_wakeup_event) {
        delete m_wakeup_event;
        m_wakeup_event = nullptr;
    }
    if (m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
}

void EventLoop::initWakeupFdEvent() {
    // * 每个 EventLoop 都有一个 wakeup_fd 用于唤醒
    // （外部通过该 EventLoop 的 m_wakeup_fd 写入，唤醒事件等待）
    int wakeup_fd = eventfd(0, EFD_NONBLOCK);
    if (wakeup_fd < 0) {
        ERRORLOG("Failed to create eventfd, error [%s]", strerror(errno));
        exit(-1);
    }
    // ! 封装 m_wakeup_fd 为 FdEvent 对象
    // 构造函数中完成设置监听事件和回调函数初始化
    m_wakeup_event = new WakeupFdEvent(wakeup_fd); // m_fd
    // 将 m_wakeup_fd 添加到 epoll 中
    // m_wakeup_event/ m_wakeup_fd 是特定功能的 eventfd, 用于唤醒 epoll_wait
    addEpollEvent(m_wakeup_event);
}

void EventLoop::initTimer() {
    m_timer = new Timer(); // 创建定时器,完成了初始化
    addEpollEvent(m_timer);
}

void EventLoop::loop() {
    m_is_looping = true;
    while (!m_stop_flag) {
        // 处理并清空任务队列，避免循环前队列不为空
        std::queue<std::function<void()>> tasks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            tasks.swap(m_pending_tasks);
        }

        while (!tasks.empty()) {
            // null check: 避免触发了非预期的事件，获取的回调函数是空的
            std::function<void()> cb(std::move(tasks.front()));
            tasks.pop();
            if (cb) {
                cb();
            }
        }

        int timeout = g_epoll_max_timeout;
        epoll_event result_events[g_epoll_max_events]; // return events
        int rt = epoll_wait(m_epoll_fd, result_events, g_epoll_max_events, timeout);

        if (rt < 0) {
            ERRORLOG("epoll_wait error, error [%s]", strerror(errno));
            continue;
        }
        else { // 0: timeout, >0: events
            for (int i = 0; i < rt; i++) {
                epoll_event trigger_event = result_events[i];
                FdEvent *fd_event = static_cast<FdEvent *>(trigger_event.data.ptr);
                if (!fd_event)
                    continue;
                // 可读事件
                if (trigger_event.events & EPOLLIN) {
                    // add task
                    auto tmp = fd_event->getHandler(TriggerEvent::IN_EVENT);
                    addTask(fd_event->getHandler(TriggerEvent::IN_EVENT));
                    DEBUGLOG("%s[%d] trigger IN event",
                             (fd_event->getFd() == m_timer->getFd()
                                  ? "Timer fd"
                                  : (fd_event->getFd() == m_wakeup_event->getFd() ? "Wakeup fd" : "fd")),
                             fd_event->getFd());
                }
                // 可写事件
                else if (trigger_event.events & EPOLLOUT) {
                    // add task
                    auto tmp = fd_event->getHandler(TriggerEvent::OUT_EVENT);
                    addTask(fd_event->getHandler(TriggerEvent::OUT_EVENT));
                    DEBUGLOG("fd[%d] trigger OUT event", fd_event->getFd());
                }
                // 其它事件，例如 EPOLLERR, EPOLLHUP（连接失败会与 EPOLLOUT 同时触发）
                else {
                    INFOLOG("fd[%d] trigger other event[%u]", fd_event->getFd(),
                            static_cast<unsigned int>(trigger_event.events));
                }
            }
        }
    }
}

// only write one byte to eventfd
// call by other thread to wakeup this thread from epoll_wait
void EventLoop::wakeup() {
    m_wakeup_event->wakeup();
}

void EventLoop::stop() {
    m_stop_flag = true;
    wakeup();
}

void EventLoop::handleWakeUp() {}

//* 添加一个监听的fd
// 说明，对于主从 Reactor模型，主Reactor负责监听新连接，从Reactor负责处理已有连接
// 自己本线程可以直接添加/更新 fd 到 epoll 中，对于跨线程，例如一个线程将 fd 分配到另一个 Reactor 线程处理,
// 通过回调函数形式添加到 这个 EventLoop 任务队列中，等到下次唤醒后 该 EventLoop 线程自己执行, 自己管理这个 fd
void EventLoop::addEpollEvent(FdEvent *event) {
    if (isInLoopThread()) {
        // auto it = m_listen_fds.find(event->getFd());
        // int op = EPOLL_CTL_ADD;
        // if (it != m_listen_fds.end()) { // 已经存在
        //     op = EPOLL_CTL_MOD;
        // }
        // epoll_event ev = event->getEpollEvent(); // 获取事件
        // int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &ev);
        // if (rt < 0) {
        //     ERRORLOG("Failed to add epoll event, error [%s]", strerror(errno));
        // }
        ADD_TO_EPOLL(event, m_epoll_fd);
    }
    else {
        // ! 不在当前 EventLoop 运行的线程，回调函数处理，添加到该 Loop 对象的任务队列
        auto callback = [event, this]() {
            ADD_TO_EPOLL(event, this->m_epoll_fd);
        };
        addTask(callback, true); // 添加到任指定 EventLoop 任务队列
    }
}

void EventLoop::deleteEpollEvent(FdEvent *event) {
    if (isInLoopThread()) {
        // auto it = m_listen_fds.find(event->getFd());
        // if (it != m_listen_fds.end()) {
        //     int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event->getFd(), nullptr);
        //     if (rt < 0) {
        //         ERRORLOG("Failed to delete epoll event, error [%s]", strerror(errno));
        //     }
        //     m_listen_fds.erase(it);
        //     DEBUGLOG("Delete epoll event, fd [%d]", event->getFd());
        // }
        DELETE_FROM_EPOLL(event, m_epoll_fd);
    }
    else {
        auto callback = [event, this]() {
            DELETE_FROM_EPOLL(event, this->m_epoll_fd);
        };
        addTask(callback, true);
    }
}

//* 添加任务到队列，注意：当前线程的 EventLoop 循环跳出后执行队列中的任务
void EventLoop::addTask(std::function<void()> cb, bool is_wakeup) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending_tasks.push(cb);
    }
    if (is_wakeup)
        wakeup();
}

//* 添加定时任务
void EventLoop::addTimerEvent(TimerEvent::s_ptr event) {
    m_timer->addTimerEvent(event);
}

//* 移除定时任务
void EventLoop::deleteTimerEvent(TimerEvent::s_ptr event) {
    m_timer->deleteTimerEvent(event);
}

bool EventLoop::isInLoopThread() {
    return getThreadId() == m_tid; // 判断是否在当前线程(即EventLoop所在的线程)
}

EventLoop *EventLoop::GetCurrentEventLoop() {
    if (t_current_loop)
        return t_current_loop;
    t_current_loop = new EventLoop();
    return t_current_loop;
}

bool EventLoop::isLooping() const {
    return m_is_looping;
}

} // namespace rapidrpc