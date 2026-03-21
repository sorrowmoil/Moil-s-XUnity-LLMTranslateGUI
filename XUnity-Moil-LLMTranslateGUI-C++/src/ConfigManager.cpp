#include "ConfigManager.h"

/**
 * Load configuration from an INI file.
 * 从INI文件加载配置。
 * 
 * @param filename Path to the INI file ; INI文件路径
 * @return AppConfig structure populated with values from the file ; 填充了文件值的AppConfig结构体
 */
AppConfig ConfigManager::loadConfig(const QString &filename)
{
    QSettings settings(filename, QSettings::IniFormat);
    AppConfig config;

    // Read basic settings with default values ; 读取基本设置并提供默认值
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

    // Glossary related settings ; 术语表相关设置
    config.enable_glossary = settings.value("Settings/enable_glossary", config.enable_glossary).toBool();
    config.glossary_path = settings.value("Settings/glossary_path", config.glossary_path).toString();
    config.glossary_history = settings.value("Settings/glossary_history").toStringList();

    // Lock states for system prompt and glossary ; 系统提示和术语表的锁定状态
    config.lock_system_prompt = settings.value("Settings/lock_system_prompt", false).toBool();
    config.lock_glossary = settings.value("Settings/lock_glossary", false).toBool();

    // UI settings (newly added) ; UI设置（新增）
    // If the INI file does not contain these keys, default values are used: 0 for ui_mode, true for is_dark.
    // 如果INI文件中没有这些键，则使用默认值：ui_mode默认为0，is_dark默认为true。
    config.ui_mode = settings.value("UI/ui_mode", 0).toInt();
    config.is_dark = settings.value("UI/is_dark", true).toBool();

    config.modern_opacity = settings.value("UI/modern_opacity", 210).toInt();
    config.is_rounded = settings.value("UI/is_rounded", true).toBool(); // Read rounded corner state ; 读取圆角状态

    // Debug and batch processing flags ; 调试和批处理标志
    config.enable_debug_mode = settings.value("Settings/enable_debug_mode", false).toBool();
    config.enable_batch = settings.value("Settings/enable_batch", false).toBool(); // Default off ; 默认关闭

    return config;
}

/**
 * Save configuration to an INI file.
 * 将配置保存到INI文件。
 * 
 * @param config The configuration to save ; 要保存的配置
 * @param filename Path to the INI file ; INI文件路径
 */
void ConfigManager::saveConfig(const AppConfig &config, const QString &filename)
{
    QSettings settings(filename, QSettings::IniFormat);

    // Write basic settings ; 写入基本设置
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

    // Glossary related settings ; 术语表相关设置
    settings.setValue("Settings/enable_glossary", config.enable_glossary);
    settings.setValue("Settings/glossary_path", config.glossary_path);
    settings.setValue("Settings/glossary_history", config.glossary_history);

    // Save lock states ; 保存锁定状态
    settings.setValue("Settings/lock_system_prompt", config.lock_system_prompt);
    settings.setValue("Settings/lock_glossary", config.lock_glossary);

    // ============================================================
    // UI settings handling: common and mode-specific values
    // UI设置处理：通用值和模式特定值
    // ============================================================

    // UI mode is always saved (common to both classic and modern modes) ; UI模式总是保存（经典和流光模式通用）
    settings.setValue("UI/ui_mode", config.ui_mode);

    // is_dark is shared by both classic and modern modes, always save it.
    // is_dark 是两种模式共用的属性，始终保存。
    settings.setValue("UI/is_dark", config.is_dark);

    // Save modern-mode specific UI settings only if the config originates from modern mode.
    // This preserves the default values for classic mode when saving from classic mode.
    // 只有当配置来自流光模式时，才保存流光模式专属的UI设置。
    // 这样从经典模式保存时，不会覆盖经典模式的默认值。
    if (config.is_from_modern)
    {
        settings.setValue("UI/is_rounded", config.is_rounded);
        settings.setValue("UI/modern_opacity", config.modern_opacity);
    }

    // Debug and batch flags ; 调试和批处理标志
    settings.setValue("Settings/enable_debug_mode", config.enable_debug_mode);
    settings.setValue("Settings/enable_batch", config.enable_batch);
    
    settings.sync();
}