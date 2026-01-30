#pragma once
#include <QObject>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <deque>
#include <mutex>
#include <map>
#include <atomic> // 必须包含 / Must include
#include "ConfigManager.h"
#include "httplib.h"

/**
 * 对话上下文结构体
 * Conversation Context Structure
 * 用于存储和管理每个客户端的对话历史记录
 * Used to store and manage conversation history for each client
 */
struct Context {
    std::deque<std::pair<QString, QString>> history; // 历史对话记录 (用户输入, AI回复) / History conversation records (user input, AI response)
    int max_len; // 最大历史记录长度 / Maximum history length
};

/**
 * 翻译服务器类
 * Translation Server Class
 * 处理HTTP翻译请求，管理API密钥轮询和上下文记忆
 * Handles HTTP translation requests, manages API key rotation and context memory
 */
class TranslationServer : public QObject {
    Q_OBJECT
    
public:
    /**
     * 构造函数
     * Constructor
     * @param parent 父对象指针 / Parent object pointer
     */
    explicit TranslationServer(QObject *parent = nullptr);
    
    /**
     * 析构函数
     * Destructor
     */
    ~TranslationServer();
    
    /**
     * 更新服务器配置
     * Update server configuration
     * @param config 应用配置对象 / Application configuration object
     */
    void updateConfig(const AppConfig& config);
    
    /**
     * 启动翻译服务器
     * Start translation server
     */
    void startServer();
    
    /**
     * 停止翻译服务器
     * Stop translation server
     */
    void stopServer();
    
    /**
     * 清除所有客户端的上下文记忆
     * Clear context memory for all clients
     */
    void clearAllContexts();

signals:
    /**
     * 日志消息信号
     * Log message signal
     * @param msg 日志消息 / Log message
     */
    void logMessage(QString msg);
    
    /**
     * Token使用量信号
     * Token usage signal
     * @param prompt 输入token数量 / Input token count
     * @param completion 输出token数量 / Output token count
     */
    void tokenUsageReceived(int prompt, int completion);
    
    /**
     * 工作开始信号
     * Work started signal
     * 当服务器开始处理翻译请求时发出
     * Emitted when server starts processing translation request
     */
    void workStarted();
    
    /**
     * 工作完成信号
     * Work finished signal
     * @param success 是否成功完成 / Whether completed successfully
     */
    void workFinished(bool success);

private:
    /**
     * 服务器主循环
     * Server main loop
     * 处理HTTP服务器线程
     * Handles HTTP server thread
     */
    void runServerLoop();
    
    /**
     * 执行翻译
     * Perform translation
     * @param text 待翻译文本 / Text to be translated
     * @param clientIP 客户端IP地址 / Client IP address
     * @return 翻译结果 / Translation result
     */
    QString performTranslation(const QString& text, const QString& clientIP);
    
    /**
     * 获取下一个API密钥（轮询机制）
     * Get next API key (round-robin mechanism)
     * @return API密钥 / API key
     */
    QString getNextApiKey();
    
    /**
     * 生成客户端唯一标识符
     * Generate client unique identifier
     * @param ip 客户端IP地址 / Client IP address
     * @return 客户端ID / Client ID
     */
    QString generateClientId(const std::string& ip);
    
    /**
     * 执行单次翻译尝试
     * Perform single translation attempt
     * @param text 待翻译文本 / Text to be translated
     * @param clientIP 客户端IP地址 / Client IP address
     * @return 翻译结果 / Translation result
     */
    QString performSingleTranslationAttempt(const QString& text, const QString& clientIP);
    
    /**
     * 验证翻译结果是否有效
     * Validate if translation result is valid
     * @param result 翻译结果 / Translation result
     * @return 是否有效 / Whether result is valid
     */
    bool isValidTranslationResult(const QString& result);

    // 配置成员变量 / Configuration member variables
    AppConfig m_config; // 应用配置 / Application configuration
    std::atomic<bool> m_running; // 服务器运行状态 / Server running status
    
    // --- 新增：原子停止旗标，用于打破阻塞 ---
    // --- New: Atomic stop flag, used to break blocking ---
    std::atomic<bool> m_stopRequested; // 停止请求标志 / Stop request flag
    
    // 线程和服务器指针 / Thread and server pointers
    std::thread* m_serverThread = nullptr; // 服务器运行线程 / Server running thread
    httplib::Server* m_svr = nullptr; // HTTP服务器实例 / HTTP server instance
    
    // 上下文管理 / Context management
    std::map<std::string, Context> m_contexts; // 客户端ID到上下文的映射 / Map from client ID to context
    std::mutex m_contextMutex; // 上下文访问互斥锁 / Context access mutex
    
    // API密钥管理 / API key management
    std::vector<QString> m_apiKeys; // API密钥列表 / API key list
    int m_currentKeyIndex = 0; // 当前使用的密钥索引 / Current key index
    std::mutex m_keyMutex; // 密钥访问互斥锁 / Key access mutex

    // 🧊 冻结/解冻相关
    QMap<QString, QString> m_xmlTagsMap;  // 存储 XML 标签的映射表
    
    // 🧊 冻结/解冻方法
    QString freezeEscapes(const QString& input);
    QString thawEscapes(const QString& input);
    

};