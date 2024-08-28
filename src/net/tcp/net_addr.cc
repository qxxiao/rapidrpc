// inet_pton(): 将支持的地址格式（IPv4或IPv6）的字符串转换为相应的网络字节序的二进制格式。inet_addr() 和 inet_aton() 是
//              IPv4 版本的
// inet_ntop(): 将网络字节序的二进制IP地址转换为支持的地址格式（IPv4或IPv6）的字符串。
// htons() 和 htonl(): 这两个函数用于将主机字节序的端口号（或其他16位或32位整数）转换为网络字节序。
//                     htons()用于16位整数，而htonl()用于32位整数。
// ntohs() 和 ntohl(): 这两个函数是htons()和htonl()的逆操作，它们将网络字节序的整数转换回主机字节序。

#include "rapidrpc/net/tcp/net_addr.h"
#include "rapidrpc/common/log.h"

#include <string.h>
#include <unistd.h>

namespace rapidrpc {

/**
 * IPv4 address
 */

IpNetAddr::IpNetAddr(const std::string &ip, uint16_t port) : m_ip(ip), m_port(port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET; // IPv4
    m_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &m_addr.sin_addr) <= 0) {
        ERRORLOG("Invalid address: %s", ip.c_str());
        m_ip.clear();
    }
}

IpNetAddr::IpNetAddr(const std::string &addr) {
    memset(&m_addr, 0, sizeof(m_addr));
    // "ip:port"
    size_t pos = addr.find_last_of(':');
    if (pos == std::string::npos) {
        ERRORLOG("Invalid address: %s", addr.c_str());
        return;
    }
    m_ip = addr.substr(0, pos);
    m_port = std::stoi(addr.substr(pos + 1));
    m_addr.sin_family = AF_INET; // IPv4
    m_addr.sin_port = htons(m_port);
    if (inet_pton(AF_INET, m_ip.c_str(), &m_addr.sin_addr) <= 0) {
        ERRORLOG("Invalid address: %s", m_ip.c_str());
        m_ip.clear();
    }
}

IpNetAddr::IpNetAddr(const sockaddr_in &addr) : m_addr(addr) {
    char buf[INET_ADDRSTRLEN];
    m_ip = inet_ntop(AF_INET, &addr.sin_addr, &buf[0], INET_ADDRSTRLEN);
    m_port = ntohs(addr.sin_port);
    // m_ip = inet_ntoa(addr.sin_addr);
}

sockaddr *IpNetAddr::getSockAddr() {
    return reinterpret_cast<sockaddr *>(&m_addr);
}

socklen_t IpNetAddr::getSockAddrLen() {
    return sizeof(m_addr);
}
int IpNetAddr::getFamily() {
    return m_addr.sin_family; // unsigned short int
}
std::string IpNetAddr::toString() {
    return m_ip + ":" + std::to_string(m_port);
}

IpNetAddr::operator bool() {
    if (m_ip.empty() || m_port < 0 || m_port > 65535) {
        return false;
    }

    if (m_addr.sin_family != AF_INET) {
        return false;
    }

    // if (inet_addr(m_ip.c_str()) == INADDR_NONE) {
    //     return false;
    // }
    if (inet_pton(AF_INET, m_ip.c_str(), &m_addr.sin_addr) <= 0) {
        return false;
    }
    return true;
}

/**
 * Unix domain address
 */

UnixNetAddr::UnixNetAddr(const std::string &path) : m_path(path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    if (path.size() >= sizeof(m_addr.sun_path)) {
        ERRORLOG("Path too long: %s", path.c_str());
    }
    strncpy(m_addr.sun_path, path.c_str(), sizeof(m_addr.sun_path) - 1);
}

UnixNetAddr::UnixNetAddr(const sockaddr_un &addr) : m_addr(addr) {
    m_path = addr.sun_path;
}

sockaddr *UnixNetAddr::getSockAddr() {
    return reinterpret_cast<sockaddr *>(&m_addr);
}
socklen_t UnixNetAddr::getSockAddrLen() {
    return sizeof(m_addr);
}
int UnixNetAddr::getFamily() {
    return m_addr.sun_family;
}
std::string UnixNetAddr::toString() {
    return m_path;
}
UnixNetAddr::operator bool() {
    if (m_path.empty()) {
        return false;
    }

    if (m_addr.sun_family != AF_UNIX) {
        return false;
    }

    // check if it is a valid path and not exist
    if (access(m_path.c_str(), F_OK) == 0) {
        return false;
    }

    return true;
}

/**
 * IPv6 address
 */

Ip6NetAddr::Ip6NetAddr(const std::string &ip, uint16_t port) : m_ip(ip), m_port(port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6; // IPv6
    m_addr.sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip.c_str(), &m_addr.sin6_addr) <= 0) {
        ERRORLOG("Invalid address: %s", ip.c_str());
        m_ip.clear();
    }
}

Ip6NetAddr::Ip6NetAddr(const std::string &addr) {
    memset(&m_addr, 0, sizeof(m_addr));
    // "ip:port"
    size_t pos = addr.find_last_of(':');
    if (pos == std::string::npos) {
        ERRORLOG("Invalid address: %s", addr.c_str());
        return;
    }
    m_ip = addr.substr(0, pos);
    m_port = std::stoi(addr.substr(pos + 1));
    m_addr.sin6_family = AF_INET6; // IPv6
    m_addr.sin6_port = htons(m_port);
    if (inet_pton(AF_INET6, m_ip.c_str(), &m_addr.sin6_addr) <= 0) {
        ERRORLOG("Invalid address: %s", m_ip.c_str());
        m_ip.clear();
    }
}

Ip6NetAddr::Ip6NetAddr(const sockaddr_in6 &addr) : m_addr(addr) {
    char buf[INET6_ADDRSTRLEN]; // {0} is not necessary, inet_ntop will add '\0' at the end
    m_ip = inet_ntop(AF_INET6, &addr.sin6_addr, &buf[0], INET6_ADDRSTRLEN);
    m_port = ntohs(addr.sin6_port);
}

sockaddr *Ip6NetAddr::getSockAddr() {
    return reinterpret_cast<sockaddr *>(&m_addr);
}

socklen_t Ip6NetAddr::getSockAddrLen() {
    return sizeof(m_addr);
}

int Ip6NetAddr::getFamily() {
    return m_addr.sin6_family;
}

std::string Ip6NetAddr::toString() {
    return "[" + m_ip + "]:" + std::to_string(m_port);
}

Ip6NetAddr::operator bool() {
    if (m_ip.empty() || m_port < 0 || m_port > 65535) {
        return false;
    }

    if (m_addr.sin6_family != AF_INET6) {
        return false;
    }
    // 0 is invalid， -1 is error
    if (inet_pton(AF_INET6, m_ip.c_str(), &m_addr.sin6_addr) <= 0) {
        return false;
    }
    return true;
}

} // namespace rapidrpc