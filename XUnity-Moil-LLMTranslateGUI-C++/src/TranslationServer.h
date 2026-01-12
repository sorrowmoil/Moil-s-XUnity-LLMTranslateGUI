#pragma once
#include <QObject>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <deque>
#include <mutex>
#include <map>
#include <atomic>
// #include <future>             // [Commented Out/已注释]
// #include <condition_variable> // [Commented Out/已注释]
#include "ConfigManager.h"
#include "httplib.h"

/**
 * @brief Conversation Context Structure
 * @brief 对话上下文结构体
 *
 * Stores the chat history for a specific client to enable continuous conversation.
 * 存储特定客户端的聊天记录，以实现连续对话（上下文记忆）功能。
 */
struct Context {
    // Stores pairs of (User Input, Assistant Response)
    // 存储 (用户输入, AI助手回复) 的成对历史
    std::deque<std::pair<QString, QString>> history;
    
    // Maximum number of context rounds to keep
    // 保留的最大上下文轮数
    int max_len;
};

/* [Commented Out] Pending Request Structure for Batch Processing
   [已注释] 用于批量处理的待处理请求结构
struct PendingRequest {
    QString originalText;
    QString clientIP;
    std::shared_ptr<std::promise<QString>> prom;
};
*/

/**
 * @brief Translation Server Logic Class
 * @brief 翻译服务端逻辑类
 *
 * Handles HTTP requests, manages API keys, maintains conversation contexts,
 * and forwards requests to the LLM API.
 * 负责处理本地 HTTP 请求，管理 API 密钥轮询，维护对话上下文，并将请求转发给远端 LLM API。
 */
class TranslationServer : public QObject {
    Q_OBJECT
public:
    explicit TranslationServer(QObject *parent = nullptr);
    ~TranslationServer();

    // Update runtime configuration (keys, prompts, etc.)
    // 更新运行时配置 (如 API 密钥, 提示词, 术语表设置等)
    void updateConfig(const AppConfig& config);

    // Start the HTTP listener thread
    // 启动 HTTP 监听线程 (使用 cpp-httplib)
    void startServer();

    // Stop the HTTP listener thread
    // 停止 HTTP 监听线程并清理资源
    void stopServer();

signals:
    // Signal to send logs to the UI main thread
    // 用于发送日志消息到 UI 主线程的信号 (跨线程通信)
    void logMessage(QString msg);

private:
    // Main loop for the httplib server (runs in a separate thread)
    // httplib 服务器的主循环 (在单独的 std::thread 中运行，不阻塞 Qt UI)
    void runServerLoop(); 
    
    // void runBatchProcessor(); // [Commented Out/已注释]

    // Core function: Sends request to AI API and parses response
    // 核心函数：构建提示词、发送请求给 AI API 并解析返回结果 (包含重试逻辑)
    QString performTranslation(const QString& text, const QString& clientIP);
    
    // void processBatch(std::vector<std::shared_ptr<PendingRequest>>& batch); // [Commented Out/已注释]

    // Get the next available API key (Round-Robin strategy)
    // 获取下一个可用的 API 密钥 (轮询策略，用于负载均衡)
    QString getNextApiKey();

    // Generate a simplified Client ID based on IP hash
    // 基于 IP 地址的哈希值生成简化的客户端 ID，用于区分不同用户的上下文
    QString generateClientId(const std::string& ip);

    AppConfig m_config;
    std::atomic<bool> m_running;            // Thread-safe running flag / 线程安全的运行标志
    std::thread* m_serverThread = nullptr;  // Pointer to the server thread / 指向 HTTP 服务器线程的指针
    // std::thread* m_batchThread = nullptr; // [Commented Out/已注释]
    
    httplib::Server* m_svr = nullptr;       // The actual HTTP server instance / 实际的 httplib 服务器实例

    /* [Commented Out] Batch processing queue
       [已注释] 批量处理队列
    std::deque<std::shared_ptr<PendingRequest>> m_requestQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    */

    // Context Storage: Map ClientID -> Context
    // 上下文存储：映射 ClientID -> 上下文结构体
    std::map<std::string, Context> m_contexts;
    
    // Mutex to protect context map from concurrent access
    // 互斥锁，保护 m_contexts 免受多线程并发访问冲突
    std::mutex m_contextMutex; 
    
    // API Key Management
    // API 密钥管理
    std::vector<QString> m_apiKeys;
    int m_currentKeyIndex = 0;
    std::mutex m_keyMutex;     // Protects key rotation / 保护密钥轮询索引

    // Error Retry Messages
    // 错误重试相关函数声明
    
    // Performs one attempt of translation without retry logic
    // 执行单次翻译尝试，不包含重试循环
    QString performSingleTranslationAttempt(const QString& text, const QString& clientIP);
    
    // Checks if the returned string is a valid translation (not an error message or empty)
    // 检查返回的字符串是否为有效的翻译结果（非错误信息或空）
    bool isValidTranslationResult(const QString& result);

};