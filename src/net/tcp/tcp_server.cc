#include "rapidrpc/net/tcp/tcp_server.h"
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/config.h"
#include "rapidrpc/net/fd_event_group.h"

namespace rapidrpc {

TcpServer::TcpServer(NetAddr::s_ptr local_addr) : m_local_addr(local_addr) {

    init();

    INFOLOG("RapidRPC server started, listen success on [%s]", m_local_addr->toString().c_str());
};

TcpServer::~TcpServer() {
    if (m_io_thread_group) {
        delete m_io_thread_group;
        m_io_thread_group = nullptr;
    }
    if (m_main_event_loop) {
        delete m_main_event_loop;
        m_main_event_loop = nullptr;
    }
};

void TcpServer::init() {
    m_acceptor = std::make_shared<TcpAcceptor>(m_local_addr); // 创建一个 Acceptor 对象, bind and listen
    m_main_event_loop = EventLoop::GetCurrentEventLoop();     // mainReactor 静态创建一个Loop

    // 读取配置， 创建 IOThreadGroup 对象
    m_io_thread_group = new IOThreadGroup(Config::GetGlobalConfig()->m_io_threads);

    m_listen_fd_event = FdEventGroup::GetGlobalFdEventGroup()->getFdEvent(m_acceptor->getListenFd());
    // ! set non-blocking
    m_listen_fd_event->setNonBlocking();
    m_listen_fd_event->listen(TriggerEvent::IN_EVENT, std::bind(&TcpServer::onAccept, this));
    // add listen_fd_event to mainReactor
    m_main_event_loop->addEpollEvent(m_listen_fd_event);
}

void TcpServer::onAccept() {
    // nothing to do with client_addr, just for accept
    NetAddr::s_ptr client_addr;
    if (m_acceptor->getFamily() == AF_INET) {
        client_addr = std::make_shared<IpNetAddr>();
    }
    else if (m_acceptor->getFamily() == AF_INET6) {
        client_addr = std::make_shared<Ip6NetAddr>();
    }
    else {
        client_addr = std::make_shared<UnixNetAddr>();
    }
    int client_fd = m_acceptor->accept(*client_addr); // accept connection
    if (client_fd < 0) {
        ERRORLOG("Failed to accept connection");
        return;
    }
    // add conn(client_fd) to IOThread
    IOThread *io_thread = m_io_thread_group->getIOThread(); // get next IOThread
    TcpConnection::s_ptr conn =
        std::make_shared<TcpConnection>(io_thread->getEventLoop(), client_fd, 1024, client_addr);
    conn->setState(TcpState::Connected);
    // ! set callback
    conn->setRemoveConnCb(std::bind(&TcpServer::removeConnection, this, TcpConnection::w_ptr(conn)));
    // add connection to set and increase client counts
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_client_counts++;
        m_connections.insert(conn);
    }
}

void TcpServer::start() {
    m_io_thread_group->start(); // 启动所有子线程的 EventLoop
    m_main_event_loop->loop();  // 启动主线程的 EventLoop
};

void TcpServer::removeConnection(TcpConnection::w_ptr conn) {
    auto tmp_ptr = conn.lock();
    if (!tmp_ptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    int rt = m_connections.erase(tmp_ptr);
    m_client_counts--;
    DEBUGLOG("TcpServer remove %d connection, client counts: %d", rt, m_client_counts);
}

} // namespace rapidrpc