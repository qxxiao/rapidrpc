/**
 * @brief TcpConnection 类，建立的连接类，用于处理连接的读写事件
 * @note 可用于表示服务端建立的连接，也可用于表示客户端建立的连接，主要区别：
 * 1. 处理的先后逻辑不同，服务端：先读取数据，然后处理数据，最后发送数据；客户端：先发送数据，然后读取数据
 * 2. 监听的初始化事件不同，服务端：accept 建立连接后直接监听可读事件；客户端：在需要发送请求之前监听可写事件
 * 3. 服务端和客户端，在使用 BUFFER 时顺序相反，使用上没有区别
 */

#include "net/tcp/tcp_connection.h"
#include "net/fd_event_group.h"
#include "common/log.h"
#include "net/coder/string_coder.h"
#include "net/coder/tinypb_coder.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

namespace rapidrpc {

TcpConnection::TcpConnection(EventLoop *event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr,
                             TcpConnectionType conn_type /*= TcpConnectionType::TcpConnectionByServer */)
    : m_peer_addr(peer_addr), m_event_loop(event_loop), m_state(TcpState::NotConnected), m_conn_type(conn_type) {

    m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

    // set non-blocking
    // ! 如果需要一次读完，非阻塞模式更容易判断；阻塞使用超时时间或者使用上层协议格式
    m_fd_event = FdEventGroup::GetGlobalFdEventGroup()->getFdEvent(fd);
    m_fd_event->setNonBlocking();

    // m_coder = std::make_shared<StringCoder>();
    m_coder = std::make_shared<TinyPBCoder>();

    // 设置读事件回调函数并添加到 epoll 中（TcpConnectionByServer）
    if (m_conn_type == TcpConnectionType::TcpConnectionByServer) {
        listenReadEvent();
    }
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
        m_event_loop->deleteEpollEvent(m_fd_event);
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
// ! 服务端读取后，有数据写入时，重新添加到 epoll 中，监听可写事件
// ！ 并且在发送完后，清除写入事件，避免可写事件一直被触发
void TcpConnection::execute() {
    if (m_conn_type == TcpConnectionType::TcpConnectionByServer) {
        // 服务端连接
        std::vector<AbstractProtocol::s_ptr> messages;
        m_coder->decode(messages, m_in_buffer);

        if (messages.empty()) {
            return;
        }

        for (size_t i = 0; i < messages.size(); i++) {

            TinyPBProtocol::s_ptr msg = std::dynamic_pointer_cast<TinyPBProtocol>(messages[i]);
            msg->setErrCodeAndInfo(0, "success");
            msg->setPbData("hello, world");
            msg->complete();
        }
        m_coder->encode(messages, m_out_buffer);
        listenWriteEvent();
    }
    else {
        // 客户端连接
        // * 从 buffer 尽可能解析出完整的消息(可能为0)，并执行其回调函数
        std::vector<AbstractProtocol::s_ptr> messages;
        m_coder->decode(messages, m_in_buffer);
        // 处理解析出的消息，调用其回调函数
        for (size_t i = 0; i < messages.size(); i++) {
            std::string req_id = messages[i]->m_req_id;
            auto it = m_read_cb.find(req_id);
            if (it != m_read_cb.end()) {
                it->second(messages[i]);
                m_read_cb.erase(it);
            }
        }
    }
}

void TcpConnection::onWrite() {
    INFOLOG("call TcpConnection::onWrite on fd[%d]", m_fd_event->getFd());

    if (m_state != TcpState::Connected) {
        ERRORLOG("TcpConnection::onWrite, state is not connected, addr[%s], clientfd[%d]",
                 m_peer_addr->toString().c_str(), m_fd_event->getFd());
        return;
    }

    // TODO: 客户端连接协议的处理
    if (m_conn_type == TcpConnectionType::TcpConnectionByClient) {

        std::vector<AbstractProtocol::s_ptr> messages;
        // TODO: lock?
        for (size_t i = 0; i < m_write_cb.size(); i++) {
            messages.push_back(m_write_cb[i].first);
        }
        m_coder->encode(messages, m_out_buffer);
    }

    // test buffer size to send
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
            m_event_loop->addEpollEvent(m_fd_event);
            break;
        }
        // n<len，尝试继续写
    }

    // TODO: 对客户端的写入数据后的回调函数执行
    // * 只能保证将客户端原始数据编码后的数据保存到 m_out_buffer 发送缓冲区中后依次执行回调函数
    if (m_conn_type == TcpConnectionType::TcpConnectionByClient) {
        for (size_t i = 0; i < m_write_cb.size(); i++) {
            m_write_cb[i].second(m_write_cb[i].first);
        }
        m_write_cb.clear();
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
    m_event_loop->deleteEpollEvent(m_fd_event);
    m_fd_event->close();
    if (m_remove_conn_cb) {
        m_remove_conn_cb();
    }
}

void TcpConnection::setRemoveConnCb(std::function<void()> &&remove_conn_cb) {
    m_remove_conn_cb = std::move(remove_conn_cb);
}

void TcpConnection::listenWriteEvent() {
    m_fd_event->listen(TriggerEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
    m_event_loop->addEpollEvent(m_fd_event); //!!重新添加到 epoll 中(修改监听的事件)
}

void TcpConnection::listenReadEvent() {
    m_fd_event->listen(TriggerEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
    m_event_loop->addEpollEvent(m_fd_event);
}

void TcpConnection::addMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> write_cb) {
    m_write_cb.emplace_back(message, write_cb);
}

void TcpConnection::addReadCb(const std::string &req_id, std::function<void(AbstractProtocol::s_ptr)> read_cb) {
    m_read_cb[req_id] = read_cb;
}

} // namespace rapidrpc