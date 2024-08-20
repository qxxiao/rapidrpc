/**
 * 作为客户端，测试 test_tcpserver.cc
 */

#include "net/tcp/net_addr.h"
#include "common/log.h"
#include "common/config.h"
#include "net/tcp/tcp_client.h"
#include "net/abstract_protocol.h"
#include "net/string_coder.h" // string protocol
#include <unistd.h>

int main() {
    rapidrpc::Config::SetGlobalConfig("/home/xiao/rapidrpc/rapidrpc/conf/rapidrpc.xml");

    // rapidrpc::IpNetAddr serverAddr("198.19.249.138:12345");
    rapidrpc::NetAddr::s_ptr serverAddr = std::make_shared<rapidrpc::IpNetAddr>("198.19.249.138:12345");

    rapidrpc::TcpClient client(serverAddr);

    // 异步的连接远程地址 connect to peer_addr, 完成后发送一条消息
    client.connect([&client]() {
        DEBUGLOG("Connect to server success");

        rapidrpc::AbstractProtocol::s_ptr message = std::make_shared<rapidrpc::StringProtocol>("Hello, server");
        message->setReqId("12345");
        client.writeMessage(message, [](rapidrpc::AbstractProtocol::s_ptr message) {
            DEBUGLOG("Write message success:%s",
                     std::dynamic_pointer_cast<rapidrpc::StringProtocol>(message)->getStr().c_str());
        });

        client.readMessage("12345", [](rapidrpc::AbstractProtocol::s_ptr message) {
            DEBUGLOG("Read message success:%s",
                     std::dynamic_pointer_cast<rapidrpc::StringProtocol>(message)->getStr().c_str());
        });
    });

    sleep(10);
    return 0;
}