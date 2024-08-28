#ifndef RAPIDRPC_COMMON_RUNTIME_H
#define RAPIDRPC_COMMON_RUNTIME_H

#include <string>

namespace rapidrpc {

class Runtime {

public:
public:
    static Runtime *GetRuntime();

    std::string getMsgId() const;
    void setMsgId(const std::string &msg_id);

    std::string getMethodName() const;
    void setMethodName(const std::string &method_name);

private:
    std::string m_msg_id;
    std::string m_method_name;
};
} // namespace rapidrpc

#endif // RAPIDRPC_COMMON_RUNTIME_H