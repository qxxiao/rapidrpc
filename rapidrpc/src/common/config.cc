#include "common/config.h"

#include <tinyxml/tinyxml.h>

#define READ_XML_ELEMENT(name, parent)                                                                                 \
    TiXmlElement *name##_element = parent->FirstChildElement(#name);                                                   \
    if (!name##_element) {                                                                                             \
        printf("Start server error, %s element is null\n", #name);                                                     \
        exit(-1);                                                                                                      \
    }

#define READ_STR_FROM_XML_NODE(name, parent)                                                                           \
    TiXmlElement *name##_element = parent->FirstChildElement(#name);                                                   \
    if (!name##_element || !name##_element->GetText()) {                                                               \
        printf("Start server error, %s element is null\n", #name);                                                     \
        exit(-1);                                                                                                      \
    }                                                                                                                  \
    std::string name = std::string(name##_element->GetText());

namespace rapidrpc {

static Config *g_config = nullptr; // global config
// TODO: 和写需要保证线程安全
Config *Config::GetGlobalConfig() {
    return g_config;
}
// TODO: 线程安全性需要保证, 在 main 中调用一次
void Config::SetGlobalConfig(const char *xmlfile) {
    if (g_config) {
        delete g_config;
        g_config = nullptr;
    }
    g_config = new Config(xmlfile);
}

Config::Config(const char *xmlfile) {

    TiXmlDocument *xml_document = new TiXmlDocument();

    bool load_ok = xml_document->LoadFile(xmlfile);
    if (!load_ok) {
        printf("Start server error, load config file %s failed\n", xmlfile);
        exit(-1);
    }

    //* params: xml's element name, parent_element
    READ_XML_ELEMENT(root, xml_document);
    READ_XML_ELEMENT(log, root_element);

    // read str from parent node
    READ_STR_FROM_XML_NODE(log_level, log_element);
    READ_STR_FROM_XML_NODE(log_file_name, log_element);
    READ_STR_FROM_XML_NODE(log_file_path, log_element);
    READ_STR_FROM_XML_NODE(log_sync_interval, log_element);
    READ_STR_FROM_XML_NODE(log_max_file_size, log_element);

    m_log_level = std::move(log_level);

    m_log_file_name = std::move(log_file_name);
    m_log_file_path = std::move(log_file_path);
    m_log_sync_interval = std::stoi(log_sync_interval);
    m_log_max_file_size = std::stoi(log_max_file_size);

    printf("Config load success\n");
    printf("log level[%s], log file name[%s], log file path[%s], log sync interval[%d], log max file size[%d]\n",
           m_log_level.c_str(), m_log_file_name.c_str(), m_log_file_path.c_str(), m_log_sync_interval,
           m_log_max_file_size);
}

} // namespace rapidrpc