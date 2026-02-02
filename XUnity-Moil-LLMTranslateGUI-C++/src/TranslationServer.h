#pragma once

// Qt 核心模块
#include <QObject>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <deque>
#include <mutex>
#include <map>
#include <atomic> 
#include "ConfigManager.h"
#include "httplib.h"

/**
 * 上下文结构体，用于存储对话历史
 * Context struct, used to store conversation history
 */
struct Context {
    std::deque<std::pair<QString, QString>> history; // 历史对话记录 (role, content) / History conversation records (role, content)
    int max_len; // 最大历史记录数 / Maximum history records count
};

/**
 * 翻译服务器类 - Translation Server Class
 * 
 * 负责运行HTTP服务器，处理翻译请求，管理API密钥轮换和上下文记忆
 * Responsible for running HTTP server, handling translation requests, managing API key rotation and context memory
 */
class TranslationServer : public QObject {
    Q_OBJECT
    
public:
    explicit TranslationServer(QObject *parent = nullptr);
    ~TranslationServer();
    
    /**
     * 更新服务器配置 / Update server configuration
     * @param config 新的配置 / New configuration
     */
    void updateConfig(const AppConfig& config);
    
    /**
     * 🔥 获取当前配置副本 (线程安全) / Get current configuration copy (thread-safe)
     * @return 当前服务器配置 / Current server configuration
     */
    AppConfig getConfig(); 
    
    /**
     * 启动服务器 / Start the server
     */
    void startServer();
    
    /**
     * 停止服务器 / Stop the server
     */
    void stopServer();
    
    /**
     * 清除所有客户端上下文 / Clear all client contexts
     */
    void clearAllContexts();
    
    /**
     * 检查服务器是否正在运行 / Check if server is running
     * @return 运行状态 / Running status
     */
    bool isRunning() const { return m_running; }

signals:
    /**
     * 日志消息信号 / Log message signal
     * @param msg 日志消息 / Log message
     */
    void logMessage(QString msg);
    
    /**
     * Token使用量信号 / Token usage signal
     * @param prompt 提示词token数 / Prompt token count
     * @param completion 完成token数 / Completion token count
     */
    void tokenUsageReceived(int prompt, int completion);
    
    /**
     * 工作开始信号 / Work started signal
     */
    void workStarted();
    
    /**
     * 工作完成信号 / Work finished signal
     * @param success 是否成功 / Whether successful
     */
    void workFinished(bool success);

private:
    /**
     * 运行服务器主循环 / Run server main loop
     */
    void runServerLoop();
    
    /**
     * 执行翻译 / Perform translation
     * @param text 要翻译的文本 / Text to translate
     * @param clientIP 客户端IP地址 / Client IP address
     * @return 翻译结果 / Translation result
     */
    QString performTranslation(const QString& text, const QString& clientIP);
    
    /**
     * 获取下一个API密钥（轮询） / Get next API key (round-robin)
     * @return API密钥 / API key
     */
    QString getNextApiKey();
    
    /**
     * 生成客户端ID / Generate client ID
     * @param ip 客户端IP地址 / Client IP address
     * @return 客户端ID / Client ID
     */
    QString generateClientId(const std::string& ip);
    
    /**
     * 执行单次翻译尝试 / Perform single translation attempt
     * @param text 要翻译的文本 / Text to translate
     * @param clientIP 客户端IP地址 / Client IP address
     * @return 翻译结果 / Translation result
     */
    QString performSingleTranslationAttempt(const QString& text, const QString& clientIP);
    
    /**
     * 验证翻译结果有效性 / Validate translation result
     * @param result 翻译结果 / Translation result
     * @return 是否有效 / Whether valid
     */
    bool isValidTranslationResult(const QString& result);

    /**
     * 🧊 本地转义替换 - 保护特殊标签不被LLM处理
     * 🧊 Local escape replacement - protect special tags from being processed by LLM
     * @param input 输入文本 / Input text
     * @param context 转义映射上下文 / Escape mapping context
     * @return 处理后的文本 / Processed text
     */
    QString freezeEscapesLocal(const QString& input, struct EscapeMap& context); 
    
    /**
     * 🧊 本地转义恢复 - 将保护标签恢复为原始内容
     * 🧊 Local escape restoration - restore protected tags to original content
     * @param input 输入文本 / Input text
     * @param context 转义映射上下文 / Escape mapping context
     * @return 恢复后的文本 / Restored text
     */
    QString thawEscapesLocal(const QString& input, const struct EscapeMap& context);

private:
    AppConfig m_config; // 当前配置 / Current configuration
    std::atomic<bool> m_running; // 服务器运行状态 / Server running status
    std::atomic<bool> m_stopRequested; // 停止请求标志 / Stop request flag
    
    std::thread* m_serverThread = nullptr; // 服务器线程 / Server thread
    httplib::Server* m_svr = nullptr; // HTTP服务器实例 / HTTP server instance
    
    std::map<std::string, Context> m_contexts; // 客户端上下文映射 / Client context map
    std::mutex m_contextMutex; // 上下文互斥锁 / Context mutex
    
    std::vector<QString> m_apiKeys; // API密钥列表 / API key list
    int m_currentKeyIndex = 0; // 当前密钥索引 / Current key index
    std::mutex m_keyMutex; // 密钥互斥锁 / Key mutex
    
    // 配置互斥锁 / Configuration mutex
    std::mutex m_configMutex;
};