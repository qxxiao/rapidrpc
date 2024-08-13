#ifndef RAPIDRPC_NET_FD_EVENT_H
#define RAPIDRPC_NET_FD_EVENT_H

#include <functional>
#include <sys/epoll.h>

namespace rapidrpc {

/**
 * @brief FdEvent class
 * 表示一个fd的事件，包括对应的回调函数
 *
 */
class FdEvent {

public:
    enum class TriggerEvent {
        IN_EVENT = EPOLLIN,   // 读事件
        OUT_EVENT = EPOLLOUT, // 写事件
    };

    FdEvent(int fd);
    ~FdEvent();

protected:
    // * only can be called by derived class Timer
    FdEvent();

public:
    /**
     * @brief return callback function
     */
    std::function<void()> getHandler(TriggerEvent even_type);

    /**
     * @brief 设置监听事件和回调函数
     * @param event: 监听事件，这里是原基础上添加的事件
     * @param callback: 该事件的回调函数
     */
    void listen(TriggerEvent event_type, std::function<void()> callback);

    int getFd() const {
        return m_fd;
    }

    epoll_event getEpollEvent() {
        return m_listen_events;
    }

    void close();

protected:
    // fd
    int m_fd{-1};
    // listen event
    epoll_event m_listen_events;

    std::function<void()> m_read_callback;
    std::function<void()> m_write_callback;
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_FD_EVENT_H