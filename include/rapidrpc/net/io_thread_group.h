/**
 * @file io_thread_group.h
 * @brief IOThreadGroup 类，用于管理多个 IOThread 对象( SubReactor 线程)
 */

#ifndef RAPIDRPC_NET_IO_THREAD_GROUP_H
#define RAPIDRPC_NET_IO_THREAD_GROUP_H

#include "rapidrpc/net/io_thread.h"

#include <vector>

namespace rapidrpc {

/**
 * @brief IOThreadGroup 类，用于管理多个 IOThread 对象( SubReactor 线程)
 * @note 只在主线程中使用，用于管理多个 IOThread 对象/SubReactor 线程
 */
class IOThreadGroup {
public:
    IOThreadGroup(int size);
    ~IOThreadGroup();

    void start();
    void join();

    IOThread *getIOThread();

private:
    int m_size{0};                             // 线程数量
    std::vector<IOThread *> m_io_thread_group; // 线程对象数组

    // 用于轮询选择线程
    int m_index{0}; // 当前 getIOThread() 返回的线程索引
};
} // namespace rapidrpc

#endif