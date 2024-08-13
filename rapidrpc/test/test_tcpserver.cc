#include "net/tcp/net_addr.h"
#include "net/tcp/tcp_buffer.h"
#include "common/log.h"
#include "common/config.h"
#include "net/tcp/tcp_acceptor.h"
#include "net/tcp/tcp_server.h"
#include <unistd.h>

int main() {

    rapidrpc::Config::SetGlobalConfig("/home/xiao/rapidrpc/rapidrpc/conf/rapidrpc.xml");

    // ipv4
    // rapidrpc::IpNetAddr ipNetAddr("0.0.0.0:12345");
    std::shared_ptr<rapidrpc::NetAddr> ipNetAddr = std::make_shared<rapidrpc::IpNetAddr>("0.0.0.0:12345");

    // ===========================================================
    // test TcpServer

    rapidrpc::TcpServer tcpServer(ipNetAddr); // init TcpServer and listen, add acceptor to mainReactor
    tcpServer.start();                        // start SubReactor & MainReactor EventLoop

    return 0;
}