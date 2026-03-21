#include "TranslationServer.h"
#include "json.hpp"
#include "GlossaryManager.h"
#include "RegexManager.h"
#include "LogManager.h"
#include "XuaConfigHijacker.h" // Ensure this header exists / 确保此头文件存在
#include <QEventLoop>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QElapsedTimer> // Required for speed measurement / 测速需要
#include <regex>
#include <chrono>
#include <thread>
#include <algorithm>

using json = nlohmann::json;

// ==========================================
// Server log messages (multilingual)
// 服务器日志消息（多语言）
// ==========================================
const char *SV_LOG_START[] = {"Server started. Port: %1, Threads: %2", "服务已启动，端口：%1，并发线程数：%2"};
const char *SV_LOG_STOP[] = {"Server stopped", "服务已停止"};
const char *SV_LOG_REQ[] = {"Request received: ", "收到请求: "};
const char *SV_ERR_KEY[] = {"Error: Invalid API Key", "错误：API 密钥无效"};
const char *SV_ERR_FMT[] = {"Error: Invalid Response Format", "错误：响应格式无效"};
const char *SV_ERR_JSON[] = {"Error: JSON Parse Error", "错误：JSON 解析失败"};
const char *SV_ERR_NET[] = {"Error: Network Request Failed", "错误：网络请求失败"};
const char *SV_NEW_TERM[] = {"✨ New Term Discovered: ", "✨ 发现新术语: "};
const char *SV_RETRY_ATTEMPT[] = {"🔄 Retry translation (%1/%2): ", "🔄 重试翻译 (%1/%2): "};
const char *SV_RETRY_SUCCESS[] = {"✅ Retry successful", "✅ 重试成功"};
const char *SV_RETRY_FAILED[] = {"❌ Retry failed, skipping text", "❌ 重试失败，跳过文本"};
const char *SV_ABORTED[] = {"⛔ Translation Aborted", "⛔ 翻译已终止"};

/**
 * Structure to hold temporary escape mappings during freeze/thaw operations.
 * 在冻结/解冻操作期间保存临时转义映射的结构体。
 */
struct EscapeMap
{
    QMap<QString, QString> map; ///< Mapping from token (e.g., "[T_0]") to original escaped string.
                                 ///< 从令牌（例如"[T_0]"）到原始转义字符串的映射。
    int counter = 0;             ///< Counter for generating unique token numbers.
                                 ///< 用于生成唯一令牌编号的计数器。
};

// ==========================================
// Freeze method: protect HTML tags and variables, leave newlines untouched.
// 冻结方法：只保护 HTML 标签和变量，绝对放过换行符！
// ==========================================
QString TranslationServer::freezeEscapesLocal(const QString &input, EscapeMap &context)
{
    QString result = input;
    context.map.clear();
    context.counter = 0;

    // Regular expression to match HTML-like tags and {{variables}}.
    // 匹配 HTML 标签和 {{变量}} 的正则表达式。
    QRegularExpression regex(R"(\{\{.*?\}\}|<[^>]+>)");

    int lastEnd = 0;
    QString newResult;

    QRegularExpressionMatchIterator i = regex.globalMatch(result);
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        newResult.append(result.mid(lastEnd, match.capturedStart() - lastEnd));

        QString original = match.captured(0);
        // Generate a unique token, e.g., "[T_0]".
        // 生成唯一令牌，例如"[T_0]"。
        QString tokenKey = QString("[T_%1]").arg(context.counter++);

        context.map[tokenKey] = original;
        newResult.append(tokenKey);

        lastEnd = match.capturedEnd();
    }
    newResult.append(result.mid(lastEnd));
    return newResult;
}

