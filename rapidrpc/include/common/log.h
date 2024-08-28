#ifndef RAPIDRPC_COMMON_LOG_H
#define RAPIDRPC_COMMON_LOG_H

#include "net/timer_event.h"

#include <string>
#include <queue>
#include <vector>
#include <mutex>
#include <memory>
#include <semaphore.h>
#include <thread>
#include <condition_variable>
#include <mutex>

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

//* debug log macro
#define DEBUGLOG(str, ...)                                                                                             \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Debug) {                             \
        rapidrpc::Logger::GetGlobalLogger()->pushLog(rapidrpc::LogEvent(rapidrpc::LogLevel::Debug).toString() + "["    \
                                                     + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"  \
                                                     + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");             \
    }

#define INFOLOG(str, ...)                                                                                              \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Info) {                              \
        rapidrpc::Logger::GetGlobalLogger()->pushLog(rapidrpc::LogEvent(rapidrpc::LogLevel::Info).toString() + "["     \
                                                     + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"  \
                                                     + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");             \
    }

#define ERRORLOG(str, ...)                                                                                             \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Error) {                             \
        rapidrpc::Logger::GetGlobalLogger()->pushLog(rapidrpc::LogEvent(rapidrpc::LogLevel::Error).toString() + "["    \
                                                     + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"  \
                                                     + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");             \
    }

// ===========================================================
//* app log macro
// only use in app service code
#define APPDEBUGLOG(str, ...)                                                                                          \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Debug) {                             \
        rapidrpc::Logger::GetGlobalLogger()->pushAppLog(rapidrpc::LogEvent(rapidrpc::LogLevel::Debug).toString() + "[" \
                                                        + std::string(__FILE__) + ":" + std::to_string(__LINE__)       \
                                                        + "]\t" + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");  \
    }

#define APPINFOLOG(str, ...)                                                                                           \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Info) {                              \
        rapidrpc::Logger::GetGlobalLogger()->pushAppLog(rapidrpc::LogEvent(rapidrpc::LogLevel::Info).toString() + "["  \
                                                        + std::string(__FILE__) + ":" + std::to_string(__LINE__)       \
                                                        + "]\t" + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");  \
    }

#define APPERRORLOG(str, ...)                                                                                          \
    if (rapidrpc::Logger::GetGlobalLogger()->getLogLevel() <= rapidrpc::LogLevel::Error) {                             \
        rapidrpc::Logger::GetGlobalLogger()->pushAppLog(rapidrpc::LogEvent(rapidrpc::LogLevel::Error).toString() + "[" \
                                                        + std::string(__FILE__) + ":" + std::to_string(__LINE__)       \
                                                        + "]\t" + rapidrpc::formatString(str, ##__VA_ARGS__) + "\n");  \
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

    //* return log string: [level]\t[time]\t[pid:tid]
    //* full log format: [%y-%m-%d %H:%M:%s.%ms]\t[pid:tid]\t[file_name:line]\t[%msg]
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
// pre-declare AsyncLogger and AsyncLogger::s_ptr
class AsyncLogger;
enum class LogType { AsyncLog = 1, SyncLog = 2 };
class Logger {
public:
    typedef std::shared_ptr<Logger> s_ptr;

public:
    Logger(LogLevel level = LogLevel::Debug, LogType log_type = LogType::AsyncLog);
    void init();

    LogLevel getLogLevel() const {
        return m_set_level;
    }
    void pushLog(const std::string &msg);
    void pushAppLog(const std::string &msg);
    void log();

    void syncLogs();

    void flushAndStop();

public:
    static Logger *GetGlobalLogger();
    static void InitGlobalLogger();

private:
    LogLevel m_set_level;

    std::mutex m_mutex;
    std::mutex m_app_mutex;
    std::vector<std::string> m_buffer;     // rpc log buffer
    std::vector<std::string> m_app_buffer; // app log buffer

    std::shared_ptr<AsyncLogger> m_async_logger;
    std::shared_ptr<AsyncLogger> m_async_app_logger;

    TimerEvent::s_ptr m_timer_event;

    LogType m_log_type;
};

/**
 * 异步日志器，用于记录日志到指定文件中
 */
class AsyncLogger {
public:
    using s_ptr = std::shared_ptr<AsyncLogger>;

public:
    AsyncLogger(const std::string &file_name, const std::string &file_path, int max_file_size);

    /**
     * @brief loop to write logs to file
     * @note 如果当前缓冲区为空，会阻塞等待，直到被其它线程通知
     */
    void loop();
    void flush();
    /**
     * @brief push logs to buffer, transfer ownership
     */
    void pushLogs(std::vector<std::string> &logs);

    void stop();

private:
    std::queue<std::vector<std::string>> m_buffer; // log buffer

    std::string m_file_name; // log file name
    std::string m_file_path; // log file path， include '/'
    int m_log_max_file_size; // max file size, unit: byte

    sem_t m_sem; // semaphore
    std::thread m_thread;

    /**
     *  打印线程，在该条件变量上等待，等待有日志到来
     */
    std::condition_variable m_cond; // cv
    std::mutex m_mutex;

    std::string m_date;            // 当前打印日志的文件日期
    FILE *m_file_handler{nullptr}; // file handler

    bool m_reopen_flag{false}; // reopen file flag

    int m_no{0}; // file no

    bool m_stop_flag{false}; // stop flag
};
} // namespace rapidrpc

#endif
