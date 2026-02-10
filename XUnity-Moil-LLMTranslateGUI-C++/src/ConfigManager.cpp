#include "ConfigManager.h"

/**
 * 加载配置文件 / Load configuration file
 * 
 * 从指定的INI格式配置文件中读取应用程序设置，并返回配置对象。
 * Reads application settings from the specified INI format configuration file and returns a configuration object.
 * 
 * @param filename 配置文件名（可选，默认为"config.ini"） / Configuration file name (optional, defaults to "config.ini")
 * @return 包含所有配置参数的AppConfig对象 / AppConfig object containing all configuration parameters
 */
AppConfig ConfigManager::loadConfig(const QString& filename) {
    // 创建QSettings对象，使用INI格式 / Create QSettings object with INI format
    QSettings settings(filename, QSettings::IniFormat);
    
    // 创建默认配置对象 / Create default configuration object
    AppConfig config;

    // API配置参数 / API Configuration Parameters
    // 读取API地址，如果不存在则使用默认值 / Read API address, use default if not exists
    config.api_address = settings.value("Settings/api_address", config.api_address).toString();
    
    // 读取API密钥，如果不存在则使用默认值 / Read API key, use default if not exists
    config.api_key = settings.value("Settings/api_key", config.api_key).toString();
    
    // 读取模型名称，如果不存在则使用默认值 / Read model name, use default if not exists
    config.model_name = settings.value("Settings/model_name", config.model_name).toString();
    
    // 读取端口号，如果不存在则使用默认值 / Read port number, use default if not exists
    config.port = settings.value("Settings/port", config.port).toInt();
    
    // 读取系统提示词，如果不存在则使用默认值 / Read system prompt, use default if not exists
    config.system_prompt = settings.value("Settings/system_prompt", config.system_prompt).toString();
    
    // 读取前置文本，如果不存在则使用默认值 / Read pre-prompt, use default if not exists
    config.pre_prompt = settings.value("Settings/pre_prompt", config.pre_prompt).toString();
    
    // 读取上下文数量，如果不存在则使用默认值 / Read context number, use default if not exists
    config.context_num = settings.value("Settings/context_num", config.context_num).toInt();
    
    // 读取温度参数，如果不存在则使用默认值 / Read temperature parameter, use default if not exists
    config.temperature = settings.value("Settings/temperature", config.temperature).toDouble();
    
    // 读取最大线程数，如果不存在则使用默认值 / Read max threads, use default if not exists
    config.max_threads = settings.value("Settings/max_threads", config.max_threads).toInt();
    
    // 读取界面语言设置，如果不存在则使用默认值 / Read UI language setting, use default if not exists
    config.language = settings.value("Settings/language", config.language).toInt();
    
    // --- 术语表相关设置 --- / --- Glossary Related Settings ---
    
    // 读取是否启用术语表，如果不存在则使用默认值 / Read whether to enable glossary, use default if not exists
    config.enable_glossary = settings.value("Settings/enable_glossary", config.enable_glossary).toBool();
    
    // 读取术语表文件路径，如果不存在则使用默认值 / Read glossary file path, use default if not exists
    config.glossary_path = settings.value("Settings/glossary_path", config.glossary_path).toString();
    
    // 读取术语表历史记录（字符串列表），如果不存在则返回空列表 / Read glossary history (string list), return empty list if not exists
    config.glossary_history = settings.value("Settings/glossary_history").toStringList();
    
    // 返回完整的配置对象 / Return complete configuration object
    return config;
}

/**
 * 保存配置文件 / Save configuration file
 * 
 * 将应用程序配置保存到指定的INI格式配置文件中。
 * Saves application configuration to the specified INI format configuration file.
 * 
 * @param config 包含要保存的配置参数的AppConfig对象 / AppConfig object containing configuration parameters to save
 * @param filename 配置文件名（可选，默认为"config.ini"） / Configuration file name (optional, defaults to "config.ini")
 */
void ConfigManager::saveConfig(const AppConfig& config, const QString& filename) {
    // 创建QSettings对象，使用INI格式 / Create QSettings object with INI format
    QSettings settings(filename, QSettings::IniFormat);

    // API配置参数 / API Configuration Parameters
    // 保存API地址到配置文件 / Save API address to configuration file
    settings.setValue("Settings/api_address", config.api_address);
    
    // 保存API密钥到配置文件 / Save API key to configuration file
    settings.setValue("Settings/api_key", config.api_key);
    
    // 保存模型名称到配置文件 / Save model name to configuration file
    settings.setValue("Settings/model_name", config.model_name);
    
    // 保存端口号到配置文件 / Save port number to configuration file
    settings.setValue("Settings/port", config.port);
    
    // 保存系统提示词到配置文件 / Save system prompt to configuration file
    settings.setValue("Settings/system_prompt", config.system_prompt);
    
    // 保存前置文本到配置文件 / Save pre-prompt to configuration file
    settings.setValue("Settings/pre_prompt", config.pre_prompt);
    
    // 保存上下文数量到配置文件 / Save context number to configuration file
    settings.setValue("Settings/context_num", config.context_num);
    
    // 保存温度参数到配置文件 / Save temperature parameter to configuration file
    settings.setValue("Settings/temperature", config.temperature);
    
    // 保存最大线程数到配置文件 / Save max threads to configuration file
    settings.setValue("Settings/max_threads", config.max_threads);
    
    // 保存界面语言设置到配置文件 / Save UI language setting to configuration file
    settings.setValue("Settings/language", config.language);
    
    // --- 术语表相关设置 --- / --- Glossary Related Settings ---
    
    // 保存是否启用术语表到配置文件 / Save whether to enable glossary to configuration file
    settings.setValue("Settings/enable_glossary", config.enable_glossary);
    
    // 保存术语表文件路径到配置文件 / Save glossary file path to configuration file
    settings.setValue("Settings/glossary_path", config.glossary_path);
    
    // 保存术语表历史记录（字符串列表）到配置文件 / Save glossary history (string list) to configuration file
    settings.setValue("Settings/glossary_history", config.glossary_history);
    
    // 确保所有设置立即写入磁盘 / Ensure all settings are immediately written to disk
    settings.sync();
}