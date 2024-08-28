#ifndef RAPIDRPC_NET_FD_EVENT_H
#define RAPIDRPC_NET_FD_EVENT_H

#include <functional>
#include <sys/epoll.h>

namespace rapidrpc {

enum class TriggerEvent {
    IN_EVENT = EPOLLIN,   // 读事件
    OUT_EVENT = EPOLLOUT, // 写事件
};

/**
 * @brief FdEvent class
 * 表示一个fd的事件，包括对应的回调函数
 *
 */
class FdEvent {

public:
    FdEvent(int fd);
    ~FdEvent();

    // set non-blocking
    void setNonBlocking();

protected:
    // 只用来构造空对象，用于 Timer 类的构造函数
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

    /**
     * @brief 关闭fd
     */
    void close();

    /**
     * @brief 取消监听 TriggerEvent 事件和清空回调函数
     */
    void clearEvent(TriggerEvent event_type);

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