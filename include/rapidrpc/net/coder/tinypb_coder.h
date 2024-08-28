#ifndef RAPIDRPC_NET_CODER_TINYPB_CODER_H
#define RAPIDRPC_NET_CODER_TINYPB_CODER_H

#include "rapidrpc/net/coder/abstract_coder.h"
#include "rapidrpc/net/coder/tinypb_protocol.h"

#include <vector>

namespace rapidrpc {

class TinyPBCoder: public AbstractCoder {
public:
    // implement
    void encode(std::vector<AbstractProtocol::s_ptr> &messages, TcpBuffer::s_ptr out_buffer) override;
    void decode(std::vector<AbstractProtocol::s_ptr> &out_messages, TcpBuffer::s_ptr in_buffer) override;
    ~TinyPBCoder() = default;

private:
    // 编码一个 TinyPBProtocol 消息，返回字节流数据
    std::vector<char> encodeMessage(TinyPBProtocol::s_ptr msg);

private:
    static int32_t checksum_xor(char *data, int len);
    static bool checksum_validate(char *data, int len);
    bool checkIndexValid(int index, int len, int end);
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_CODER_TINYPB_CODER_H