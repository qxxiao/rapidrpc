#ifndef RAPIDRPC_NET_RPC_CHANNEL_H
#define RAPIDRPC_NET_RPC_CHANNEL_H

#include "net/tcp/net_addr.h"
#include "net/tcp/tcp_client.h"
#include "net/rpc/rpc_controller.h"
#include "net/timer_event.h"

#include <google/protobuf/service.h>
#include <memory>

/**
 * @brief create new Rpc request/response message
 * @param name shared_ptr variable name
 * @param msgType requestType or responseType. eg. makeOrderRequest, makeOrderResponse
 */
#define NEW_RPC_MESSAGE(name, msgType) std::shared_ptr<msgType> name = std::make_shared<msgType>();

/**
 * @brief create new Rpc controller
 * @param controllerName shared_ptr variable name
 * @note can set timeout(ms) by controllerName->SetTimeout(timeout) or get status by controllerName->Failed()
 */
#define NEW_RPC_CONTROLLER(controllerName)                                                                             \
    std::shared_ptr<rapidrpc::RpcController> controllerName = std::make_shared<rapidrpc::RpcController>();

#define NEW_RPC_CHANNEL(channelName, peerAddrString)                                                                   \
    std::shared_ptr<rapidrpc::RpcChannel> channelName =                                                                \
        std::make_shared<rapidrpc::RpcChannel>(std::make_shared<rapidrpc::IpNetAddr>(peerAddrString));

/**
 * @brief Call remote service micro
 * @param addr remote service address, eg. "127.0.0.1:12345" or use sockaddr_in directly
 * @param StubType stubType, eg. Order_Stub
 * @param methodName service method name, eg. makeOrder
 * @param controller rapidrpc::RpcController, shared_ptr variable name, use NEW_RPC_CONTROLLER(controller) to create
 * @param request requestType, shared_ptr variable name, use NEW_RPC_REQUEST(request, requestType) to create
 * @param response responseType, shared_ptr variable name, use NEW_RPC_RESPONSE(response, responseType) to create
 * @param done rapidrpc::RpcClosure, shared_ptr variable name, use std::make_shared<rapidrpc::RpcClosure>([controller,
 * response](){...}) to create
 */
#define CALL_RPC(addr, StubType, methodName, controller, request, response, done)                                      \
    {                                                                                                                  \
        NEW_RPC_CHANNEL(channel, addr);                                                                                \
        channel->Init(controller, request, response, done);                                                            \
        StubType stub(channel.get());                                                                                  \
        stub.methodName(controller.get(), request.get(), response.get(), done.get());                                  \
    }

namespace rapidrpc {

/**
 * RpcChannel is a channel to call remote service
 * @note 由于在异步的回调函数中会使用 RpcController, Message(response), Closure(done),
 * 所以这些对象需要保证在回调函数执行完毕之前不被销毁，因此推荐使用 shared_ptr，并在内部保存这些对象的 shared_ptr;
 * 同时保证 RpcChannel 使用 shared_ptr 管理
 */
class RpcChannel: public google::protobuf::RpcChannel, public std::enable_shared_from_this<RpcChannel> {
public:
    using s_ptr = std::shared_ptr<RpcChannel>;

    using controller_s_ptr = std::shared_ptr<google::protobuf::RpcController>;
    using message_s_ptr = std::shared_ptr<google::protobuf::Message>;
    using closure_s_ptr = std::shared_ptr<google::protobuf::Closure>;

public:
    RpcChannel(NetAddr::s_ptr peer_addr);
    ~RpcChannel();

    void Init(controller_s_ptr controller, message_s_ptr request, message_s_ptr response, closure_s_ptr done);

    // Call the given method of the remote service.  The signature of this
    // procedure looks the same as Service::CallMethod(), but the requirements
    // are less strict in one important way:  the request and response objects
    // need not be of any specific class as long as their descriptors are
    // method->input_type() and method->output_type().
    void CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request, google::protobuf::Message *response,
                    google::protobuf::Closure *done) override;

    // Getter raw pointer
    google::protobuf::RpcController *getController() const {
        return m_controller.get();
    }
    // Getter raw pointer
    google::protobuf::Message *getRequest() const {
        return m_request.get();
    }
    // Getter raw pointer
    google::protobuf::Message *getResponse() const {
        return m_response.get();
    }
    // Getter raw pointer
    google::protobuf::Closure *getClosure() const {
        return m_done.get();
    }
    // Getter raw pointer
    TcpClient *getClient() const {
        return m_client.get();
    }

    TimerEvent::s_ptr getTimerEvent() const {
        return m_timer_event;
    }

private:
    NetAddr::s_ptr m_peer_addr;
    NetAddr::s_ptr m_local_addr;
    TcpClient::s_ptr m_client;

    // saved controller, request, response, done
    controller_s_ptr m_controller;
    message_s_ptr m_request;
    message_s_ptr m_response;
    closure_s_ptr m_done;
    bool m_is_init{false}; // check saved controller, request, response, done

    // timer event for timeout
    TimerEvent::s_ptr m_timer_event;
};
} // namespace rapidrpc

#endif // RAPIDRPC_NET_RPC_CHANNEL_H