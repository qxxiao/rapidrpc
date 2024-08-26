#ifndef RAPIDRPC_NET_RPC_RPC_CONTROLLER_H
#define RAPIDRPC_NET_RPC_RPC_CONTROLLER_H

#include "net/tcp/net_addr.h"
#include "common/error_code.h"

#include <google/protobuf/service.h>
#include <string>

namespace rapidrpc {

class RpcController: public google::protobuf::RpcController {

public:
    RpcController() = default;
    ~RpcController() = default;

    void Reset() override;

    bool Failed() const override;

    std::string ErrorText() const override;

    void StartCancel() override;

    void SetFailed(const std::string &reason) override;

    bool IsCanceled() const override;

    void NotifyOnCancel(google::protobuf::Closure *callback) override;

    // self defined

    // 设置错误码和错误信息
    void SetError(Error err, const std::string error_info);
    // 设置错误码和错误信息
    void SetError(int32_t err, const std::string error_info);
    int32_t GetErrorCode() const;
    std::string GetErrorInfo() const;

    void SetMsgId(const std::string &msg_id);
    std::string GetMsgId() const;

    void SetLocalAddr(NetAddr::s_ptr addr);
    NetAddr::s_ptr GetLocalAddr() const;

    void SetPeerAddr(NetAddr::s_ptr addr);
    NetAddr::s_ptr GetPeerAddr() const;

    // 设置超时时间, 单位 ms; 默认 1000ms
    void SetTimeout(int timeout);
    int GetTimeout() const;

private:
    int m_error_code{0};
    std::string m_error_info;
    std::string m_msg_id;

    bool m_is_failed{false};
    bool m_is_canceled{false};

    NetAddr::s_ptr m_local_addr;
    NetAddr::s_ptr m_peer_addr;

    int m_timeout{1000}; // ms
};

} // namespace rapidrpc

#endif // RAPIDRPC_NET_RPC_RPC_CONTROLLER_H