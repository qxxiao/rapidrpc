/**
 * 添加自定义的 proto 文件
 */
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/config.h"
#include "rapidrpc/net/tcp/net_addr.h"
#include "rapidrpc/net/tcp/tcp_server.h"
#include "rapidrpc/net/rpc/dispatcher.h"
#include "order.pb.h"

#include <google/protobuf/service.h>
#include <unistd.h>
#include <thread>
#include <chrono>

// 服务端实现
class OrderImpl: public Order {
public:
    void makeOrder(google::protobuf::RpcController *controller, const ::makeOrderRequest *request,
                   ::makeOrderResponse *response, google::protobuf::Closure *done) {
        APPDEBUGLOG("server start sleeping 2s");
        sleep(2);
        APPDEBUGLOG("server end sleeping 2s");
        if (request->price() < 10) {
            response->set_ret_code(-1);
            response->set_res_info("short of money");
            return;
        }
        response->set_ret_code(0);
        response->set_res_info("success");
        response->set_order_id("20240101");
        APPDEBUGLOG("rpc call success");
    }
};

int main() {
    rapidrpc::Config::SetGlobalConfig("./conf/rapidrpc.xml");
    rapidrpc::Logger::InitGlobalLogger();

    // register service
    // service name: "Order", is the same as the service name in proto file | Order base on Order.proto
    rapidrpc::Dispatcher::GetDispatcher()->registerService(std::make_shared<OrderImpl>());

    std::shared_ptr<rapidrpc::NetAddr> ipNetAddr = std::make_shared<rapidrpc::IpNetAddr>(
        rapidrpc::Config::GetGlobalConfig()->m_ip, rapidrpc::Config::GetGlobalConfig()->m_port);

    rapidrpc::TcpServer tcpServer(ipNetAddr); // init TcpServer and listen, add acceptor to mainReactor
    tcpServer.start();                        // start SubReactor & MainReactor EventLoop

    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
}