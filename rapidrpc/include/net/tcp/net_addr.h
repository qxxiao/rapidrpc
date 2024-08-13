/**
 * define class NetAddr, a base class for IpNetAddr, UnixNetAddr, Ip6NetAddr
 * IpNetAddr: IPv4 address, constructor with ip and port, constructor with addr, constructor with sockaddr_in
 * UnixNetAddr: Unix domain address, constructor with path, constructor with sockaddr_un
 * Ip6NetAddr: IPv6 address, constructor with ip and port, constructor with addr, constructor with sockaddr_in6
 */

#ifndef RAPIDRPC_NET_TCP_TCP_BUFFER_H
#define RAPIDRPC_NET_TCP_TCP_BUFFER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

#include <string>
#include <memory>

namespace rapidrpc {

/**
 * Address base class for IPv4, IPv6, Unix domain
 */
class NetAddr {

public:
    using s_ptr = std::shared_ptr<NetAddr>;
    virtual sockaddr *getSockAddr() = 0;
    virtual socklen_t getSockAddrLen() = 0;

    virtual int getFamily() = 0;

    virtual std::string toString() = 0;

    // check if the address is valid
    virtual operator bool() = 0;
};

/**
 * IPv4 address
 */
class IpNetAddr: public NetAddr {
public:
    IpNetAddr() = default; // only for accept client address
    /**
     * @param ip "xxx.xxx.xxx.xxx"
     */
    IpNetAddr(const std::string &ip, uint16_t port);
    /**
     * @param addr "ip:port"
     */
    IpNetAddr(const std::string &addr);
    /**
     * @param addr sockaddr_in
     */
    IpNetAddr(const sockaddr_in &addr);

    sockaddr *getSockAddr() override;
    socklen_t getSockAddrLen() override;
    int getFamily() override;
    std::string toString() override;
    operator bool() override;

private:
    std::string m_ip;
    uint16_t m_port{0};
    sockaddr_in m_addr;
};

/**
 * Unix domain address
 */
class UnixNetAddr: public NetAddr {
public:
    /**
     * @param path Unix domain path
     */
    UnixNetAddr(const std::string &path);
    UnixNetAddr(const sockaddr_un &addr);
    UnixNetAddr() = default; // only for accept client address

    sockaddr *getSockAddr() override;
    socklen_t getSockAddrLen() override;
    int getFamily() override;
    std::string toString() override;
    operator bool() override;

private:
    sockaddr_un m_addr; // Unix domain address
    std::string m_path; // Unix domain path
};

/**
 * IPv6 address
 */
class Ip6NetAddr: public NetAddr {
public:
    Ip6NetAddr(const std::string &ip, uint16_t port);
    Ip6NetAddr(const std::string &addr);
    Ip6NetAddr(const sockaddr_in6 &addr);
    Ip6NetAddr() = default; // only for accept client address

    sockaddr *getSockAddr() override;
    socklen_t getSockAddrLen() override;
    int getFamily() override;
    std::string toString() override;
    operator bool() override;

private:
    std::string m_ip;
    uint16_t m_port{0};
    sockaddr_in6 m_addr;
};

} // namespace rapidrpc

#endif // !RAPIDRPC_NET_TCP_TCP_BUFFER_H
