#include "ConfigManager.h"
#include <QUrl>

namespace
{
QString normalizeBaseUrl(const QString &url)
{
    QString normalized = url.trimmed();
    while (normalized.endsWith('/'))
    {
        normalized.chop(1);
    }
    return normalized;
}

QString encodeBaseUrlKey(const QString &url)
{
    return QString::fromUtf8(QUrl::toPercentEncoding(url));
}
}

QString ConfigManager::loadApiKeyForBaseUrl(const QString &baseUrl, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return QString();
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("ApiKeysByBaseUrl");
    const QString apiKey = settings.value(encodeBaseUrlKey(normalized)).toString();
    settings.endGroup();
    return apiKey;
}

QString ConfigManager::loadModelForBaseUrl(const QString &baseUrl, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return QString();
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("ModelByBaseUrl");
    const QString modelName = settings.value(encodeBaseUrlKey(normalized)).toString();
    settings.endGroup();
    return modelName;
}

void ConfigManager::saveApiKeyForBaseUrl(const QString &baseUrl, const QString &apiKey, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return;
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("ApiKeysByBaseUrl");
    settings.setValue(encodeBaseUrlKey(normalized), apiKey);
    settings.endGroup();
    settings.sync();
}

void ConfigManager::removeApiKeyForBaseUrl(const QString &baseUrl, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return;
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("ApiKeysByBaseUrl");
    settings.remove(encodeBaseUrlKey(normalized));
    settings.endGroup();
    settings.sync();
}

void ConfigManager::saveModelForBaseUrl(const QString &baseUrl, const QString &modelName, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return;
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("ModelByBaseUrl");
    settings.setValue(encodeBaseUrlKey(normalized), modelName);
    settings.endGroup();
    settings.sync();
}

void ConfigManager::removeModelForBaseUrl(const QString &baseUrl, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return;
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("ModelByBaseUrl");
    settings.remove(encodeBaseUrlKey(normalized));
    settings.endGroup();
    settings.sync();
}

QString ConfigManager::loadPresetNameForBaseUrl(const QString &baseUrl, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return QString();
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("PresetNameByBaseUrl");
    const QString presetName = settings.value(encodeBaseUrlKey(normalized)).toString();
    settings.endGroup();
    return presetName;
}

void ConfigManager::savePresetNameForBaseUrl(const QString &baseUrl, const QString &presetName, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return;
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("PresetNameByBaseUrl");
    settings.setValue(encodeBaseUrlKey(normalized), presetName);
    settings.endGroup();
    settings.sync();
}

void ConfigManager::removePresetNameForBaseUrl(const QString &baseUrl, const QString &filename)
{
    const QString normalized = normalizeBaseUrl(baseUrl);
    if (normalized.isEmpty())
    {
        return;
    }

    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("PresetNameByBaseUrl");
    settings.remove(encodeBaseUrlKey(normalized));
    settings.endGroup();
    settings.sync();
}

