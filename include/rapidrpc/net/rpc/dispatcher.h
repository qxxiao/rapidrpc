#ifndef RAPIDRPC_NET_RPC_DISPATCHER
#define RAPIDRPC_NET_RPC_DISPATCHER

#include "rapidrpc/net/coder/abstract_protocol.h"

#include <memory>
#include <string>
#include <map>
#include <google/protobuf/service.h>

namespace rapidrpc {

class Dispatcher {
public:
    using s_ptr = std::shared_ptr<Dispatcher>;
    using service_s_ptr = std::shared_ptr<google::protobuf::Service>;

public:
    static Dispatcher *GetDispatcher();
    // 注册 Service 对象
    void registerService(service_s_ptr service);

    void dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response);

private:
    // parse service name and method name from request
    bool parseServiceAndMethod(const std::string &full_name, std::string &service_name, std::string &method_name);

private:
    std::map<std::string, service_s_ptr> m_services;
};
} // namespace rapidrpc

#endif // RAPIDRPC_NET_RPC_DISPATCHER