// ==========================================
// Thaw method: tolerant regular expression to restore tags.
// 解冻方法：宽容的正则表达式，用于恢复标签。
// ==========================================
QString TranslationServer::thawEscapesLocal(const QString &input, const EscapeMap &context)
{
    QString result = input;

    // Regular expression that matches various possible token formats
    // (including extra spaces and different brackets).
    // 匹配各种可能的令牌格式（包括多余空格和不同括号）的正则表达式。
    QRegularExpression tokenRegex(R"(\s*[\[<【{]\s*T_(\d+)\s*[\]>】}]\s*)", QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator i = tokenRegex.globalMatch(result);
    QString newResult;
    int lastEnd = 0;

    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        newResult.append(result.mid(lastEnd, match.capturedStart() - lastEnd));

        QString key = QString("[T_%1]").arg(match.captured(1));
        if (context.map.contains(key))
        {
            newResult.append(context.map[key]); // Restore original tag / 恢复原始标签
        }
        else
        {
            newResult.append(match.captured(0)); // If not found, keep as is / 如果找不到，保留原样
        }
        lastEnd = match.capturedEnd();
    }
    newResult.append(result.mid(lastEnd));
    return newResult;
}

// ==========================================
// Implementation of TranslationServer
// TranslationServer 的实现
// ==========================================

/**
 * Constructor. Initializes the server and connects internal signals.
 * 构造函数。初始化服务器并连接内部信号。
 * 
 * @param parent Parent QObject.
 */
TranslationServer::TranslationServer(QObject *parent) : QObject(parent), m_running(false)
{
    m_stopRequested = false;
    m_svr = nullptr;
    m_serverThread = nullptr;

    // Forward log messages to the global LogManager.
    // 将日志消息转发到全局 LogManager。
    connect(this, &TranslationServer::logMessage, [](const QString &msg)
            { LogManager::instance().addLog(msg); });
}

/**
 * Destructor. Ensures the server is stopped.
 * 析构函数。确保服务器已停止。
 */
TranslationServer::~TranslationServer()
{
    stopServer();
}

/**
 * Update the server configuration from a new AppConfig.
 * 用新的 AppConfig 更新服务器配置。
 * 
 * @param config New configuration.
 */
void TranslationServer::updateConfig(const AppConfig &config)
{
    std::lock_guard<std::mutex> keyLock(m_keyMutex);
    std::lock_guard<std::mutex> cfgLock(m_configMutex);
    m_config = config;
    m_apiKeys.clear();
    QStringList keys = m_config.api_key.split(',', Qt::SkipEmptyParts);
    for (const auto &k : keys)
        m_apiKeys.push_back(k.trimmed());
    m_currentKeyIndex = 0;
    if (m_config.enable_glossary)
    {
        GlossaryManager::instance().setFilePath(m_config.glossary_path);
    }
}

/**
 * Get the current server configuration.
 * 获取当前服务器配置。
 * 
 * @return Copy of the current AppConfig.
 */
AppConfig TranslationServer::getConfig()
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config;
}

/**
 * Start the HTTP server in a background thread.
 * 在后台线程中启动 HTTP 服务器。
 */
void TranslationServer::startServer()
{
    if (m_running)
        return;
    m_running = true;
    m_stopRequested = false;

    m_serverThread = new std::thread(&TranslationServer::runServerLoop, this);

    int lang = 1;
    int port = 6800;
    int threads = 64;
    QString glossaryPath = "";

    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        lang = m_config.language;
        port = m_config.port;
        threads = std::clamp(m_config.max_threads, 64, 256);
        glossaryPath = m_config.glossary_path;
    }

    emit logMessage(QString(SV_LOG_START[lang]).arg(port).arg(threads));

    // Batch mode hijacking logic.
    // 打包模式接管逻辑。
    if (m_config.enable_batch && !glossaryPath.isEmpty())
    {
        QString hijackedFile = XuaConfigHijacker::autoDetectAndHijack(glossaryPath, port, threads);
        if (!hijackedFile.isEmpty())
        {
            QString logMsg = (lang == 0) ? QString("🔗 Batch Mode ON: Game config injected (%1)").arg(hijackedFile)
                                         : QString("🔗 打包模式已开启：游戏配置已智能接管 (%1)").arg(hijackedFile);
            emit logMessage(logMsg);
        }
    }
    else
    {
        QString logMsg = (lang == 0) ? "🛡️ Standard Mode: Config.ini untouched."
                                     : "🛡️ 标准模式：保持游戏原生配置不动。";
        emit logMessage(logMsg);
    }

    emit serverStarted();
}

