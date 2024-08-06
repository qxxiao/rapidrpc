#include "net/wakeup_fd_event.h"
#include "common/log.h"

#include <unistd.h>
#include <string.h>

namespace rapidrpc {

WakeupFdEvent::WakeupFdEvent(int fd) : FdEvent(fd) {
    // set wakeup_fd's read callback
    init();
}

WakeupFdEvent::~WakeupFdEvent() {}

// wakeup_fd is nonblocking
// !!! 这里的初始化、绑定需要和 FDEvent 的 listen 函数功能相同
void WakeupFdEvent::init() {
    // memset(&m_listen_events, 0, sizeof(m_listen_events));
    m_listen_events.events = EPOLLIN;
    m_read_callback = [this]() {
        // read wakeup_fd to clear
        char buf[8];
        ssize_t rt = 0;
        // read and clear counter
        while ((rt = read(m_fd, buf, sizeof(buf))) == -1 && errno == EAGAIN)
            ;
        if (rt < 0) { // error： EINVAL, never happen
            ERRORLOG("WakeupFdEvent read callback failed, read less than 8 bytes");
        }
        // 每次跨线程添加 fd， 唤醒后，会回调本函数清空 wakeup_fd
        DEBUGLOG("WakeupFdEvent read callback finished, read full %ld bytes", rt);
    };
    m_listen_events.data.ptr = this;
}

// wakeup_fd is nonblocking - EAGAIN
void WakeupFdEvent::wakeup() {

    uint64_t one = 1;
    // ssize_t rt = writen(m_fd, &one, sizeof(one));
    // if (rt != sizeof(one)) {
    //     ERRORLOG("WakeupFdEvent write wakeup failed,  write less than 8 bytes");
    // }
    ssize_t rt = 0;
    while ((rt = write(m_fd, &one, sizeof(one))) == -1 && errno == EAGAIN)
        ;
    if (rt < 0) { // error： EINVAL, never happen
        ERRORLOG("WakeupFdEvent write wakeup failed,  write less than 8 bytes");
    }
}

} // namespace rapidrpc