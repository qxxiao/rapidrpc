#include "net/fd_event_group.h"

namespace rapidrpc {

// 全局对象
static FdEventGroup *g_fd_event_group = nullptr;
FdEventGroup *FdEventGroup::GetGlobalFdEventGroup() {
    if (g_fd_event_group) {
        return g_fd_event_group;
    }
    // local static variable, thread safe
    static FdEventGroup fd_event_group(128); // 1024个fd_event
    g_fd_event_group = &fd_event_group;
    return g_fd_event_group;
}

FdEventGroup::FdEventGroup(int size) : m_fd_event_group(size, nullptr) {

    // fd 0~size-1
    for (int i = 0; i < size; i++) {
        m_fd_event_group[i] = new FdEvent(i);
    }
}

FdEventGroup::~FdEventGroup() {
    for (size_t i = 0; i < m_fd_event_group.size(); i++) {
        if (m_fd_event_group[i]) {
            delete m_fd_event_group[i];
            m_fd_event_group[i] = nullptr;
        }
    }
}

FdEvent *FdEventGroup::getFdEvent(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if ((size_t)fd >= m_fd_event_group.size()) {
        // resize
        int old_size = m_fd_event_group.size();
        int new_size = fd * 1.5;
        m_fd_event_group.resize(new_size, nullptr);
        for (int i = old_size; i < new_size; i++) {
            m_fd_event_group[i] = new FdEvent(i);
        }
    }
    // 如果是 new fd/ 或者释放后重新获取的(FdEvent.close)，都使用 memset fd_event
    // m_fd_event_group[fd]->m_fd = fd;
    return m_fd_event_group[fd];
}

} // namespace rapidrpc