#pragma once

#include <QObject>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <deque>
#include <mutex>
#include <map>
#include <atomic> 
#include <thread> 
#include "ConfigManager.h"
#include "httplib.h"

struct Context {
    std::deque<std::pair<QString, QString>> history; 
    int max_len; 
};

class TranslationServer : public QObject {
    Q_OBJECT
    
public:
    void injectLog(const QString& msg) {
        emit logMessage(msg);
    }

    explicit TranslationServer(QObject *parent = nullptr);
    ~TranslationServer();
    
    void updateConfig(const AppConfig& config);
    AppConfig getConfig(); 
    
    void startServer();
    void stopServer();
    
    void clearAllContexts();
    bool isRunning() const { return m_running; }

signals:
    void logMessage(QString msg);
    void tokenUsageReceived(int prompt, int completion);
    void workStarted();
    void workFinished(bool success);
    void serverStarted();
    void serverStopped();

private:
    void runServerLoop();
    QString performTranslation(const QString& text, const QString& clientIP);
    QString getNextApiKey();
    QString generateClientId(const std::string& ip);

    QString performSingleTranslationAttempt(const QString& text, const QString& clientIP);
    bool isValidTranslationResult(const QString& result);
    QString freezeEscapesLocal(const QString& input, struct EscapeMap& context); 
    QString thawEscapesLocal(const QString& input, const struct EscapeMap& context);
    
    // 🔥 核心外科手术：修复标签丢失与幻觉
    QString repairTranslationResult(const QString& original, const QString& translated);

    // 纯符号/数字过滤器
    bool containsTranslatableContent(const QString& text);
    
    // Unity富文本转Qt HTML
    QString unityToHtml(const QString& text);
    
    // 彩虹文字生成器 (预留)
    QString makeRainbow(const QString& text);

private:
    AppConfig m_config; 
    std::atomic<bool> m_running; 
    std::atomic<bool> m_stopRequested; 
    std::atomic<bool> m_isStopping;

    std::thread* m_serverThread = nullptr; 
    std::thread* m_cleanupThread = nullptr;

    httplib::Server* m_svr = nullptr; 
    
    std::map<std::string, Context> m_contexts; 
    std::mutex m_contextMutex; 
    
    std::vector<QString> m_apiKeys; 
    int m_currentKeyIndex = 0; 
    std::mutex m_keyMutex; 
    
    std::mutex m_configMutex;
};