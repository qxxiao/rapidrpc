#ifndef RAPIDRPC_COMMON_CONFIG_H
#define RAPIDRPC_COMMON_CONFIG_H

#include <string>

namespace rapidrpc {

/**
 * @brief Config class
 * 配置类，用于读取配置文件，在全局 main 中调用 SetGlobalConfig 一次,
 * 后续线程只是读取，不用考虑线程安全性
 */
class Config {
private:
    Config(const char *xmlfile);

public:
    static Config *GetGlobalConfig();
    static void SetGlobalConfig(const char *xmlfile);

public:
    // std::unordered_map<std::string, std::string> m_config_values;
    std::string m_log_level;

    std::string m_log_file_name;
    std::string m_log_file_path;
    int m_log_max_file_size; // bytes
    int m_log_sync_interval; // ms
};

} // namespace rapidrpc

#endif // !RAPIDRPC_COMMON_CONFIG_H
