#include "rapidrpc/common/util.h"

#include <sys/syscall.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

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

// 封装 write 函数，确保写入指定的字节数
ssize_t writen(int fd, const void *buf, size_t count) {
    const char *b = static_cast<const char *>(buf);
    ssize_t bytes_written = 0;

    while (bytes_written < static_cast<ssize_t>(count)) {
        ssize_t result = write(fd, b + bytes_written, count - bytes_written);
        if (result == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                // 如果是由于EINTR中断，继续写入剩余的数据
                continue;
            }
            else {
                // ERRORLOG("Write error: %s", strerror(errno));
                return -1;
            }
        }
        bytes_written += result;
    }
    return bytes_written;
}

// 封装 read 函数，确保读取指定的字节数(如果读取到文件末尾，返回读取的字节数)
ssize_t readn(int fd, void *buf, size_t count) {
    char *b = static_cast<char *>(buf);
    ssize_t bytes_read = 0;

    while (bytes_read < static_cast<ssize_t>(count)) {
        ssize_t result = read(fd, b + bytes_read, count - bytes_read);
        if (result == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                // 如果是由于EINTR中断，继续读取剩余的数据
                continue;
            }
            else {
                // ERRORLOG("Read error: %s", strerror(errno));
                return -1;
            }
        }
        else if (result == 0) {
            // 读取到文件末尾，网络编程通常表示连接已经关闭
            break;
        }
        bytes_read += result;
    }
    return bytes_read;
}

int64_t getNowMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t now = (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return now;
}

// for debug log
std::string getFormatTime(int64_t ms) {

    time_t seconds = ms / 1000;
    ms = ms % 1000;

    struct tm localTime;
    localtime_r(&seconds, &localTime);

    char buf[64] = {0};
    strftime(buf, sizeof(buf), "%y-%m-%d %H:%M:%S", &localTime);
    std::string result(buf);
    std::string str_ms = std::to_string(ms);
    if (ms < 10) {
        str_ms = "00" + str_ms;
    }
    else if (ms < 100) {
        str_ms = "0" + str_ms;
    }
    result += "." + str_ms;
    return result;
}

} // namespace rapidrpc