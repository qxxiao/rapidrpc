#include "rapidrpc/net/rpc/rpc_channel.h"
#include "rapidrpc/net/coder/tinypb_protocol.h"
#include "rapidrpc/net/rpc/rpc_controller.h"
#include "rapidrpc/net/tcp/tcp_client.h"
#include "rapidrpc/common/msg_id_util.h"
#include "rapidrpc/common/log.h"
#include "rapidrpc/common/error_code.h"

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

namespace rapidrpc {

RpcChannel::RpcChannel(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    // create client
    m_client = std::make_shared<TcpClient>(m_peer_addr);
}

RpcChannel::~RpcChannel() {
    DEBUGLOG("RpcChannel::~RpcChannel");
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller, const google::protobuf::Message *request,
                            google::protobuf::Message *response, google::protobuf::Closure *done) {

    // 1. 创建一个 TinyPBProtocol 对象，设置 method name
    TinyPBProtocol::s_ptr req = std::make_shared<TinyPBProtocol>();

    RpcController *rpc_controller = dynamic_cast<RpcController *>(controller);
    if (!rpc_controller) {
        ERRORLOG("RpcChannel::CallMethod: msg_id=[%s], RpcController is null", req->m_msg_id.c_str());
        return;
    }
    // check msg_id
    if (!rpc_controller->GetMsgId().empty()) {
        req->setMsgId(rpc_controller->GetMsgId());
    }
    else {
        req->setMsgId(MsgIdUtil::GenMsgId());
        rpc_controller->SetMsgId(req->m_msg_id);
    }
    // set method name
    req->setMethodName(method->full_name());

    // check if init
    if (!m_is_init) {
        ERRORLOG("RpcChannel::CallMethod: msg_id=[%s], RpcChannel not init", req->m_msg_id.c_str());
        rpc_controller->SetError(Error::SYS_CHANNEL_NOT_INIT, "RpcChannel not init");
        if (done)
            done->Run();
        return;
    }
    // set pb_data
    if (request && !request->SerializeToString(&req->m_pb_data)) {
        ERRORLOG("RpcChannel::CallMethod: msg_id=[%s], Serialize request failed, request=[%s]", req->m_msg_id.c_str(),
                 request->ShortDebugString().c_str());
        rpc_controller->SetError(Error::SYS_FAILED_SERIALIZE, "Serialize request failed");
        if (done)
            done->Run();
        return;
    }
    req->complete(); // compute pk_len
    // 准备好请求数据，开始调用远程服务
    DEBUGLOG("rpc call start, msg_id=[%s]: method_name=[%s], request=[%s]", req->m_msg_id.c_str(),
             req->m_method_name.c_str(), request->ShortDebugString().c_str());

    // 2. client connect to server
    s_ptr channel = shared_from_this(); // shared_ptr, error if raw pointer not managed by shared_ptr

    // set timeout task
    // !! req, channel will not be released until timeout task is executed
    // 如果没有超时，在取消定时任务时务必释放 req, channel，将定时任务回调函数清空/或者直接调用 deleteTimerEvent
    m_timer_event = std::make_shared<TimerEvent>(rpc_controller->GetTimeout(), false, [req, channel]() {
        // timeout task
        auto controller = std::dynamic_pointer_cast<RpcController>(channel->m_controller);
        controller->StartCancel(); // canceled
        controller->SetError(Error::SYS_RPC_CALL_TIMEOUT,
                             "rpc call timeout, msg_id=[" + req->m_msg_id + "], " + "method_name=[" + req->m_method_name
                                 + "] " + "peer_addr=[" + channel->m_peer_addr->toString() + "], " + "timeout=["
                                 + std::to_string(controller->GetTimeout()) + "ms]");
        if (channel->m_done)
            channel->m_done->Run();
        channel->m_client->close();
    });
    m_client->addTimerEvent(m_timer_event); // execute timeout task after timeout and not be canceled

    m_client->connect([req, channel]() {
        // !! check if connect success
        if (channel->m_client->getConnectErrorCode() != static_cast<int>(Error::OK)) {
            auto controller = std::dynamic_pointer_cast<RpcController>(channel->m_controller);
            controller->SetError(channel->m_client->getConnectErrorCode(), channel->m_client->getConnectErrorInfo());

            ERRORLOG("RpcChannel connect failed, error_code=%d, error_info=[%s], peer_addr=[%s]",
                     channel->m_client->getConnectErrorCode(), channel->m_client->getConnectErrorInfo().c_str(),
                     channel->m_peer_addr->toString().c_str());
            if (channel->m_done)
                channel->m_done->Run();
            channel->m_client->close();
            return;
        }
        // send
        channel->m_client->writeMessage(req, [req, channel](AbstractProtocol::s_ptr msg) {
            // after send to buffer
            // regiter callback for read response
            channel->m_client->readMessage(req->m_msg_id, [channel](AbstractProtocol::s_ptr msg) {
                // check if already canceled
                if (channel->m_controller->IsCanceled()) {
                    return;
                }
                // 成功读取到 msg 数据包后，取消超时任务
                // channel->m_client->deleteTimerEvent(channel->m_timer_event); // better
                channel->m_timer_event->setCanceled(
                    true); // cancel timeout task, and delete from timer queue until next timeout/onTimer

                // after read response tiny pb protocol
                TinyPBProtocol::s_ptr resp = std::dynamic_pointer_cast<TinyPBProtocol>(msg);
                auto controller = std::dynamic_pointer_cast<RpcController>(channel->m_controller);

                if (resp->m_err_code != static_cast<int32_t>(Error::OK)) {
                    controller->SetError(resp->m_err_code, resp->m_err_info);
                }

                // parse response
                else if (!channel->m_response->ParseFromString(resp->m_pb_data)) {
                    ERRORLOG("RpcChannel::CallMethod: msg_id=[%s], Deserialize response failed, response=[%s]",
                             resp->m_msg_id.c_str(), resp->m_pb_data.c_str());
                    controller->SetError(Error::SYS_FAILED_DESERIALIZE, "Deserialize response failed");
                }

                DEBUGLOG(
                    "rpc call success, msg_id=[%s]: error_code=%d, error_info=[%s], method_name=[%s], response=[%s]",
                    resp->m_msg_id.c_str(), resp->m_err_code, resp->m_err_info.c_str(), resp->m_method_name.c_str(),
                    channel->m_response->ShortDebugString().c_str());

                if (channel->m_done)
                    channel->m_done->Run();
                channel->m_client->close();
            });
        });
    });
}

void RpcChannel::Init(controller_s_ptr controller, message_s_ptr request, message_s_ptr response, closure_s_ptr done) {
    if (!m_is_init) {
        m_controller = controller;
        m_request = request;
        m_response = response;
        m_done = done;
        m_is_init = true;
    }
}

} // namespace rapidrpc