/**
 * Stop the HTTP server and clean up.
 * 停止 HTTP 服务器并清理。
 */
void TranslationServer::stopServer()
{
    if (!m_running)
        return;

    m_stopRequested = true;
    m_running = false;

    if (m_svr)
        m_svr->stop();

    if (m_serverThread && m_serverThread->joinable())
    {
        m_serverThread->join();
        delete m_serverThread;
        m_serverThread = nullptr;
    }

    delete m_svr;
    m_svr = nullptr;

    int lang = 1;
    int port = 6800; // default value / 默认值
    QString glossaryPath = "";

    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        lang = m_config.language;
        glossaryPath = m_config.glossary_path;
        port = m_config.port; // Get the current port from config / 获取当前配置的端口
    }

    // Restore the hijacked config.
    // 恢复被接管的配置。
    if (!glossaryPath.isEmpty())
    {
        QString restoredFile = XuaConfigHijacker::autoDetectAndRestore(glossaryPath, port);
        if (!restoredFile.isEmpty())
        {
            QString logMsg = (lang == 0) ? QString("✅ Config routing cleared: %1").arg(restoredFile)
                                         : QString("✅ 游戏配置路由已清除：%1").arg(restoredFile);
            emit logMessage(logMsg);
        }
    }

    emit logMessage(SV_LOG_STOP[lang]);
    emit serverStopped();
}

/**
 * Main server loop (runs in a separate thread).
 * 主服务器循环（在单独线程中运行）。
 */
