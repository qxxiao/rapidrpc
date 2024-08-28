/**
 * 作为客户端，测试 test_rpc_server.cc
 */

#include "rapidrpc/net/tcp/net_addr.h"
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/config.h"
#include "rapidrpc/net/tcp/tcp_client.h"
#include "rapidrpc/net/coder/abstract_protocol.h"
#include "rapidrpc/net/coder/tinypb_protocol.h"
#include "rapidrpc/net/rpc/rpc_channel.h"
#include "rapidrpc/net/rpc/rpc_controller.h"
#include "rapidrpc/net/rpc/rpc_closure.h"
#include "order.pb.h"

#include <unistd.h>
#include <memory>
#include <stdio.h>

void test1() {
    rapidrpc::Config::SetGlobalConfig(nullptr);
    rapidrpc::Logger::InitGlobalLogger();

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
        msg->setMsgId("12345").setMethodName("Order.makeOrder").setPbData(request.SerializeAsString()).complete();

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

    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
}

void test2() {
    rapidrpc::Config::SetGlobalConfig(nullptr);
    rapidrpc::Logger::InitGlobalLogger();

    // rapidrpc::IpNetAddr serverAddr("198.19.249.138:12345");
    rapidrpc::NetAddr::s_ptr serverAddr = std::make_shared<rapidrpc::IpNetAddr>("198.19.249.138:12345");

    // channel, shared_ptr, error if raw pointer not managed by shared_ptr
    rapidrpc::RpcChannel::s_ptr channel = std::make_shared<rapidrpc::RpcChannel>(serverAddr);

    // controller, request, response, done
    auto controller = std::make_shared<rapidrpc::RpcController>();
    auto request = std::make_shared<makeOrderRequest>();
    auto response = std::make_shared<makeOrderResponse>();
    rapidrpc::RpcChannel::closure_s_ptr done = std::make_shared<rapidrpc::RpcClosure>([controller, response]() {
        if (controller->Failed()) {
            ERRORLOG("RpcClosure rpc failed: %s", controller->ErrorText().c_str());
        }
        else {
            INFOLOG("RpcClosure rpc success: resp=[%s]", response->ShortDebugString().c_str());
        }
    });
    // init channel
    channel->Init(controller, request, response, done);

    // set request
    request->set_price(100);
    request->set_goods("apple");
    controller->SetTimeout(5000);

    Order_Stub stub(channel.get());
    stub.makeOrder(controller.get(), request.get(), response.get(), done.get());

    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
}

void test() {
    rapidrpc::Config::SetGlobalConfig(nullptr);
    rapidrpc::Logger::InitGlobalLogger();

    NEW_RPC_MESSAGE(request, makeOrderRequest);
    NEW_RPC_MESSAGE(response, makeOrderResponse);

    request->set_price(100);
    request->set_goods("apple");

    NEW_RPC_CONTROLLER(controller);
    controller->SetTimeout(10000);

    auto done = std::make_shared<rapidrpc::RpcClosure>([controller, response]() {
        if (controller->Failed()) {
            ERRORLOG("RpcClosure rpc failed: %s", controller->ErrorText().c_str());
        }
        else {
            INFOLOG("RpcClosure rpc success: resp=[%s]", response->ShortDebugString().c_str());
        }
    });

    CALL_RPC("198.19.249.138:12345", Order_Stub, makeOrder, controller, request, response, done);

    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
}

int main() {
    test();

    return 0;
}