/**
 * RPC closure: a callback to be executed when the RPC is done.
 */

#ifndef RAPIDRPC_NET_RPC_RPC_CLOSURE_H
#define RAPIDRPC_NET_RPC_RPC_CLOSURE_H

#include <google/protobuf/stubs/callback.h>
#include <functional>

namespace rapidrpc {

class RpcClosure: public google::protobuf::Closure {
public:
    RpcClosure(std::function<void()> callback) : m_callback(callback) {}

    void Run() override {
        if (m_callback) {
            m_callback();
        }
    }

private:
    std::function<void()> m_callback{nullptr};
};
} // namespace rapidrpc

#endif