#include "net/tcp/net_addr.h"
#include "net/tcp/tcp_buffer.h"
#include "common/log.h"
#include "common/config.h"
#include "net/tcp/tcp_acceptor.h"
#include <unistd.h>

int main() {

    rapidrpc::Config::SetGlobalConfig("/home/xiao/rapidrpc/rapidrpc/conf/rapidrpc.xml");

    // test NetAddr
    // ipv4
    rapidrpc::IpNetAddr ipNetAddr1("127.0.0.1:8888");
    rapidrpc::IpNetAddr ipNetAddr2("127.0.0.1", 1111);
    rapidrpc::IpNetAddr ipNetAddr3(*reinterpret_cast<sockaddr_in *>(ipNetAddr1.getSockAddr()));

    INFOLOG("ipNetAddr1: %s, isValid: %s", ipNetAddr1.toString().c_str(), ipNetAddr1 ? "true" : "false");
    INFOLOG("ipNetAddr2: %s, isValid: %s", ipNetAddr2.toString().c_str(), ipNetAddr2 ? "true" : "false");
    INFOLOG("ipNetAddr3: %s, isValid: %s", ipNetAddr3.toString().c_str(), ipNetAddr3 ? "true" : "false");

    // unix domain
    rapidrpc::UnixNetAddr unixNetAddr1("/tmp/rapidrpc.sock");
    rapidrpc::UnixNetAddr unixNetAddr2(*reinterpret_cast<sockaddr_un *>(unixNetAddr1.getSockAddr()));
    rapidrpc::UnixNetAddr unixNetAddr3("/tmp/hello.sock"); // test exist path

    INFOLOG("unixNetAddr1: %s, isValid: %s", unixNetAddr1.toString().c_str(), unixNetAddr1 ? "true" : "false");
    INFOLOG("unixNetAddr2: %s, isValid: %s", unixNetAddr2.toString().c_str(), unixNetAddr2 ? "true" : "false");
    INFOLOG("unixNetAddr3: %s, isValid: %s", unixNetAddr3.toString().c_str(), unixNetAddr3 ? "true" : "false");

    // ipv6
    rapidrpc::Ip6NetAddr ip6NetAddr1("::1:8888");
    rapidrpc::Ip6NetAddr ip6NetAddr2("::1", 1111);
    rapidrpc::Ip6NetAddr ip6NetAddr3(*reinterpret_cast<sockaddr_in6 *>(ip6NetAddr1.getSockAddr()));
    rapidrpc::Ip6NetAddr ip6NetAddr4("::ffff:192.0.2.128:8888"); // 零压缩+末尾4字节ipv4地址混合

    INFOLOG("ip6NetAddr1: %s, isValid: %s", ip6NetAddr1.toString().c_str(), ip6NetAddr1 ? "true" : "false");
    INFOLOG("ip6NetAddr2: %s, isValid: %s", ip6NetAddr2.toString().c_str(), ip6NetAddr2 ? "true" : "false");
    INFOLOG("ip6NetAddr3: %s, isValid: %s", ip6NetAddr3.toString().c_str(), ip6NetAddr3 ? "true" : "false");
    INFOLOG("ip6NetAddr4: %s, isValid: %s", ip6NetAddr4.toString().c_str(), ip6NetAddr4 ? "true" : "false");

    // ===========================================================
    // test TcpAcceptor

    // pointer address
    rapidrpc::NetAddr::s_ptr paddr = std::make_shared<rapidrpc::IpNetAddr>("0.0.0.0:12345");
    if (!*paddr) {
        ERRORLOG("Invalid address");
        return -1;
    }
    rapidrpc::TcpAcceptor tcpAcceptor(paddr, false); // blocking

    // client addr
    rapidrpc::IpNetAddr clientAddr;
    int clientfd = tcpAcceptor.accept(clientAddr);

    // echo server
    char buf[1024]{0};
    while (true) {
        int n = read(clientfd, buf, sizeof(buf));
        if (n < 0) {
            ERRORLOG("Failed to read");
            break;
        }
        if (n == 0) {
            INFOLOG("Client closed");
            break;
        }
        write(clientfd, buf, n);
    }

    return 0;
}