#pragma once
#include <QString>
#include <QSettings>

// 应用程序配置结构体，用于存储所有设置项
// Structure to hold application configuration and store all settings
struct AppConfig {
    // 默认 API 地址 / Default API address
    QString api_address = "https://api.openai.com/v1";
    // 默认 API 密钥 / Default API key
    QString api_key = "sk-xxxxxxxx";
    // 模型名称 / Model name
    QString model_name = "gpt-3.5-turbo";
    // 服务端口号 / Service port number
    int port = 6800;
    // 系统提示词 / System prompt
    QString system_prompt;
    // 预设提示词（用于翻译任务） / Pre-prompt (used for translation tasks)
    QString pre_prompt = "将下面的文本翻译成简体中文：";
    // 上下文数量 / Number of context items
    int context_num = 5;
    // 温度参数（控制随机性） / Temperature parameter (controls randomness)
    double temperature = 1.0;
    // 最大线程数 / Maximum number of threads
    int max_threads = 8;
    // 语言设置 / Language setting
    int language = 1; 
    
    // --- 新增 / New Additions ---
    // 是否开启术语表 / Whether to enable the glossary
    bool enable_glossary = false; 
    // _Substitutions.txt 路径 / Path to _Substitutions.txt
    QString glossary_path = "";   

    // 构造函数 / Constructor
    AppConfig() {
        // 初始化默认的系统提示词
        // Initialize the default system prompt
        system_prompt = "你是一个游戏翻译模型，可以流畅通顺地将任意的游戏文本翻译成简体中文。";
    }
};

// 配置管理器类，负责配置的加载与保存
// ConfigManager class, responsible for loading and saving configurations
class ConfigManager {
public:
    // 从文件加载配置（默认为 config.ini）
    // Load configuration from file (defaults to config.ini)
    static AppConfig loadConfig(const QString& filename = "config.ini");

    // 将配置保存到文件（默认为 config.ini）
    // Save configuration to file (defaults to config.ini)
    static void saveConfig(const AppConfig& config, const QString& filename = "config.ini");
};
