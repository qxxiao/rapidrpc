/**
 * @file msg_id_util.h
 * @brief 消息 ID 工具类
 */

#ifndef RAPIDRPC_COMMON_MSG_ID_UTIL_H
#define RAPIDRPC_COMMON_MSG_ID_UTIL_H

#include <string>

namespace rapidrpc {

class MsgIdUtil {
public:
    static std::string GenMsgId();
};
} // namespace rapidrpc

#endif // RAPIDRPC_COMMON_MSG_ID_UTIL_H