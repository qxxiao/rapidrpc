#ifndef RAPIDRPC_COMMON_UTIL_H
#define RAPIDRPC_COMMON_UTIL_H

#include <sys/types.h>
#include <unistd.h>
namespace rapidrpc {
pid_t getPid();
pid_t getThreadId();
} // namespace rapidrpc
#endif // !RAPIDRPC_COMMON_UTIL_H
