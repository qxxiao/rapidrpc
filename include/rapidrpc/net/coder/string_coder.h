/**
 * A simple test protocol and coder for string message
 */

#ifndef RAPIDRPC_NET_STRING_CODER_H
#define RAPIDRPC_NET_STRING_CODER_H

#include "rapidrpc/net/coder/abstract_coder.h"
#include "rapidrpc/net/coder/abstract_protocol.h"

#include <vector>

namespace rapidrpc {

class StringProtocol: public AbstractProtocol {
public:
    using s_ptr = std::shared_ptr<StringProtocol>; // override the s_ptr

public:
    StringProtocol(const std::string &str) : str(str) {}
    StringProtocol(const char *str) : str(str) {}
    virtual ~StringProtocol() = default;

    std::string getStr() const {
        return str;
    }

private:
    std::string str; // message
};

class StringCoder: public AbstractCoder {
public:
    using s_ptr = std::shared_ptr<StringCoder>;

public:
    virtual void encode(std::vector<AbstractProtocol::s_ptr> &messages, TcpBuffer::s_ptr out_buffer) override {
        for (auto &message : messages) {
            StringProtocol::s_ptr str_message = std::dynamic_pointer_cast<StringProtocol>(message);
            out_buffer->writeToBuffer(str_message->getStr().c_str(), str_message->getStr().size());
        }
    }

    virtual void decode(std::vector<AbstractProtocol::s_ptr> &out_messages, TcpBuffer::s_ptr in_buffer) override {
        std::vector<char> str(in_buffer->readAvailable());
        in_buffer->readFromBuffer(str, str.capacity());
        std::string s(str.begin(), str.end());
        auto msg = std::make_shared<StringProtocol>(s);
        msg->m_msg_id = "12345";
        out_messages.push_back(msg);
    }
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_STRING_CODER_H
