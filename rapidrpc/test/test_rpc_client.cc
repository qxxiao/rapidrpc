/**
 * 作为客户端，测试 test_rpc_server.cc
 */

#include "net/tcp/net_addr.h"
#include "common/log.h"
#include "common/config.h"
#include "net/tcp/tcp_client.h"
#include "net/coder/abstract_protocol.h"
#include "net/coder/tinypb_protocol.h"
#include "order.pb.h"
#include <unistd.h>

int main() {
    rapidrpc::Config::SetGlobalConfig("/home/xiao/rapidrpc/rapidrpc/conf/rapidrpc.xml");

    // rapidrpc::IpNetAddr serverAddr("198.19.249.138:12345");
    rapidrpc::NetAddr::s_ptr serverAddr = std::make_shared<rapidrpc::IpNetAddr>("198.19.249.138:12345");

    rapidrpc::TcpClient client(serverAddr);

    // 异步的连接远程地址 connect to peer_addr, 完成后发送一条消息
    makeOrderRequest request;
    request.set_price(100);
    request.set_goods("apple");

    client.connect([&client, request]() {
        DEBUGLOG("Connect to server success");

        auto msg = std::make_shared<rapidrpc::TinyPBProtocol>();
        msg->setReqId("12345").setMethodName("Order.makeOrder").setPbData(request.SerializeAsString()).complete();

        client.writeMessage(msg, [request](rapidrpc::AbstractProtocol::s_ptr message) {
            auto msg = std::dynamic_pointer_cast<rapidrpc::TinyPBProtocol>(message);
            DEBUGLOG("Write message success: [%s]", msg->toString().c_str());
            DEBUGLOG("Write request success: [%s]", request.ShortDebugString().c_str());
        });

        client.readMessage("12345", [](rapidrpc::AbstractProtocol::s_ptr message) {
            makeOrderResponse response;
            auto msg = std::dynamic_pointer_cast<rapidrpc::TinyPBProtocol>(message);
            response.ParseFromString(msg->m_pb_data);

            DEBUGLOG("Read message success: [%s]", msg->toString().c_str());
            DEBUGLOG("Read response success: [%s]", response.ShortDebugString().c_str());
        });
    });

    sleep(10);
    return 0;
}