#include "rapidrpc/net/tcp/tcp_acceptor.h"
#include "rapidrpc/common/log.h"

#include <fcntl.h>

namespace rapidrpc {

TcpAcceptor::TcpAcceptor(const NetAddr::s_ptr paddr, int backlog) : m_addr(paddr), m_backlog(backlog) {
    if (!paddr || !*m_addr) {
        ERRORLOG("Invalid address");
        exit(-1);
    }
    m_family = m_addr->getFamily();

    m_listenfd = socket(m_family, SOCK_STREAM, 0); // tcp
    if (m_listenfd < 0) {
        ERRORLOG("Failed to create socket");
        exit(-1);
    }

    int opt = 1;
    if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { // reuse addr
        ERRORLOG("Failed to set reuse addr");
    }
    // nonblock
    // if (isNonBlock) {
    //     int flags = fcntl(m_listenfd, F_GETFL, 0);                // get old flags
    //     if (fcntl(m_listenfd, F_SETFL, flags | O_NONBLOCK) < 0) { // set nonblock
    //         ERRORLOG("Failed to set nonblock");
    //         exit(-1);
    //     }
    // }

    if (bind(m_listenfd, m_addr->getSockAddr(), m_addr->getSockAddrLen()) < 0) {
        ERRORLOG("Failed to bind socket");
        exit(-1);
    }

    if (listen(m_listenfd, m_backlog) < 0) {
        ERRORLOG("Failed to listen on socket");
        exit(-1);
    }
}
TcpAcceptor::~TcpAcceptor() {}

int TcpAcceptor::accept(NetAddr &clientAddr) {
    if (m_family == AF_INET) {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int clientfd = ::accept(m_listenfd, (sockaddr *)&addr, &len);
        // TODO: non-blocking
        if (clientfd < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
            ERRORLOG("Failed to accept connection");
            return -1;
        }

        static_cast<IpNetAddr &>(clientAddr) = IpNetAddr(addr);

        INFOLOG("Accept connection from %s", clientAddr.toString().c_str());
        return clientfd;
    }

    if (m_family == AF_UNIX) {
        sockaddr_un addr;
        socklen_t len = sizeof(addr);
        int clientfd = ::accept(m_listenfd, (sockaddr *)&addr, &len);
        if (clientfd < 0) {
            ERRORLOG("Failed to accept connection");
            return -1;
        }

        static_cast<UnixNetAddr &>(clientAddr) = UnixNetAddr(addr);
        INFOLOG("Accept connection from %s", clientAddr.toString().c_str());
        return clientfd;
    }

    if (m_family == AF_INET6) {
        sockaddr_in6 addr;
        socklen_t len = sizeof(addr);
        int clientfd = ::accept(m_listenfd, (sockaddr *)&addr, &len);
        if (clientfd < 0) {
            ERRORLOG("Failed to accept connection");
            return -1;
        }

        static_cast<Ip6NetAddr &>(clientAddr) = Ip6NetAddr(addr);

        INFOLOG("Accept connection from %s", clientAddr.toString().c_str());
        return clientfd;
    }
    ERRORLOG("Unknown address family");
    return -1;
}
} // namespace rapidrpc