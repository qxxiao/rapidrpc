#include "rapidrpc/net/rpc/dispatcher.h"
#include "rapidrpc/net/coder/tinypb_protocol.h"
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/error_code.h"
#include "rapidrpc/net/rpc/rpc_controller.h"
#include "rapidrpc/net/tcp/net_addr.h"
#include "rapidrpc/common/runtime.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

namespace rapidrpc {

static Dispatcher *g_dispatcher = nullptr;
Dispatcher *Dispatcher::GetDispatcher() {
    if (g_dispatcher) {
        return g_dispatcher;
    }
    static Dispatcher dispatcher;
    g_dispatcher = &dispatcher;
    return g_dispatcher;
}

void Dispatcher::registerService(service_s_ptr service) {
    std::string service_name = service->GetDescriptor()->full_name();
    m_services[service_name] = service;
    DEBUGLOG("Dispatcher::registerService: service [%s] registered", service_name.c_str());
}

// 服务器启动之前首先注册 Service 对象。
// 1. 从客户端得到的 TinyPBProtocol 对象中找到 service name 和 method name, 通过 service name 找到对应的 Service 对象,
// 通过 method name 找到对应的 Method 函数。
// 2. 找到对应的 request type 和 response type
// 3. 将请求中的 pb_data 反序列化为 request type 对象, 创建一个空的 response type 对象
// 4. 调用 func(request, response)
// 5. 将 response type 对象序列化为 pb_data，写入 response 对象
void Dispatcher::dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response) {
    TinyPBProtocol::s_ptr req = std::dynamic_pointer_cast<TinyPBProtocol>(request);
    TinyPBProtocol::s_ptr resp = std::dynamic_pointer_cast<TinyPBProtocol>(response);
    resp->setMsgId(req->m_msg_id).setMethodName(req->m_method_name); // set msg_id and method_name for response

    // get service name and method name
    std::string service_name, method_name;
    if (!parseServiceAndMethod(req->m_method_name, service_name, method_name)) {
        ERRORLOG("Dispatcher::dispatch: msg_id=[%s], invalid full_name: [%s]", req->m_msg_id.c_str(),
                 req->m_method_name.c_str());
        resp->setErrCodeAndInfo(Error::SYS_FAILED_PARSE_SERVICE_NAME, "invalid method name").complete();
        return;
    }
    // get service object and method object
    auto service_iter = m_services.find(service_name);
    if (service_iter == m_services.end()) {
        ERRORLOG("Dispatcher::dispatch: msg_id=[%s], service [%s] not found", req->m_msg_id.c_str(),
                 service_name.c_str());
        resp->setErrCodeAndInfo(Error::SYS_SERVICE_NOT_FOUND, "service not found").complete();
        return;
    }

    service_s_ptr service = service_iter->second;
    // 先获取服务的 descriptor，然后通过 descriptor 获取方法的 descriptor
    const google::protobuf::ServiceDescriptor *service_desc = service->GetDescriptor();
    const google::protobuf::MethodDescriptor *method_desc = service_desc->FindMethodByName(method_name);
    if (method_desc == nullptr) {
        ERRORLOG("Dispatcher::dispatch: msg_id=[%s], method [%s] not found in service [%s]", req->m_msg_id.c_str(),
                 method_name.c_str(), service_name.c_str());
        resp->setErrCodeAndInfo(Error::SYS_METHOD_NOT_FOUND, "method not found").complete();
        return;
    }

    // get method request type and response type
    google::protobuf::Message *func_req = service->GetRequestPrototype(method_desc).New();
    google::protobuf::Message *func_resp = service->GetResponsePrototype(method_desc).New();
    // parse request
    if (!func_req->ParseFromString(req->m_pb_data)) {
        ERRORLOG("Dispatcher::dispatch: msg_id=[%s], deserialize request failed", req->m_msg_id.c_str());
        resp->setErrCodeAndInfo(Error::SYS_FAILED_DESERIALIZE, "deserialize request failed").complete();
        delete func_req;
        delete func_resp;
        return;
    }
    // TODO: check shortdebug info
    INFOLOG("Dispatcher::dispatch get rpc request: msg_id=[%s], service [%s], method [%s], rpc request[%s]",
            req->m_msg_id.c_str(), service_name.c_str(), method_name.c_str(), func_req->ShortDebugString().c_str());
    // ! call method
    // TODO: rpc controller
    RpcController rpcController;
    rpcController.SetLocalAddr(nullptr);
    rpcController.SetPeerAddr(nullptr);
    rpcController.SetMsgId(req->m_msg_id);

    // set app context for runtime
    Runtime::GetRuntime()->setMsgId(req->m_msg_id);
    Runtime::GetRuntime()->setMethodName(method_name);
    service->CallMethod(method_desc, &rpcController, func_req, func_resp, nullptr);

    // serialize response
    auto pb_data = func_resp->SerializeAsString();
    if (pb_data.empty()) {
        ERRORLOG("Dispatcher::dispatch: msg_id=[%s], serialize response failed", req->m_msg_id.c_str());
        resp->setErrCodeAndInfo(Error::SYS_FAILED_SERIALIZE, "serialize response failed").complete();
        delete func_req;
        delete func_resp;
        return;
    }
    resp->setErrCodeAndInfo(Error::OK, "success").setPbData(pb_data).complete();
    INFOLOG("Dispatcher::dispatch rpc call success: msg_id=[%s], service [%s], method [%s], rpc response[%s]",
            req->m_msg_id.c_str(), service_name.c_str(), method_name.c_str(), func_resp->ShortDebugString().c_str());
    delete func_req;
    delete func_resp;
}

bool Dispatcher::parseServiceAndMethod(const std::string &full_name, std::string &service_name,
                                       std::string &method_name) {
    auto pos = full_name.find_first_of('.');
    if (pos == std::string::npos) {
        return false;
    }
    service_name = full_name.substr(0, pos);
    method_name = full_name.substr(pos + 1);

    if (service_name.empty() || method_name.empty()) {
        return false;
    }
    return true;
}

} // namespace rapidrpc