#include "rapidrpc/net/tcp/net_addr.h"
#include "rapidrpc/net/tcp/tcp_buffer.h"
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/config.h"
#include "rapidrpc/net/tcp/tcp_acceptor.h"
#include "rapidrpc/net/tcp/tcp_server.h"
#include <unistd.h>

int main() {

    rapidrpc::Config::SetGlobalConfig(nullptr);
    rapidrpc::Logger::InitGlobalLogger();

    // ipv4
    // rapidrpc::IpNetAddr ipNetAddr("0.0.0.0:12345");
    std::shared_ptr<rapidrpc::NetAddr> ipNetAddr = std::make_shared<rapidrpc::IpNetAddr>("0.0.0.0:12345");

    // ===========================================================
    // test TcpServer

    rapidrpc::TcpServer tcpServer(ipNetAddr); // init TcpServer and listen, add acceptor to mainReactor
    tcpServer.start();                        // start SubReactor & MainReactor EventLoop

    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
    return 0;
}