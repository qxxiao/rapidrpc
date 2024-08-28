#ifndef RAPID_NET_ABSTRACT_CODER_H
#define RAPID_NET_ABSTRACT_CODER_H

#include "rapidrpc/net/tcp/tcp_buffer.h"
#include "rapidrpc/net/coder/abstract_protocol.h"

#include <vector>
#include <memory>

namespace rapidrpc {

class AbstractCoder {
public:
    using s_ptr = std::shared_ptr<AbstractCoder>;

public:
    // encode the message to the out_buffer
    virtual void encode(std::vector<AbstractProtocol::s_ptr> &messages,
                        TcpBuffer::s_ptr out_buffer) = 0; // encode the message
    // decode the in_buffer to the out_messages
    virtual void decode(std::vector<AbstractProtocol::s_ptr> &out_messages,
                        TcpBuffer::s_ptr in_buffer) = 0; // decode the message

    virtual ~AbstractCoder() = default;
};

} // namespace rapidrpc

#endif // !RAPID_NET_ABSTRACT_CODER_H
