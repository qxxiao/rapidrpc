#include "common/runtime.h"

namespace rapidrpc {

thread_local Runtime *t_runtime = nullptr;

Runtime *Runtime::GetRuntime() {
    if (t_runtime) {
        return t_runtime;
    }

    t_runtime = new Runtime();
    return t_runtime;
}

std::string Runtime::getMsgId() const {
    return m_msg_id;
}

std::string Runtime::getMethodName() const {
    return m_method_name;
}

void Runtime::setMsgId(const std::string &msg_id) {
    m_msg_id = msg_id;
}

void Runtime::setMethodName(const std::string &method_name) {
    m_method_name = method_name;
}

} // namespace rapidrpc