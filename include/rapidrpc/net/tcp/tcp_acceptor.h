#ifndef RAPIDRPC_NET_TCP_TCP_ACCEPTOR_H
#define RAPIDRPC_NET_TCP_TCP_ACCEPTOR_H

#include "rapidrpc/net/tcp/net_addr.h"
#include <memory>

namespace rapidrpc {

class TcpAcceptor {

public:
    using s_ptr = std::shared_ptr<TcpAcceptor>;
    /**
     * Constructor
     * @param paddr Address to bind to
     * @param backlog Maximum number of pending connections
     * @note set Non-blocking(by fd_event) and Reuse address for Eventloop use
     */
    TcpAcceptor(const NetAddr::s_ptr paddr, int backlog = 1000);
    ~TcpAcceptor();

    // Accept a connection, return the client file descriptor
    int accept(NetAddr &clientAddr);

    // Get the listening file descriptor
    int getListenFd() const {
        return m_listenfd;
    }

    // Get the address family
    int getFamily() const {
        return m_family;
    }

private:
    NetAddr::s_ptr m_addr; // Address to bind to
    int m_family{-1};      // Address family

    int m_listenfd{-1}; // Listening file descriptor
    int m_backlog{-1};  // Maximum number of pending connections
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_TCP_TCP_ACCEPTOR_H
