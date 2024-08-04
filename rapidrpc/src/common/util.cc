#include "common/util.h"
#include <sys/syscall.h>

namespace rapidrpc {

static int g_pid = 0;
static thread_local int g_tid = 0; // thread local
pid_t getPid() {
    if (g_pid != 0)
        return g_pid;
    return g_pid = getpid();
}

pid_t getThreadId() {
    if (g_tid != 0)
        return g_tid;
    return g_tid = syscall(SYS_gettid);
}
} // namespace rapidrpc