void TranslationServer::runServerLoop()
{
    m_svr = new httplib::Server();

    int threads = 64;
    int port = 6800;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        threads = std::clamp(m_config.max_threads, 64, 256);
        port = m_config.port;
    }

    m_svr->new_task_queue = [threads]
    { return new httplib::ThreadPool(threads); };

    // =========================================================
    // Route 1: Original Custom endpoint (with newline protection)
    // 路由 1：原本的 Custom 端点（自带换行符强力保护）
    // =========================================================
    auto customHandler = [this](const httplib::Request &req, httplib::Response &res)
    {
        if (!req.has_param("text"))
        {
            res.set_content("", "text/plain");
            return;
        }

        QString text = QString::fromStdString(req.get_param_value("text")).trimmed();
        if (text.isEmpty())
        {
            res.set_content("", "text/plain");
            return;
        }

        int langIdx = 1;
        bool isDebug = false;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            langIdx = m_config.language;
            isDebug = m_config.enable_debug_mode;
        }

        // Prepare log text (replace newlines for display).
        // 准备日志文本（替换换行符以便显示）。
        QString logText = text;
        logText.replace("\n", "[LF]");
        if (isDebug)
            emit logMessage(QString("[Custom] ") + QString(SV_LOG_REQ[langIdx]) + logText);
        else
            emit logMessage(QString(SV_LOG_REQ[langIdx]) + logText);

        emit workStarted();

        QElapsedTimer timer;
        timer.start();

        // Protect newlines by replacing them with a placeholder before sending to the LLM.
        // 在发送给大模型前，用占位符保护换行符。
        text.replace("\r\n", "[LF]");
        text.replace("\n", "[LF]");

        QString result = performTranslation(text, QString::fromStdString(req.remote_addr));

        // Restore newlines from the placeholder.
        // 从占位符恢复换行符。
        result.replace("[LF]", "\n");

        qint64 elapsed = timer.elapsed();
        emit workFinished(!result.isEmpty() && !m_stopRequested);

        bool isDebugFinal = false;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            isDebugFinal = m_config.enable_debug_mode;
        }

        if (result.isEmpty())
        {
            res.status = 500;
            res.set_content("Failed", "text/plain");
        }
        else
        {
            // For log display, replace newlines back to [LF] for readability.
            // 日志显示时，将换行符替换回 [LF] 以便阅读。
            QString displayResult = result;
            displayResult.replace("\n", "[LF]");

            if (isDebugFinal)
                emit logMessage(QString("  -> %1 [⏱️ %2 ms]").arg(displayResult).arg(elapsed));
            else
                emit logMessage(QString("  -> %1").arg(displayResult));

            res.set_content(result.toStdString(), "text/plain; charset=utf-8");
        }
    };

    m_svr->Get("/", customHandler);
    m_svr->Post("/", customHandler);

    // =========================================================
    // Route 2: Fake Google Translate API endpoint (for batch mode)
    // 路由 2：伪装 Google Translate API 端点（多行打包模式）
    // =========================================================
    auto googleHandler = [this](const httplib::Request &req, httplib::Response &res)
    {
        if (!req.has_param("q"))
        {
            res.set_content("[]", "application/json");
            return;
        }

        QString text = QString::fromStdString(req.get_param_value("q")).trimmed();
        if (text.isEmpty())
        {
            res.set_content("[]", "application/json");
            return;
        }

        int langIdx = 1;
        bool isDebug = false;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            langIdx = m_config.language;
            isDebug = m_config.enable_debug_mode;
        }

        emit workStarted();

        QElapsedTimer timer;
        timer.start();

        // Newlines here are line separators; we keep them as is.
        // 这里的换行符是行分隔符，我们原样保留。
        QString result = performTranslation(text, QString::fromStdString(req.remote_addr));

        qint64 elapsed = timer.elapsed();
        emit workFinished(!result.isEmpty() && !m_stopRequested);

        if (result.isEmpty())
        {
            res.status = 500;
            res.set_content("[]", "application/json");
            return;
        }

        bool isDebugFinal = false;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            isDebugFinal = m_config.enable_debug_mode;
        }

        QStringList origLines = text.split('\n');
        QStringList transLines = result.split('\n');

        for (int i = 0; i < origLines.size(); ++i)
        {
            QString origL = origLines[i];
            QString transL = (i < transLines.size()) ? transLines[i] : origL;

            if (transL.isEmpty())
                transL = "❌ [Missing]";

            if (isDebugFinal)
                emit logMessage(QString("[Google] ") + QString(SV_LOG_REQ[langIdx]) + origL);
            else
                emit logMessage(QString(SV_LOG_REQ[langIdx]) + origL);

            if (isDebugFinal && i == origLines.size() - 1)
            {
                emit logMessage(QString("  -> %1 [📦 包总耗时: %2 ms]").arg(transL).arg(elapsed));
            }
            else
            {
                emit logMessage("  -> " + transL);
            }
        }

        json innerArray = json::array();
        for (int i = 0; i < origLines.size(); ++i)
        {
            QString origL = origLines[i];
            QString transL = (i < transLines.size()) ? transLines[i] : origL;
            json item = json::array({transL.toStdString(), origL.toStdString(), nullptr, nullptr, 1});
            innerArray.push_back(item);
        }

        json responseArray = json::array({innerArray, nullptr, "ja"});
        res.set_content(responseArray.dump(), "application/json; charset=utf-8");
    };

    m_svr->Get("/translate_a/single", googleHandler);
    m_svr->Post("/translate_a/single", googleHandler);

    m_svr->listen("0.0.0.0", port);
}

/**
 * Perform translation with retry logic.
 * 执行带有重试逻辑的翻译。
 * 
 * @param text      Input text.
 * @param clientIP  Client IP address (for context separation).
 * @return Translated text, or empty string on failure.
 */
