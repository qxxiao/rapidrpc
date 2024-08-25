#include "net/rpc/rpc_controller.h"

namespace rapidrpc {

void RpcController::Reset() {
    m_error_code = 0;
    m_error_info.clear();
    m_msg_id.clear();

    m_is_failed = false;
    m_is_canceled = false;

    m_local_addr.reset();
    m_peer_addr.reset();

    m_timeout = 1000;
}

bool RpcController::Failed() const {
    return m_is_failed;
}

std::string RpcController::ErrorText() const {
    return m_error_info;
}

void RpcController::StartCancel() {
    m_is_canceled = true;
}

void RpcController::SetFailed(const std::string &reason) {
    m_is_failed = true;
    m_error_info = reason;
}

bool RpcController::IsCanceled() const {
    return m_is_canceled;
}

void RpcController::NotifyOnCancel(google::protobuf::Closure *callback) {}

// self defined
void RpcController::SetError(Error err, const std::string error_info) {
    m_error_code = static_cast<int32_t>(err);
    m_error_info = error_info;
    m_is_failed = true;
}
void RpcController::SetError(int32_t err, const std::string error_info) {
    m_error_code = err;
    m_error_info = error_info;
    m_is_failed = true;
}
int32_t RpcController::GetErrorCode() const {
    return m_error_code;
}
std::string RpcController::GetErrorInfo() const {
    return m_error_info;
}

void RpcController::SetMsgId(const std::string &msg_id) {
    m_msg_id = msg_id;
}
std::string RpcController::GetMsgId() const {
    return m_msg_id;
}

void RpcController::SetLocalAddr(NetAddr::s_ptr addr) {
    m_local_addr = addr;
}
NetAddr::s_ptr RpcController::GetLocalAddr() const {
    return m_local_addr;
}

void RpcController::SetPeerAddr(NetAddr::s_ptr addr) {
    m_peer_addr = addr;
}
NetAddr::s_ptr RpcController::GetPeerAddr() const {
    return m_peer_addr;
}

void RpcController::SetTimeout(int timeout) {
    m_timeout = timeout;
}
int RpcController::GetTimeout() const {
    return m_timeout;
}
} // namespace rapidrpc