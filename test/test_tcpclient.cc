/**
 * 作为客户端，测试 test_tcpserver.cc
 */

#include "rapidrpc/net/tcp/net_addr.h"
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/config.h"
#include "rapidrpc/net/tcp/tcp_client.h"
#include "rapidrpc/net/coder/abstract_protocol.h"
#include "rapidrpc/net/coder/tinypb_protocol.h"
#include <unistd.h>

int main() {
    rapidrpc::Config::SetGlobalConfig(nullptr);
    rapidrpc::Logger::InitGlobalLogger();

    // rapidrpc::IpNetAddr serverAddr("198.19.249.138:12345");
    rapidrpc::NetAddr::s_ptr serverAddr = std::make_shared<rapidrpc::IpNetAddr>("198.19.249.138:12345");

    rapidrpc::TcpClient client(serverAddr);

    // 异步的连接远程地址 connect to peer_addr, 完成后发送一条消息
    client.connect([&client]() {
        DEBUGLOG("Connect to server success");

        auto msg = std::make_shared<rapidrpc::TinyPBProtocol>();
        msg->setMsgId("12345").setMethodName("test").setPbData("hello").complete();

        client.writeMessage(msg, [](rapidrpc::AbstractProtocol::s_ptr message) {
            DEBUGLOG("Write message success: [%s]",
                     std::dynamic_pointer_cast<rapidrpc::TinyPBProtocol>(message)->toString().c_str());
        });

        client.readMessage("12345", [](rapidrpc::AbstractProtocol::s_ptr message) {
            DEBUGLOG("Read message success:[%s]",
                     std::dynamic_pointer_cast<rapidrpc::TinyPBProtocol>(message)->toString().c_str());
        });
    });

    sleep(10);
    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
    return 0;
}