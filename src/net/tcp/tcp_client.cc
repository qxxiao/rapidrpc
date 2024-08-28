#include "rapidrpc/common/log.h"
#include "rapidrpc/common/error_code.h"
#include "rapidrpc/net/tcp/tcp_client.h"
#include "rapidrpc/net/fd_event_group.h"

namespace rapidrpc {

TcpClient::TcpClient(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    // ! 通过 EventLoop::GetCurrentEventLoop() 获取当前线程的 EventLoop
    m_event_loop = EventLoop::GetCurrentEventLoop();

    if (!m_peer_addr) {
        ERRORLOG("TcpClient::TcpClient, peer_addr is invalid");
        return;
    }
    // 创建客户端 socket
    m_fd = socket(m_peer_addr->getFamily(), SOCK_STREAM, 0);
    if (m_fd < 0) {
        ERRORLOG("TcpClient::TcpClient, create socket failed, err[%s]", strerror(errno));
        return;
    }

    // 设置非阻塞
    m_fd_event = FdEventGroup::GetGlobalFdEventGroup()->getFdEvent(m_fd);
    m_fd_event->setNonBlocking(); // 可重入
    // TODO: 其中设置了非阻塞，绑定了 m_fd 的读事件等
    m_connection = std::make_shared<TcpConnection>(m_event_loop, m_fd, 1024, m_peer_addr,
                                                   TcpConnectionType::TcpConnectionByClient);
}

TcpClient::~TcpClient() {
    // TODO: close m_fd manually in close() function

    // if (m_fd > 0) {
    //     m_fd_event->close();
    // }
    DEBUGLOG("TcpClient::~TcpClient");
}

// 异步的连接远程地址 connect to peer_addr
// 只有 返回 -1 && errno == EINPROGRESS 时，才会注册 OUT_EVENT 事件来监听连接是否成功
// 最后清除 OUT_EVENT 事件
void TcpClient::connect(std::function<void()> conn_cb) {

    int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockAddrLen());
    // rt = 0, connect success || rt < 0, errno == EINPROGRESS, connect in progress
    if (rt == 0) {
        // TODO: 需要加锁吗？
        m_connection->setState(TcpState::Connected);
        m_connect_error_code = static_cast<int>(Error::OK);
        initLocalAddr();
        DEBUGLOG("TcpClient::connect, connect [%s] success", m_peer_addr->toString().c_str());
        if (conn_cb) {
            conn_cb();
        }
    }
    // ! 非阻塞需要通过可写事件回调中判断是否连接成功
    else { // -1
        if (errno != EINPROGRESS) {
            ERRORLOG("TcpClient::connect, connect failed, err[%s]", strerror(errno));
            this->m_connect_error_code = static_cast<int>(Error::SYS_FAILED_CONNECT);
            this->m_connect_error_info = strerror(errno);
            if (conn_cb) {
                conn_cb();
            }
            return;
        }
        // connect in progress
        // !! 连接前的一次性事件，连接成功或失败后会清除
        m_fd_event->listen(TriggerEvent::OUT_EVENT, [this, conn_cb]() {
            // check connect error by connect again
            int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockAddrLen());
            if ((rt < 0 && errno == EISCONN) || rt == 0) {
                // 连接成功
                // TODO: 需要加锁吗？还没有建立连接时，会不会有其他线程访问 m_connection 例如发送数据请求时
                m_connection->setState(TcpState::Connected);
                m_connect_error_code = static_cast<int>(Error::OK);
                initLocalAddr();
                DEBUGLOG("TcpClient::connect, connect [%s] success", m_peer_addr->toString().c_str());
            }
            else {
                // 连接失败, connection refused, timeout, etc.
                if (errno == ECONNREFUSED)
                    m_connect_error_code = static_cast<int>(Error::SYS_PEER_CLOSED);
                else
                    m_connect_error_code = static_cast<int>(Error::SYS_FAILED_CONNECT);
                m_connect_error_info = strerror(errno);
                ERRORLOG("TcpClient::connect, connect failed, err[%s]", strerror(errno));
            }

            // 连接失败直接清除该 fd 监听
            // 连接成功，也通过 delete 来清除 OUT_EVENT 事件
            this->m_event_loop->deleteEpollEvent(m_fd_event);

            // !! 连接成功时，需要先清空 OUT_EVENT 事件后再执行回调函数
            // !! 否则会导致回调函数中的写事件无法注册/被上面的覆盖(在回调函数中使用 writeMessage
            // 发送数据时会注册写事件)
            // 不管连接成功与否，都会调用回调函数
            if (conn_cb) {
                conn_cb();
            }
        });
        m_event_loop->addEpollEvent(m_fd_event);
    }

    if (!m_event_loop->isLooping()) {
        m_event_loop->loop();
    }
}

// 异步的发送数据
void TcpClient::writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> write_cb) {
    if (!message)
        return;
    // 1. 写入到 TcpConnection 的等待队列
    m_connection->addMessage(message, write_cb);
    // 2. 注册写事件，等待写事件触发
    // !! 类似于服务端连接，都是在有数据时添加写事件，发送完毕后清除写事件
    m_connection->listenWriteEvent();
}

// 异步的读取数据
void TcpClient::readMessage(const std::string &msg_id, std::function<void(AbstractProtocol::s_ptr)> read_cb) {
    // 1. 监听可读事件
    // 2. 读取数据，并解码出 message 对象

    m_connection->addReadCb(msg_id, read_cb);
    // TODO: 每次读取后，需要清除读事件吗？
    m_connection->listenReadEvent();
}

// TODO: 每次rpc调用完毕后，需要关闭连接，当前只能一次rpc调用
// 在 eventloop 事件回调函数中执行
void TcpClient::close() {
    if (m_event_loop->isLooping()) {
        m_event_loop->stop();
        m_fd_event->close();
    }
}

int TcpClient::getConnectErrorCode() const {
    return m_connect_error_code;
}

std::string TcpClient::getConnectErrorInfo() const {
    return m_connect_error_info;
}

NetAddr::s_ptr TcpClient::getPeerAddr() const {
    return m_peer_addr;
}

NetAddr::s_ptr TcpClient::getLocalAddr() const {
    return m_local_addr;
}

void TcpClient::initLocalAddr() {
    if (m_connect_error_code != static_cast<int>(Error::OK) || m_connection->getState() != TcpState::Connected) {
        return;
    }
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int rt = getsockname(m_fd, reinterpret_cast<sockaddr *>(&addr), &len);
    m_local_addr = std::make_shared<IpNetAddr>(addr);
    if (rt < 0) {
        ERRORLOG("TcpClient::initLocalAddr, getsockname failed, err[%s]", strerror(errno));
        m_local_addr.reset();
    }
}

void TcpClient::addTimerEvent(TimerEvent::s_ptr timer_event) {
    m_event_loop->addTimerEvent(timer_event);
}

void TcpClient::deleteTimerEvent(TimerEvent::s_ptr timer_event) {
    m_event_loop->deleteTimerEvent(timer_event);
}

} // namespace rapidrpc