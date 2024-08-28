#include "common/log.h"
#include "common/util.h"
#include "common/config.h"
#include "net/eventloop.h"
#include "common/runtime.h"

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
        exit(-1);
    }
    // default is INFO(unkown level string)
    LogLevel global_log_level = logLevelFromString(Config::GetGlobalConfig()->m_log_level);
    static Logger logger(global_log_level); //  thread safe
    g_logger = &logger;
    return g_logger;
}

void Logger::InitGlobalLogger() {
    // 二段构造, 先初始化，再调用 init
    // 主要是为了避免使用定时任务，间接调用 DEBUGLOG, 造成死循环
    // 主线程中提前初始化 g_logger， 然后在 init 中初始化定时任务
    GetGlobalLogger()->init();
}

Logger::Logger(LogLevel level) : m_set_level(level) {
    m_async_logger = std::make_shared<AsyncLogger>(Config::GetGlobalConfig()->m_log_file_name + "_rpc",
                                                   Config::GetGlobalConfig()->m_log_file_path,
                                                   Config::GetGlobalConfig()->m_log_max_file_size);

    m_async_app_logger = std::make_shared<AsyncLogger>(Config::GetGlobalConfig()->m_log_file_name + "_app",
                                                       Config::GetGlobalConfig()->m_log_file_path,
                                                       Config::GetGlobalConfig()->m_log_max_file_size);
}

// 二段构造
void Logger::init() {

    // !! 不能在构造函数中直接初始化 m_timer_event，因为 TimerEvent 的构造函数中会调用 DEBUGLOG, 造成死循环
    m_timer_event = std::make_shared<TimerEvent>(Config::GetGlobalConfig()->m_log_sync_interval, true,
                                                 std::bind(&Logger::syncLogs, this));
    EventLoop::GetCurrentEventLoop()->addTimerEvent(m_timer_event);
}

void Logger::syncLogs() {
    // 线程安全性, 同步日志到 AsyncLogger 的缓冲区中
    std::vector<std::string> tmp;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        tmp.swap(m_buffer);
    }
    if (!tmp.empty() && m_async_logger) {
        m_async_logger->pushLogs(tmp);
    }

    // 线程安全性, 同步日志到 AsyncLogger 的缓冲区中
    std::vector<std::string> app_tmp;
    {
        std::lock_guard<std::mutex> lock(m_app_mutex);
        app_tmp.swap(m_app_buffer);
    }
    if (!app_tmp.empty() && m_async_app_logger) {
        m_async_app_logger->pushLogs(app_tmp);
    }
}

void Logger::flushAndStop() {
    // 如果有剩余的日志，同步到 m_async_logger | m_async_app_logger
    syncLogs();

    if (m_async_logger) {
        m_async_logger->stop(); // join thread
    }
    if (m_async_app_logger) {
        m_async_app_logger->stop(); // join thread
    }
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
    // ! app runtime log, eg. [msg_id:method_name]
    std::string msg_id = Runtime::GetRuntime()->getMsgId();
    std::string method_name = Runtime::GetRuntime()->getMethodName();
    if (!msg_id.empty() && !method_name.empty()) {
        ss << "[" << msg_id << ":" << method_name << "]\t";
    }
    return ss.str();
}

void Logger::pushLog(const std::string &msg) {
    // 线程安全性
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.push_back(msg);
}

void Logger::pushAppLog(const std::string &msg) {
    // 线程安全性
    std::lock_guard<std::mutex> lock(m_app_mutex);
    m_app_buffer.push_back(msg);
}

// @deprecated : use AsyncLogger to log to file scheduled by time instead
void Logger::log() {
    // // 线程安全性保证
    // std::lock_guard<std::mutex> lock(m_mutex);
    // std::queue<std::string> output(std::move(m_buffer));
    // lock.~lock_guard();

    // while (!output.empty()) {
    //     std::string msg = output.front();
    //     output.pop();

    //     printf("%s", msg.c_str());
    //     // debug for logger
    //     // TODO: print logger address
    //     // printf("%p\n", this);
    // }
}

// AsyncLogger
AsyncLogger::AsyncLogger(const std::string &file_name, const std::string &file_path, int max_file_size)
    : m_file_name(file_name), m_file_path(file_path), m_log_max_file_size(max_file_size) {
    sem_init(&m_sem, 0, 0);
    m_thread = std::thread(&AsyncLogger::loop, this);
    sem_wait(&m_sem); // wait for thread to start
}

void AsyncLogger::loop() {
    sem_post(&m_sem);

    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] {
            return !m_buffer.empty() || m_stop_flag;
        });

        if (m_buffer.empty()) { // stop
            break;
        }

        std::vector<std::string> output(std::move(m_buffer.front()));
        m_buffer.pop();
        lock.unlock();

        // cur time(year-month-day) when log
        timeval now_time;
        gettimeofday(&now_time, nullptr);

        struct tm localTime;
        localtime_r(&now_time.tv_sec, &localTime);

        const char *format = "%Y%m%d";
        static char date[32];
        strftime(date, sizeof(date), format, &localTime);

        // 第一次打开文件/跨天
        if (std::string(date) != m_date) {
            m_no = 0;
            m_reopen_flag = true; // open another file
            m_date = std::string(date);
        }
        // 文件大小超过限制
        else if (ftell(m_file_handler) >= m_log_max_file_size) {
            m_no++;
            m_reopen_flag = true;
        }

        if (m_reopen_flag) {
            if (m_file_handler) {
                fclose(m_file_handler);
                m_file_handler = nullptr;
            }
            std::stringstream ss;
            ss << m_file_path << m_file_name << "_" << m_date << "_log." << m_no;
            std::string log_file_name = ss.str();

            m_file_handler = fopen(log_file_name.c_str(), "a");
            if (m_file_handler == nullptr) {
                printf("AsyncLogger::loop, open file failed\n");
                exit(-1);
            }
            m_reopen_flag = false;
        }

        for (auto &msg : output) {
            if (!msg.empty()) {
                // fprintf(m_file_handler, "%s\n", msg.c_str());
                fwrite(msg.c_str(), 1, msg.size(), m_file_handler);
            }
        }
        fflush(m_file_handler);
    }
}

void AsyncLogger::flush() {
    if (m_file_handler) {
        fflush(m_file_handler);
    }
}

void AsyncLogger::pushLogs(std::vector<std::string> &logs) {

    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.push(std::move(logs));
    m_cond.notify_one();
    // unlock
}

void AsyncLogger::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop_flag = true;
        m_cond.notify_one();
    }
    m_thread.join();
}

} // namespace rapidrpc
