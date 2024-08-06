#include "net/fd_event.h"
#include "common/log.h"

#include <unistd.h>
#include <string.h>

namespace rapidrpc {

FdEvent::FdEvent(int fd) : m_fd(fd) {
    if (m_fd < 0) {
        ERRORLOG("Invalid fd[%d]", m_fd);
    }
    // ! 初始化 m_listen_events, 避免监听事件未设置
    memset(&m_listen_events, 0, sizeof(m_listen_events));
}

FdEvent::~FdEvent() {
    // if (m_fd > 0) {
    //     close(m_fd);
    // }
}

// * 应该只能被调用一次
std::function<void()> FdEvent::getHandler(TriggerEvent event) {
    if (event == TriggerEvent::IN_EVENT) {
        // read callback
        return m_read_callback;
    }
    else {
        // write callback
        return m_write_callback;
    }
}
void FdEvent::listen(TriggerEvent event, std::function<void()> callback) {
    if (event == TriggerEvent::IN_EVENT) {
        m_listen_events.events |= EPOLLIN;
        // m_read_callback = callback;
        m_read_callback = std::move(callback);
    }
    else {
        m_listen_events.events |= EPOLLOUT;
        // m_write_callback = callback;
        m_write_callback = std::move(callback);
    }
    //! 同时设置 data.ptr 为 this 指针，用于在 epoll_wait 时获取到对应的 FdEvent 对象
    // 将 epoll_event{events,data} 与 本对象绑定
    // 通常会设置 ev.data.fd = fd(这里用 FdEvent 包装类型), 都是为了在 epoll_wait 返回时知道是哪个fd触发了事件
    m_listen_events.data.ptr = this; // set the pointer to this object
}

} // namespace rapidrpc