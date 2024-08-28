/**
 * @file fd_event_group.h
 * 使用全局对象，管理所有的fd_event，fd 一般是顺序分配的，所以可以使用vector来管理
 * 避免fd_event对象的频繁创建，管理生命周期
 */

#ifndef RAPIDRPC_NET_FD_EVENT_GROUP_H
#define RAPIDRPC_NET_FD_EVENT_GROUP_H

#include "rapidrpc/net/fd_event.h"

#include <vector>
#include <mutex>

namespace rapidrpc {

class FdEventGroup {

public:
    FdEventGroup(int size);
    ~FdEventGroup();

    /**
     * @brief 获取全局的 FdEventGroup, thread safe
     */
    static FdEventGroup *GetGlobalFdEventGroup();

    /**
     * @brief 获取 fd 对应的 FdEvent
     * 如果 fd 不存在，增加 vector 的大小并添加新的 FdEvent
     */
    FdEvent *getFdEvent(int fd);

private:
    std::vector<FdEvent *> m_fd_event_group;
    std::mutex m_mutex; // 保护 m_fd_event_group
};
} // namespace rapidrpc

#endif