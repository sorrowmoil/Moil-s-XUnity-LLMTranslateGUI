#include "ConfigManager.h"

// 实现加载配置的函数
// Implementation of the function to load configuration
AppConfig ConfigManager::loadConfig(const QString& filename) {
    // 创建 QSettings 对象，指定使用 Ini 格式
    // Create a QSettings object, specifying Ini format
    QSettings settings(filename, QSettings::IniFormat);
    
    AppConfig config;

    // 读取各项配置。如果键不存在，则使用 config 中的默认值
    // Read configuration items. If the key does not exist, use the default value in config
    
    config.api_address = settings.value("Settings/api_address", config.api_address).toString();
    config.api_key = settings.value("Settings/api_key", config.api_key).toString();
    config.model_name = settings.value("Settings/model_name", config.model_name).toString();
    config.port = settings.value("Settings/port", config.port).toInt();
    config.system_prompt = settings.value("Settings/system_prompt", config.system_prompt).toString();
    config.pre_prompt = settings.value("Settings/pre_prompt", config.pre_prompt).toString();
    config.context_num = settings.value("Settings/context_num", config.context_num).toInt();
    config.temperature = settings.value("Settings/temperature", config.temperature).toDouble();
    config.max_threads = settings.value("Settings/max_threads", config.max_threads).toInt();
    config.language = settings.value("Settings/language", config.language).toInt();
    
    // --- 新增 / New Additions ---
    // 读取术语表相关设置
    // Read glossary-related settings
    config.enable_glossary = settings.value("Settings/enable_glossary", config.enable_glossary).toBool();
    config.glossary_path = settings.value("Settings/glossary_path", config.glossary_path).toString();
    
    return config;
}

// 实现保存配置的函数
// Implementation of the function to save configuration
void ConfigManager::saveConfig(const AppConfig& config, const QString& filename) {
    // 创建 QSettings 对象准备写入
    // Create QSettings object strictly for writing
    QSettings settings(filename, QSettings::IniFormat);

    // 将当前 config 结构体中的值写入到设置文件中
    // Write values from the current config structure to the settings file
    settings.setValue("Settings/api_address", config.api_address);
    settings.setValue("Settings/api_key", config.api_key);
    settings.setValue("Settings/model_name", config.model_name);
    settings.setValue("Settings/port", config.port);
    settings.setValue("Settings/system_prompt", config.system_prompt);
    settings.setValue("Settings/pre_prompt", config.pre_prompt);
    settings.setValue("Settings/context_num", config.context_num);
    settings.setValue("Settings/temperature", config.temperature);
    settings.setValue("Settings/max_threads", config.max_threads);
    settings.setValue("Settings/language", config.language);
    
    // --- 新增 / New Additions ---
    // 保存术语表相关设置
    // Save glossary-related settings
    settings.setValue("Settings/enable_glossary", config.enable_glossary);
    settings.setValue("Settings/glossary_path", config.glossary_path);
    
    // 强制将更改同步到磁盘（确保数据被写入）
    // Force synchronization of changes to disk (ensure data is written)
    settings.sync();
}