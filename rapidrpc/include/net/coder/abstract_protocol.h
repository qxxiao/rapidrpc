#ifndef RAPIDRPC_NET_TCP_ABSTRACT_PROTOCOL_H
#define RAPIDRPC_NET_TCP_ABSTRACT_PROTOCOL_H

#include "net/tcp/tcp_buffer.h"

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
        return std::string("{\n") + "\tm_req_id: \"" + m_req_id + "\"\n}";
    }

public:
    std::string m_req_id; // request id/response id
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_TCP_ABSTRACT_PROTOCOL_H
