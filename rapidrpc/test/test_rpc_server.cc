/**
 * 添加自定义的 proto 文件
 */
#include "common/log.h"
#include "common/config.h"
#include "net/tcp/net_addr.h"
#include "net/tcp/tcp_server.h"
#include "net/rpc/dispatcher.h"
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
        APPDEBUGLOG("server start sleeping 5s");
        sleep(5);
        APPDEBUGLOG("server end sleeping 5s");
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
    rapidrpc::Config::SetGlobalConfig("/home/xiao/rapidrpc/rapidrpc/conf/rapidrpc.xml");
    rapidrpc::Logger::InitGlobalLogger();

    // register service
    // service name: "Order", is the same as the service name in proto file | Order base on Order.proto
    rapidrpc::Dispatcher::GetDispatcher()->registerService(std::make_shared<OrderImpl>());

    // ipv4
    // rapidrpc::IpNetAddr ipNetAddr("0.0.0.0:12345");
    std::shared_ptr<rapidrpc::NetAddr> ipNetAddr = std::make_shared<rapidrpc::IpNetAddr>("0.0.0.0:12345");

    rapidrpc::TcpServer tcpServer(ipNetAddr); // init TcpServer and listen, add acceptor to mainReactor
    tcpServer.start();                        // start SubReactor & MainReactor EventLoop

    rapidrpc::Logger::GetGlobalLogger()->flushAndStop();
}