#include "net/coder/tinypb_coder.h"
#include "net/coder/tinypb_protocol.h"
#include "common/log.h"

#include <vector>
#include <arpa/inet.h>
#include <cstring>

namespace rapidrpc {

void TinyPBCoder::encode(std::vector<AbstractProtocol::s_ptr> &messages, TcpBuffer::s_ptr out_buffer) {
    for (auto &message : messages) {
        auto msg = std::dynamic_pointer_cast<TinyPBProtocol>(message);
        if (msg == nullptr) {
            continue;
        }
        auto data = encodeMessage(msg);
        out_buffer->writeToBuffer(data.data(), data.size());
    }
}

void TinyPBCoder::decode(std::vector<AbstractProtocol::s_ptr> &out_messages, TcpBuffer::s_ptr in_buffer) {
    // 解码为 TinyPBProtocol 消息格式，每次读取一个完整的消息
    while (true) {
        // move read index to the first PB_START
        while (in_buffer->readAvailable() > 0
               && in_buffer->m_buffer[in_buffer->readIndex()] != TinyPBProtocol::PB_START) {
            in_buffer->moveReadIndex(1);
        }
        // test pk_len
        if ((size_t)in_buffer->readAvailable() < (sizeof(char) + sizeof(int32_t))) {
            return;
        }
        int pk_len = ntohl(*(int *)(&in_buffer->m_buffer[in_buffer->readIndex() + 1]));
        if (in_buffer->readAvailable() < pk_len) {
            return;
        }
        // test pb_end
        char end = in_buffer->m_buffer[in_buffer->readIndex() + pk_len - 1];
        if (end != TinyPBProtocol::PB_END) {
            ERRORLOG("TinyPBCoder decode error, pb_end error");
            in_buffer->moveReadIndex(pk_len);
            continue;
        }

        // ! read pk_len bytes to tmp
        std::vector<char> tmp(pk_len);
        in_buffer->readFromBuffer(tmp, pk_len); // move read index
        int read_index = 0;
        auto message = std::make_shared<TinyPBProtocol>();
        message->m_pk_len = pk_len;
        read_index += sizeof(char) + sizeof(int32_t); // +5

        // ! read req_id_len and req_id
        if (!checkIndexValid(read_index, sizeof(int32_t), pk_len)) {
            continue;
        }
        message->m_req_id_len = ntohl(*(int *)(&tmp[read_index]));
        read_index += sizeof(int32_t);

        if (!checkIndexValid(read_index, message->m_req_id_len, pk_len)) {
            continue;
        }
        message->m_req_id = std::string(tmp.begin() + read_index, tmp.begin() + read_index + message->m_req_id_len);
        read_index += message->m_req_id_len;

        // ! read method_len and method_name
        if (!checkIndexValid(read_index, sizeof(int32_t), pk_len)) {
            continue;
        }
        message->m_method_name_len = ntohl(*(int *)(&tmp[read_index]));
        read_index += sizeof(int32_t);

        if (!checkIndexValid(read_index, message->m_method_name_len, pk_len)) {
            continue;
        }
        message->m_method_name =
            std::string(tmp.begin() + read_index, tmp.begin() + read_index + message->m_method_name_len);
        read_index += message->m_method_name_len;

        // ! read err_code and err_info_len and err_info
        if (!checkIndexValid(read_index, sizeof(int32_t), pk_len)) {
            continue;
        }
        message->m_err_code = ntohl(*(int *)(&tmp[read_index]));
        read_index += sizeof(int32_t);
        if (!checkIndexValid(read_index, sizeof(int32_t), pk_len)) {
            continue;
        }
        message->m_err_info_len = ntohl(*(int *)(&tmp[read_index]));
        read_index += sizeof(int32_t);
        if (!checkIndexValid(read_index, message->m_err_info_len, pk_len)) {
            continue;
        }
        message->m_err_info = std::string(tmp.begin() + read_index, tmp.begin() + read_index + message->m_err_info_len);
        read_index += message->m_err_info_len;

        // ! read pb_data
        if (tmp.end() < tmp.begin() + read_index + sizeof(char) + sizeof(int32_t)) {
            ERRORLOG("TinyPBCoder decode error, pb_data len error");
            continue;
        }
        message->m_pb_data = std::string(tmp.begin() + read_index, tmp.end() - sizeof(char) - sizeof(int32_t));
        // ! check checksum
        if (!checksum_validate(tmp.data(), pk_len)) {
            ERRORLOG("TinyPBCoder decode error, checksum error");
            continue;
        }
        DEBUGLOG("TinyPBCoder decode success, message: req_id=[%s], method_name=[%s]", message->m_req_id.c_str(),
                 message->m_method_name.c_str());
        out_messages.push_back(message);
    }
}

std::vector<char> TinyPBCoder::encodeMessage(TinyPBProtocol::s_ptr msg) {
    // TODO: check msg
    if (msg->m_req_id.empty()) {
        msg->m_req_id = "123456";
    }

    // cal pk_len
    int pk_len = 2 * sizeof(char) + 6 * sizeof(int32_t) + msg->m_req_id.size() + msg->m_method_name.size()
                 + msg->m_err_info.size() + msg->m_pb_data.size();
    std::vector<char> ret;
    ret.reserve(pk_len);
    // ! encode pb_start and pk_len
    ret.push_back(TinyPBProtocol::PB_START);
    pk_len = htonl(msg->m_pk_len = pk_len);
    ret.insert(ret.end(), (char *)&pk_len, (char *)&pk_len + sizeof(int32_t));

    // ! encode req_id_len and req_id
    int req_id_len = htonl(msg->m_req_id_len = msg->m_req_id.size());
    ret.insert(ret.end(), (char *)&req_id_len, (char *)&req_id_len + sizeof(int32_t));
    ret.insert(ret.end(), msg->m_req_id.begin(), msg->m_req_id.end());

    // ! encode method_len and method_name
    int method_len = htonl(msg->m_method_name_len = msg->m_method_name.size());
    ret.insert(ret.end(), (char *)&method_len, (char *)&method_len + sizeof(int32_t));
    ret.insert(ret.end(), msg->m_method_name.begin(), msg->m_method_name.end());

    // ! encode err_code and err_info_len and err_info
    int err_code = htonl(msg->m_err_code);
    ret.insert(ret.end(), (char *)&err_code, (char *)&err_code + sizeof(int32_t));
    int err_info_len = htonl(msg->m_err_info_len = msg->m_err_info.size());
    ret.insert(ret.end(), (char *)&err_info_len, (char *)&err_info_len + sizeof(int32_t));
    ret.insert(ret.end(), msg->m_err_info.begin(), msg->m_err_info.end());

    // ! encode pb_data
    ret.insert(ret.end(), msg->m_pb_data.begin(), msg->m_pb_data.end());

    // ! encode checksum
    int32_t checksum = 0;
    ret.insert(ret.end(), (char *)&checksum, (char *)&checksum + sizeof(int32_t));

    ret.push_back(TinyPBProtocol::PB_END);

    checksum = msg->m_checksum = checksum_xor(ret.data(), ret.size());
    // 不需要转换，因为校验时按照4字节异或
    std::memcpy(ret.data() + ret.size() - sizeof(char) - sizeof(int32_t), &checksum, sizeof(int32_t));

    DEBUGLOG("TinyPBCoder encode message success, req_id=%s", msg->m_req_id.c_str());
    return ret;
}

bool TinyPBCoder::checkIndexValid(int index, int len, int end) {
    if (index + len >= end) {
        ERRORLOG("TinyPBCoder decode error, index out of range[field start: %d, field len: %d, package_end: %d]", index,
                 len, end);
        return false;
    }
    return true;
}

int32_t TinyPBCoder::checksum_xor(char *data, int len) {
    // 采用 xor 校验
    int32_t ret = 0;
    int t = len / sizeof(int32_t);
    for (int i = 0; i < t; i++) {
        ret ^= *(int32_t *)(data + i * sizeof(int32_t));
    }
    // ! 处理剩余的字节
    int remain = len % sizeof(int32_t);
    if (remain > 0) {
        int32_t tmp = 0;
        std::memcpy(&tmp, data + t * sizeof(int32_t), remain);
        ret ^= tmp;
    }
    return ret;
}

bool TinyPBCoder::checksum_validate(char *data, int len) {
    // xor
    int32_t checksum = *(int32_t *)(data + len - sizeof(char) - sizeof(int32_t));
    *(int32_t *)(data + len - sizeof(char) - sizeof(int32_t)) = 0;
    int32_t ret = checksum_xor(data, len);
    *(int32_t *)(data + len - sizeof(char) - sizeof(int32_t)) = checksum;
    return ret == checksum;
}

} // namespace rapidrpc