// 实现加载配置的函数
AppConfig ConfigManager::loadConfig(const QString &filename)
{
    QSettings settings(filename, QSettings::IniFormat);
    AppConfig config;

    config.api_address = settings.value("Settings/api_address", config.api_address).toString();
    config.api_key = settings.value("Settings/api_key", config.api_key).toString();

    // 若存在按 base_url 记忆的 key，则优先使用映射中的值
    const QString mappedApiKey = loadApiKeyForBaseUrl(config.api_address, filename);
    if (!mappedApiKey.isEmpty())
    {
        config.api_key = mappedApiKey;
    }

    config.model_name = settings.value("Settings/model_name", config.model_name).toString();

    const QString mappedModelName = loadModelForBaseUrl(config.api_address, filename);
    if (!mappedModelName.isEmpty())
    {
        config.model_name = mappedModelName;
    }
    config.port = settings.value("Settings/port", config.port).toInt();
    config.system_prompt = settings.value("Settings/system_prompt", config.system_prompt).toString();
    config.pre_prompt = settings.value("Settings/pre_prompt", config.pre_prompt).toString();
    config.context_num = settings.value("Settings/context_num", config.context_num).toInt();
    config.temperature = settings.value("Settings/temperature", config.temperature).toDouble();
    config.max_threads = settings.value("Settings/max_threads", config.max_threads).toInt();
    config.language = settings.value("Settings/language", config.language).toInt();
    
    config.custom_api_urls = settings.value("Settings/custom_api_urls").toStringList();

    // --- 术语表相关设置 ---
    config.enable_glossary = settings.value("Settings/enable_glossary", config.enable_glossary).toBool();

    config.glossary_path = settings.value("Settings/glossary_path", config.glossary_path).toString();
    config.glossary_history = settings.value("Settings/glossary_history").toStringList();

    config.lock_system_prompt = settings.value("Settings/lock_system_prompt", false).toBool();
    config.lock_glossary = settings.value("Settings/lock_glossary", false).toBool();

    // --- 🔥 新增：读取 UI 设置 ---
    // 如果配置文件里没有这一项，默认返回 0 (Classic) 和 true (Dark)
    config.ui_mode = settings.value("UI/ui_mode", 0).toInt();
    config.is_dark = settings.value("UI/is_dark", true).toBool();

    config.modern_opacity = settings.value("UI/modern_opacity", 210).toInt();
    config.is_rounded = settings.value("UI/is_rounded", true).toBool(); // 👈 新增：读取圆角状态
    config.glass_render_mode = settings.value("UI/glass_render_mode", 0).toInt();
    config.hue_shift = settings.value("UI/hue_shift", 0).toInt();
    config.tint_intensity = settings.value("UI/tint_intensity", 100).toInt();

    config.enable_debug_mode = settings.value("Settings/enable_debug_mode", false).toBool();
    config.enable_batch = settings.value("Settings/enable_batch", false).toBool(); // 默认关闭
    config.handle_rich_text = settings.value("Settings/handle_rich_text", false).toBool(); // 默认关闭
    config.extract_newline = settings.value("Settings/extract_newline", false).toBool(); // 默认开启

    return config;
}

// 实现保存配置的函数
void ConfigManager::saveConfig(const AppConfig &config, const QString &filename)
{
    QSettings settings(filename, QSettings::IniFormat);

    settings.setValue("Settings/api_address", config.api_address);
    settings.setValue("Settings/api_key", config.api_key);
    saveApiKeyForBaseUrl(config.api_address, config.api_key, filename);
    settings.setValue("Settings/model_name", config.model_name);
    saveModelForBaseUrl(config.api_address, config.model_name, filename);
    settings.setValue("Settings/port", config.port);
    settings.setValue("Settings/system_prompt", config.system_prompt);
    settings.setValue("Settings/pre_prompt", config.pre_prompt);
    settings.setValue("Settings/context_num", config.context_num);
    settings.setValue("Settings/temperature", config.temperature);
    settings.setValue("Settings/max_threads", config.max_threads);
    settings.setValue("Settings/language", config.language);
    
    settings.setValue("Settings/custom_api_urls", config.custom_api_urls);

    // --- 术语表相关设置 ---
    settings.setValue("Settings/enable_glossary", config.enable_glossary);
    settings.setValue("Settings/glossary_path", config.glossary_path);
    settings.setValue("Settings/glossary_history", config.glossary_history);

    // --- 保存锁定状态 ---
    settings.setValue("Settings/lock_system_prompt", config.lock_system_prompt);
    settings.setValue("Settings/lock_glossary", config.lock_glossary);

    // ==========================================
    // 🔥 CAN: 修复核心区域开始
    // ==========================================

    settings.setValue("UI/ui_mode", config.ui_mode); // 模式标记是共用的，必须存

    // 💡 修复点：is_dark 是双界面共用的属性，必须放在防御屏障之外！
    settings.setValue("UI/is_dark", config.is_dark);

    // 只有当配置是流光模式发出时，才允许覆写【流光专属】UI 参数！
    // 这样经典模式保存时，硬盘里的圆角和透明度数据就能作为“中间值”被完美保护！
    if (config.is_from_modern)
    {
        settings.setValue("UI/is_rounded", config.is_rounded);
        settings.setValue("UI/modern_opacity", config.modern_opacity);
        settings.setValue("UI/glass_render_mode", config.glass_render_mode);
        settings.setValue("UI/hue_shift", config.hue_shift);
        settings.setValue("UI/tint_intensity", config.tint_intensity);
    }

    // ==========================================
    // 🔥 CAN: 修复核心区域结束
    // ==========================================

    settings.setValue("Settings/enable_debug_mode", config.enable_debug_mode);
    settings.setValue("Settings/enable_batch", config.enable_batch);
    settings.setValue("Settings/handle_rich_text", config.handle_rich_text);
    settings.setValue("Settings/extract_newline", config.extract_newline);
    
    settings.sync();
}