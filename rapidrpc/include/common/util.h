#ifndef RAPIDRPC_COMMON_UTIL_H
#define RAPIDRPC_COMMON_UTIL_H

#include <sys/types.h>
#include <unistd.h>
#include <string>
namespace rapidrpc {
pid_t getPid();
pid_t getThreadId();

ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);

int64_t getNowMs();
std::string getFormatTime(int64_t ms);

} // namespace rapidrpc
#endif // !RAPIDRPC_COMMON_UTIL_H
