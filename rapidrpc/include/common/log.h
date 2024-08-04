#ifndef RAPIDRPC_COMMON_LOG_H
#define RAPIDRPC_COMMON_LOG_H

#include <string>
#include <queue>
#include <mutex>
#include <memory>
namespace rapidrpc {

//* format to std::string
template <typename... Args>
std::string formatString(const char *str, Args &&...args) {
    int size = snprintf(nullptr, 0, str, args...); // cal size
    std::string result;
    if (size > 0) {
        result.resize(size);
        snprintf(&result[0], size + 1, str, args...);
    }
    return result;
}

// TODO: add __FILE__ and __LINE__
//* debug log macro
#define DEBUGLOG(str, ...)                                                                                             \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Debug) {                             \
        rapidrpc::Logger::GetGlobalLogger()->pushLog((new rapidrpc::LogEvent(rapidrpc::LogLevel::Debug))->toString()   \
                                                     + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__)    \
                                                     + "]\t" + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");     \
        rapidrpc::Logger::GetGlobalLogger()->log();                                                                    \
    }

#define INFOLOG(str, ...)                                                                                              \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Info) {                              \
        rapidrpc::Logger::GetGlobalLogger()->pushLog((new rapidrpc::LogEvent(rapidrpc::LogLevel::Info))->toString()    \
                                                     + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__)    \
                                                     + "]\t" + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");     \
        rapidrpc::Logger::GetGlobalLogger()->log();                                                                    \
    }

#define ERRORLOG(str, ...)                                                                                             \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Error) {                             \
        rapidrpc::Logger::GetGlobalLogger()->pushLog((new rapidrpc::LogEvent(rapidrpc::LogLevel::Error))->toString()   \
                                                     + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__)    \
                                                     + "]\t" + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");     \
        rapidrpc::Logger::GetGlobalLogger()->log();                                                                    \
    }

enum class LogLevel { Debug = 1, Info = 2, Error = 3 };
std::string logLevelToString(LogLevel level);
LogLevel logLevelFromString(const std::string &level);

class LogEvent {
public:
    LogEvent(LogLevel level) : m_level(level) {}

    std::string getFileName() const {
        return m_file_name;
    }

    LogLevel getLogLevel() const {
        return m_level;
    }

    //* return log string
    //* log format: [%y-%m-%d %H:%M:%s.%ms]\t[pid:tid]\t[file_name:line]\t[%msg]
    std::string toString();

private:
    std::string m_file_name; // file name
    int32_t m_file_line;     // file line
    int32_t m_pid;           // process id
    int32_t m_tid;           // thread id
    // std::string m_msg;       // log message

    LogLevel m_level; // log level
};

/**
 * 日志器，用于记录日志
 * - 设置打印级别
 * - 打印到文件，日期命名，日志滚动按照大小切分
 * - 线程安全
 */
class Logger {
public:
    typedef std::shared_ptr<Logger> s_ptr;

    Logger(LogLevel level = LogLevel::Info) : m_set_level(level) {}

    LogLevel getLogLevel() const {
        return m_set_level;
    }
    void pushLog(const std::string &msg);
    void log();

public:
    static Logger *GetGlobalLogger();

private:
    LogLevel m_set_level;

    std::mutex m_mutex;
    std::queue<std::string> m_buffer;
};
} // namespace rapidrpc

#endif
