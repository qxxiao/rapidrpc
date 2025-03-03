#ifndef RAPIDRPC_NET_TCP_ABSTRACT_PROTOCOL_H
#define RAPIDRPC_NET_TCP_ABSTRACT_PROTOCOL_H

#include "rapidrpc/net/tcp/tcp_buffer.h"

#include <memory>
#include <string>

namespace rapidrpc {

// AbstractProtocol is the base class for all protocols
// 数据包协议的基类
class AbstractProtocol {
public:
    using s_ptr = std::shared_ptr<AbstractProtocol>;

public:
    AbstractProtocol() = default;
    virtual ~AbstractProtocol() = default;
    virtual std::string toString() const {
        // json format with fields
        return "m_msg_id: \"" + m_msg_id + "\"";
    }

public:
    std::string m_msg_id; // request id/response id
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_TCP_ABSTRACT_PROTOCOL_H
