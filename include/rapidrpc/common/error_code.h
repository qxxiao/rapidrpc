#ifndef RAPIDRPC_COMMON_ERROR_CODE_H
#define RAPIDRPC_COMMON_ERROR_CODE_H

#ifndef SYS_ERROR_PREFIX
#    define SYS_ERROR_PREFIX(xxx) 1000##xxx
#endif

namespace rapidrpc {

enum class Error {
    OK = 0,

    // system error
    SYS_PEER_CLOSED = SYS_ERROR_PREFIX(0000),        // 对端关闭连接
    SYS_FAILED_CONNECT = SYS_ERROR_PREFIX(0001),     // 连接失败
    SYS_FAILED_GET_REPLY = SYS_ERROR_PREFIX(0002),   // 获取回复失败
    SYS_FAILED_DESERIALIZE = SYS_ERROR_PREFIX(0003), // 反序列化失败
    SYS_FAILED_SERIALIZE = SYS_ERROR_PREFIX(0004),   // 序列化失败

    SYS_FAILED_ENCODE = SYS_ERROR_PREFIX(0005), // 编码失败
    SYS_FAILED_DECODE = SYS_ERROR_PREFIX(0006), // 解码失败

    SYS_RPC_CALL_TIMEOUT = SYS_ERROR_PREFIX(0007), // rpc 调用超时

    SYS_SERVICE_NOT_FOUND = SYS_ERROR_PREFIX(0008), // 服务未找到
    SYS_METHOD_NOT_FOUND = SYS_ERROR_PREFIX(0009),  // 方法未找到

    SYS_FAILED_PARSE_SERVICE_NAME = SYS_ERROR_PREFIX(0010), // 解析服务名失败

    SYS_CHANNEL_NOT_INIT = SYS_ERROR_PREFIX(0011), // channel 未初始化
};
}

#endif // RAPIDRPC_COMMON_ERROR_CODE