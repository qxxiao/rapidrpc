#include "net/tcp/tcp_server.h"
#include "common/log.h"

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
    // TODO: 读取配置文件和配置类
    m_io_thread_group = new IOThreadGroup(2);

    // FdEvent listen_fd_event(m_acceptor->getListenFd());
    m_listen_fd_event = new FdEvent(m_acceptor->getListenFd());
    m_listen_fd_event->listen(FdEvent::TriggerEvent::IN_EVENT, std::bind(&TcpServer::onAccept, this));
    // add listen_fd_event to mainReactor
    m_main_event_loop->addEpollEvent(m_listen_fd_event);
}

void TcpServer::onAccept() {
    // client addr, IpNetAddr/IpV6NetAddr/UnixNetAddr for different family
    // IpNetAddr client_addr;
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
    m_client_counts++;
    // TODO: add client_fd to subReactor
    // test...
}

void TcpServer::start() {
    m_io_thread_group->start(); // 启动所有子线程的 EventLoop
    m_main_event_loop->loop();  // 启动主线程的 EventLoop
};

} // namespace rapidrpc