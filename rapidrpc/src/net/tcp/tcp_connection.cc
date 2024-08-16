#include "net/tcp/tcp_connection.h"
#include "net/fd_event_group.h"
#include "common/log.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

namespace rapidrpc {

TcpConnection::TcpConnection(IOThread *io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr)
    : m_peer_addr(peer_addr), m_io_thread(io_thread) {

    m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

    // set non-blocking
    // ! 如果需要一次读完，非阻塞模式更容易判断；阻塞使用超时时间或者使用上层协议格式
    m_fd_event = FdEventGroup::GetGlobalFdEventGroup()->getFdEvent(fd);
    m_fd_event->setNonBlocking();
    // 设置读事件回调函数
    m_fd_event->listen(TriggerEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
    // !!添加到 epoll 中
    m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);
    m_state = TcpState::Connected;
}

TcpConnection::~TcpConnection() {}

void TcpConnection::onRead() {
    // 读取数据
    if (m_state != TcpState::Connected) {
        ERRORLOG("TcpConnection::read, state is not connected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(),
                 m_fd_event->getFd());
        return;
    }
    // 读取数据, 非阻塞模式下尽可能读取；（如果是阻塞模式由于是 LT 模式，会一直触发，直到读完）
    while (true) {
        //
        if (m_in_buffer->writeAvailable() == 0) {
            // buffer is full
            m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
        }
        // read data from socket
        int write_index = m_in_buffer->writeIndex();
        int free_len = m_in_buffer->writeAvailable(); // 最多读取 free_len 字节, 保证 moveWriteIndex 也不会越界
        int n = read(m_fd_event->getFd(), &m_in_buffer->m_buffer[write_index], free_len);

        DEBUGLOG("success read %d bytes from addr[%s], clientfd[%d]", n, m_peer_addr->toString().c_str(),
                 m_fd_event->getFd());

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 当前没有数据可读，退出循环
                break;
            }
            else {
                ERRORLOG("Error on read, err[%s]", strerror(errno));
                return;
            }
        }
        else if (n == 0) {
            // 对端关闭连接
            m_state = TcpState::Closed;
        }
        // move write index
        m_in_buffer->moveWriteIndex(n); // n>=0

        if (n < free_len) {
            // 读取完毕
            break;
        }
    }
    // ! Close the connection
    if (m_state == TcpState::Closed) {
        // TODO: 关闭连接, 调用 fd_event 的 close
        m_io_thread->getEventLoop()->deleteEpollEvent(m_fd_event);
        m_fd_event->close();
        if (m_remove_conn_cb) {
            m_remove_conn_cb();
        }
        INFOLOG("peer close connection, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd_event->getFd());
        return;
    }

    execute();
}

// * 执行请求
// ! 第一次有数据写入时，重新添加到 epoll 中，监听可写事件
// ！ 并且在发送完后，清除写入事件，避免可写事件一直被触发
void TcpConnection::execute() {
    // TODO: 读取请求，应该需要判断完整的请求，然后执行
    int len = m_in_buffer->readAvailable();
    std::vector<char> data(len);
    m_in_buffer->readFromBuffer(data, len);

    std::string msg(data.begin(), data.end());

    // TODO: 解析请求
    INFOLOG("success get request from client[%s], request[%s]", m_peer_addr->toString().c_str(), msg.c_str());

    m_out_buffer->writeToBuffer(msg.data(), msg.size());
    // TODO: 可写，但是需要在写之前判断是否有数据
    m_fd_event->listen(TriggerEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
    m_io_thread->getEventLoop()->addEpollEvent(m_fd_event); //!!重新添加到 epoll 中(修改监听的事件)
}

void TcpConnection::onWrite() {
    INFOLOG("call TcpConnection::onWrite on fd[%d]", m_fd_event->getFd());

    if (m_state != TcpState::Connected) {
        ERRORLOG("TcpConnection::onWrite, state is not connected, addr[%s], clientfd[%d]",
                 m_peer_addr->toString().c_str(), m_fd_event->getFd());
        return;
    }

    // write data to socket
    if (m_out_buffer->readAvailable() == 0)
        return;

    // 尽量发送完数据, 直到socket 发送缓冲区满，或者数据发送完毕
    // 另一种策略，只写入一次，然后等待下次写事件
    while (true) {
        int read_index = m_out_buffer->readIndex();
        int len = m_out_buffer->readAvailable();
        int n = write(m_fd_event->getFd(), &m_out_buffer->m_buffer[read_index], len);

        DEBUGLOG("success write %d bytes to addr[%s], clientfd[%d]", n, m_peer_addr->toString().c_str(),
                 m_fd_event->getFd());

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 发送缓冲区已满，当前不可写，退出循环，等待下次写事件
                break;
            }
            else {
                ERRORLOG("Error on write, err[%s]", strerror(errno));
                return;
            }
        }
        // move read index
        m_out_buffer->moveReadIndex(n); // n>=0

        if (n >= len) {
            // 写完数据
            // clear write event
            m_fd_event->clearEvent(TriggerEvent::OUT_EVENT);
            // 重新添加到 epoll 中, 已经存在的事件会被更新
            m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);
            break;
        }
        // n<len，尝试继续写
    }
}

void TcpConnection::setState(const TcpState state) {
    m_state = state;
}
TcpState TcpConnection::getState() const {
    return m_state;
}

void TcpConnection::shutdown() {
    if (m_state == TcpState::Closed || m_state == TcpState::NotConnected) {
        return;
    }
    m_state = TcpState::HalfClosed;

    // shut_rdwr 关闭了读写，对端会收到 FIN
    ::shutdown(m_fd_event->getFd(), SHUT_RDWR);
    m_io_thread->getEventLoop()->deleteEpollEvent(m_fd_event);
    m_fd_event->close();
    if (m_remove_conn_cb) {
        m_remove_conn_cb();
    }
}

void TcpConnection::setRemoveConnCb(std::function<void()> &&remove_conn_cb) {
    m_remove_conn_cb = std::move(remove_conn_cb);
}

} // namespace rapidrpc