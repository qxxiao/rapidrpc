#include "common/log.h"
#include "common/util.h"
#include "common/config.h"

#include <sys/time.h>
#include <time.h>
#include <sstream>

namespace rapidrpc {

// TODO : 线程安全性需要保证
// * 会被不同线程，或者同一次输出多次调用，必须保证返回相同的对象
static Logger *g_logger = nullptr;
Logger *Logger::GetGlobalLogger() {
    if (g_logger) {
        return g_logger;
    }
    if (!Config::GetGlobalConfig()) {
        printf("Logger::GetGlobalLogger, Config is not set\n");
        exit(0);
    }
    // default is INFO(unkown level string)
    LogLevel global_log_level = logLevelFromString(Config::GetGlobalConfig()->m_log_level);
    static Logger logger(global_log_level); //  thread safe
    g_logger = &logger;
    return g_logger;
}

std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

LogLevel logLevelFromString(const std::string &level) {
    if (level == "DEBUG") {
        return LogLevel::Debug;
    }
    else if (level == "INFO") {
        return LogLevel::Info;
    }
    else if (level == "ERROR") {
        return LogLevel::Error;
    }
    else {
        return LogLevel::Info;
    }
}

std::string LogEvent::toString() {
    struct timeval now_time; // sec and usec
    gettimeofday(&now_time, nullptr);

    struct tm localTime;                       // local time
    localtime_r(&now_time.tv_sec, &localTime); // thread safe version of localtime

    char buf[128];
    strftime(buf, sizeof(buf), "%y-%m-%d %H:%M:%S", &localTime);
    std::string time_str(buf);

    int ms = now_time.tv_usec / 1000;
    // to 3 digits
    time_str = time_str + "." + (ms < 10 ? "00" : (ms < 100 ? "0" : "")) + std::to_string(ms);

    m_pid = getPid();
    m_tid = getThreadId();

    std::stringstream ss;
    ss << "[" << logLevelToString(m_level) << "]\t"
       << "[" << time_str << "]\t"
       << "[" << m_pid << ":" << m_tid << "]\t";
    return ss.str();
}

void Logger::pushLog(const std::string &msg) {
    // 线程安全性
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.push(msg);
}

// TODO: write to file
void Logger::log() {
    // 线程安全性保证
    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<std::string> output(std::move(m_buffer)); // move out of
    lock.~lock_guard();

    while (!output.empty()) {
        std::string msg = output.front();
        output.pop();

        printf("%s", msg.c_str());
        // debug for logger
        // TODO: print logger address
        // printf("%p\n", this);
    }
}

} // namespace rapidrpc
