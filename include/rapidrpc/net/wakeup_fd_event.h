/**
 * wakeup fd to FdEvent
 */

#ifndef RAIPDRPC_NET_WAKEUP_FD_EVENT_H
#define RAIPDRPC_NET_WAKEUP_FD_EVENT_H

#include "rapidrpc/net/fd_event.h"

namespace rapidrpc {

class WakeupFdEvent: public FdEvent {

public:
    /**
     * @brief wakeup fd event
     * @param fd: wakeup fd
     * @note 自动完成了fd监听的读事件和读取回调函数的设置，调用 wakeup() 函数可以唤醒 epoll_wait。无需调用 listen() 函数
     */
    WakeupFdEvent(int fd);
    ~WakeupFdEvent();

    // disable listen, automatically set read event/read callback in constructor
    void listen(TriggerEvent event_type, std::function<void()> callback) = delete;

    // bind fd with read callback
    void init();

    void wakeup();

private:
};
} // namespace rapidrpc

#endif // !RAIPDRPC_NET_WAKEUP_FD_EVENT_H