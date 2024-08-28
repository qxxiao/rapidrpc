#include "rapidrpc/common/log.h"
#include "rapidrpc/net/io_thread_group.h"

namespace rapidrpc {

IOThreadGroup::IOThreadGroup(int size) : m_size(size) {
    m_io_thread_group.reserve(m_size);
    for (int i = 0; i < m_size; i++) {
        m_io_thread_group.push_back(new IOThread());
    }
}

IOThreadGroup::~IOThreadGroup() {
    for (auto &thread : m_io_thread_group) {
        delete thread;
    }
}

void IOThreadGroup::start() {
    for (auto &thread : m_io_thread_group) {
        thread->start();
    }
}

void IOThreadGroup::join() {
    for (auto &thread : m_io_thread_group) {
        thread->join();
    }
}

IOThread *IOThreadGroup::getIOThread() {
    int ret = m_index;
    m_index = (m_index + 1) % m_size;
    return m_io_thread_group[ret];
}

} // namespace rapidrpc