#ifndef RAPIDRPC_NET_CODER_TINYPB_PROTOCOL_H
#define RAPIDRPC_NET_CODER_TINYPB_PROTOCOL_H

#include "net/coder/abstract_protocol.h"
#include "common/error_code.h"

#include <string>

namespace rapidrpc {

class TinyPBProtocol: public AbstractProtocol {
public:
    using s_ptr = std::shared_ptr<TinyPBProtocol>;

public:
    static constexpr char PB_START = 0x02;
    static constexpr char PB_END = 0x03;

    virtual ~TinyPBProtocol() = default;

    // set msg_id
    TinyPBProtocol &setMsgId(const std::string &msg_id) {
        m_msg_id = msg_id;
        m_msg_id_len = m_msg_id.size();
        return *this;
    }
    // set method_name
    TinyPBProtocol &setMethodName(const std::string &method_name) {
        m_method_name = method_name;
        m_method_name_len = m_method_name.size();
        return *this;
    }
    // set err_code and err_info
    TinyPBProtocol &setErrCodeAndInfo(Error err, const std::string &err_info) {
        m_err_code = static_cast<int32_t>(err);
        m_err_info = err_info;
        m_err_info_len = m_err_info.size();
        return *this;
    }
    // set pb_data
    TinyPBProtocol &setPbData(const std::string &pb_data) {
        m_pb_data = pb_data;
        return *this;
    }
    // compute pk_len
    TinyPBProtocol &complete() {
        m_pk_len = 2 * sizeof(char) + 6 * sizeof(int32_t) + m_msg_id.size() + m_method_name.size() + m_err_info.size()
                   + m_pb_data.size();
        return *this;
    }

    std::string toString() const override {
        return "m_msg_id: \"" + m_msg_id + "\", m_method_name: \"" + m_method_name
               + "\", m_err_code: " + std::to_string(m_err_code) + ", m_err_info: \"" + m_err_info + "\"";
    }

public:
    int32_t m_pk_len{0};
    int32_t m_msg_id_len{0};
    // msg_id 继承自父类

    int32_t m_method_name_len{0}; // 方法名长度
    std::string m_method_name;    // 方法名
    int32_t m_err_code{0};        // 错误码
    int32_t m_err_info_len{0};    // 错误信息长度
    std::string m_err_info;

    std::string m_pb_data;
    int32_t m_checksum{0}; // 校验和，no-use
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_CODER_TINYPB_PROTOCOL_H