QString TranslationServer::performTranslation(const QString &text, const QString &clientIP)
{
    QString resultText = "";
    int retryCount = 0;
    const int MAX_RETRY_COUNT = 5;
    const int RETRY_DELAY_MS = 1000;
    int langIdx = 1;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        langIdx = m_config.language;
    }

    while (retryCount < MAX_RETRY_COUNT)
    {
        if (m_stopRequested)
        {
            emit logMessage(SV_ABORTED[langIdx]);
            return "";
        }
        if (retryCount > 0)
        {
            QString retryMsg = QString(SV_RETRY_ATTEMPT[langIdx]).arg(retryCount + 1).arg(MAX_RETRY_COUNT);
            emit logMessage(retryMsg);
            for (int i = 0; i < RETRY_DELAY_MS / 100; ++i)
            {
                if (m_stopRequested)
                    return "";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        QString attemptResult = performSingleTranslationAttempt(text, clientIP);
        if (m_stopRequested)
            return "";
        if (isValidTranslationResult(attemptResult))
        {
            if (retryCount > 0)
                emit logMessage(SV_RETRY_SUCCESS[langIdx]);
            resultText = attemptResult;
            break;
        }
        retryCount++;
        if (retryCount >= MAX_RETRY_COUNT)
        {
            emit logMessage(SV_RETRY_FAILED[langIdx]);
            resultText = "";
        }
    }
    return resultText;
}

/**
 * Check if a translation result is valid (non‑empty and not an error message).
 * 检查翻译结果是否有效（非空且不是错误消息）。
 * 
 * @param result The translation result.
 * @return True if valid.
 */
bool TranslationServer::isValidTranslationResult(const QString &result)
{
    return !result.isEmpty() &&
           !result.startsWith("Error", Qt::CaseInsensitive) &&
           !result.contains("翻译失败", Qt::CaseInsensitive) &&
           !result.contains("translation failed", Qt::CaseInsensitive) &&
           result.length() > 0;
}

/**
 * Perform a single translation attempt (no retry).
 * 执行单次翻译尝试（无重试）。
 * 
 * @param text      Input text.
 * @param clientIP  Client IP.
 * @return Translated text, or empty string on failure.
 */
QString TranslationServer::performSingleTranslationAttempt(const QString &text, const QString &clientIP)
{
    if (m_stopRequested)
        return "";

    AppConfig cfg;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        cfg = m_config;
    }

    QString apiKey = getNextApiKey();
    if (apiKey.isEmpty())
    {
        emit logMessage("❌ " + QString(SV_ERR_KEY[cfg.language]));
        return "";
    }

    EscapeMap escapeCtx;
    QString processedText = freezeEscapesLocal(text, escapeCtx);

    if (cfg.enable_glossary)
    {
        processedText = RegexManager::instance().processPre(processedText);
    }

    std::string clientId = generateClientId(clientIP.toStdString()).toStdString();

    QString finalSystemPrompt = cfg.system_prompt;
    bool performExtraction = false;

    finalSystemPrompt += "\n\n【Translation Rules (CRITICAL)】:\n"
                         "1. 🛑 PRESERVE TAGS: Keep tags like '[T_0]' EXACTLY as is.\n"
                         "2. 🛑 PRESERVE NEWLINES: Keep '[LF]' EXACTLY as is. It represents a line break in dialogs.\n"
                         "   - Input: \"A:[LF]Hello\"\n"
                         "   - Output: \"A:[LF]你好\"\n"
                         "3. 🔰 TERM CODES: Keep 'Z[A-Z]{2}Z' (e.g., 'ZMCZ') codes exactly as is, BUT TRANSLATE THE TEXT BETWEEN THEM!\n"
                         "   - Input: \"ZMCZtechniqueZMDZ\" -> Output: \"ZMCZ技术ZMDZ\"\n"
                         "4. 🛑 ANTI-BLEED / NO SENTENCE COMPLETION (Highest Priority):\n"
                         "   - You will receive fragmented UI texts. Treat EACH LINE as 100% INDEPENDENT.\n"
                         "   - DO NOT look at chat history to complete an incomplete sentence.\n"
                         "   - If input is a single word like \"CAMPAIGN\", output ONLY the noun \"活动\" or \"战役\". NEVER append context.\n"
                         "5. Output ONLY the translated result.\n";

    if (cfg.enable_glossary)
    {
        QString glossaryContext = GlossaryManager::instance().getContextPrompt(processedText);
        if (!glossaryContext.isEmpty())
        {
            finalSystemPrompt += "\n" + glossaryContext;
        }
        if (text.length() > 5)
        {
            performExtraction = true;
            finalSystemPrompt += "\n【Term Extraction】:\n"
                                 "1. Wrap translation in <tl>...</tl>.\n"
                                 "2. If you find Proper Nouns NOT in glossary, append <tm>Src=Trgt</tm> AFTER the translation.\n"
                                 "3. Keep <tm> tags OUTSIDE of <tl> tags.\n";
        }
    }

    json messages = json::array();
    messages.push_back({{"role", "system"}, {"content", finalSystemPrompt.toStdString()}});

    {
        std::lock_guard<std::mutex> lock(m_contextMutex);
        Context &ctx = m_contexts[clientId];
        if (ctx.max_len != cfg.context_num)
            ctx.max_len = cfg.context_num;
        for (const auto &pair : ctx.history)
        {
            messages.push_back({{"role", "user"}, {"content", pair.first.toStdString()}});
            messages.push_back({{"role", "assistant"}, {"content", pair.second.toStdString()}});
        }
    }

    QString currentUserContent = cfg.pre_prompt + processedText;
    messages.push_back({{"role", "user"}, {"content", currentUserContent.toStdString()}});

    json payload;
    payload["model"] = cfg.model_name.toStdString();
    payload["messages"] = messages;
    payload["temperature"] = cfg.temperature;

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(cfg.api_address + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    request.setTransferTimeout(45000);

    QNetworkReply *reply = manager.post(request, QByteArray::fromStdString(payload.dump()));

    QEventLoop loop;
    QTimer checkTimer;
    checkTimer.setInterval(100);

    QObject::connect(&checkTimer, &QTimer::timeout, [&]()
                     {
        if (m_stopRequested) { reply->abort(); loop.quit(); } });
    checkTimer.start();

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timeoutTimer.start(40000);
    loop.exec();

    QString resultText = "";

    if (m_stopRequested)
    {
        reply->deleteLater();
        return "";
    }

    if (!timeoutTimer.isActive())
    {
        emit logMessage("❌ Request Timeout");
        reply->abort();
        reply->deleteLater();
        return "";
    }
    timeoutTimer.stop();
    checkTimer.stop();

    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray responseBytes = reply->readAll();
        try
        {
            json response = json::parse(responseBytes.toStdString());

            if (response.contains("usage"))
            {
                int p = response["usage"].value("prompt_tokens", 0);
                int c = response["usage"].value("completion_tokens", 0);
                if (p > 0 || c > 0)
                    emit tokenUsageReceived(p, c);
            }

            if (response.contains("choices") && !response["choices"].empty())
            {
                std::string content = response["choices"][0]["message"]["content"];
                QString rawContent = QString::fromStdString(content);

                QString cleanContent = rawContent;
                // Clean up any thinking tags and unnecessary markdown.
                // 清理思考标签和不必要的 Markdown 符号。
                cleanContent.remove(QRegularExpression("<think(?:ing)?>.*?</think(?:ing)?>",
                                                       QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
                cleanContent.remove(QRegularExpression("</?think(?:ing)?>", QRegularExpression::CaseInsensitiveOption));
                cleanContent.replace(QRegularExpression("^\\*\\*|\\*\\*$"), "");
                cleanContent = cleanContent.trimmed();

                if (performExtraction)
                {
                    QRegularExpression reTm("<tm>\\s*(.*?)\\s*=\\s*(.*?)\\s*</tm>", QRegularExpression::DotMatchesEverythingOption);
                    QRegularExpression tokenRegex(R"(\[T_\d+\])");
                    QRegularExpression lfRegex(R"(\[LF\])");
                    QRegularExpression termCodeRegex("Z[A-Z]{2}Z");

                    QString reconstructionBuffer;
                    int lastPos = 0;

                    QRegularExpressionMatchIterator i = reTm.globalMatch(cleanContent);
                    while (i.hasNext())
                    {
                        QRegularExpressionMatch match = i.next();
                        QString k = match.captured(1).trimmed();
                        QString v = match.captured(2).trimmed();

                        reconstructionBuffer.append(cleanContent.mid(lastPos, match.capturedStart() - lastPos));

                        bool isValidTerm = true;
                        if (k.isEmpty() || v.isEmpty())
                            isValidTerm = false;
                        else if (k.contains(tokenRegex) || v.contains(tokenRegex))
                            isValidTerm = false;
                        else if (k.contains(lfRegex) || v.contains(lfRegex))
                            isValidTerm = false;
                        else if (k.contains(termCodeRegex) || v.contains(termCodeRegex))
                            isValidTerm = false;

                        if (isValidTerm && processedText.contains(k, Qt::CaseInsensitive))
                        {
                            GlossaryManager::instance().addNewTerm(k, v);
                            emit logMessage(QString(SV_NEW_TERM[cfg.language]) + k + " = " + v);
                        }
                        reconstructionBuffer.append(v);
                        lastPos = match.capturedEnd();
                    }
                    reconstructionBuffer.append(cleanContent.mid(lastPos));
                    cleanContent = reconstructionBuffer;
                }

                QRegularExpression reTl("<tl>(.*?)</tl>", QRegularExpression::DotMatchesEverythingOption);
                QRegularExpressionMatch matchTl = reTl.match(cleanContent);

                if (matchTl.hasMatch())
                {
                    resultText = matchTl.captured(1).trimmed();
                }
                else
                {
                    resultText = cleanContent.trimmed();
                }

                resultText.remove("<tl>", Qt::CaseInsensitive);
                resultText.remove("</tl>", Qt::CaseInsensitive);

                resultText = thawEscapesLocal(resultText, escapeCtx);

                if (cfg.enable_glossary)
                {
                    resultText = RegexManager::instance().processPost(resultText);
                }

                if (isValidTranslationResult(resultText))
                {
                    std::lock_guard<std::mutex> lock(m_contextMutex);
                    Context &ctx = m_contexts[clientId];
                    ctx.history.push_back({currentUserContent, resultText});
                    while (ctx.history.size() > ctx.max_len)
                        ctx.history.pop_front();
                }
                else
                {
                    resultText = "";
                }
            }
            else
            {
                emit logMessage("❌ " + QString(SV_ERR_FMT[cfg.language]));
                resultText = "";
            }
        }
        catch (...)
        {
            emit logMessage("❌ " + QString(SV_ERR_JSON[cfg.language]));
            resultText = "";
        }
    }
    else
    {
        emit logMessage("❌ Network Error: " + reply->errorString());
        resultText = "";
    }

    reply->deleteLater();
    return resultText;
}

/**
 * Get the next API key in round‑robin fashion.
 * 以轮询方式获取下一个 API 密钥。
 * 
 * @return Next API key, or empty string if none.
 */
QString TranslationServer::getNextApiKey()
{
    std::lock_guard<std::mutex> lock(m_keyMutex);
    if (m_apiKeys.empty())
        return "";
    QString key = m_apiKeys[m_currentKeyIndex];
    m_currentKeyIndex = (m_currentKeyIndex + 1) % m_apiKeys.size();
    return key;
}

/**
 * Generate a short client ID from an IP address (for context separation).
 * 从 IP 地址生成一个简短的客户端 ID（用于上下文隔离）。
 * 
 * @param ip Client IP as std::string.
 * @return First 8 characters of the MD5 hash of the IP.
 */
QString TranslationServer::generateClientId(const std::string &ip)
{
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(ip), QCryptographicHash::Md5);
    return hash.toHex().left(8);
}

/**
 * Clear all context histories for all clients.
 * 清除所有客户端的上下文历史。
 */
void TranslationServer::clearAllContexts()
{
    std::lock_guard<std::mutex> lock(m_contextMutex);
    m_contexts.clear();

    int langIdx = 1;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        langIdx = m_config.language;
    }
    QString msg = (langIdx == 0) ? "🧹 Context memory cleared." : "🧹 上下文记忆已清空。";
    LOG(msg);
}