#include "TranslationServer.h"
#include "json.hpp"
#include "GlossaryManager.h"
#include "RegexManager.h"
#include "LogManager.h"
#include "XuaConfigHijacker.h"
#include <QEventLoop>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include <regex>
#include <chrono>
#include <thread>
#include <algorithm>
#include <memory> // 🔥 引入现代C++智能指针

using json = nlohmann::json;

// ==========================================
// 日志与常量 (HTML Optimized)
// ==========================================
const char *SV_LOG_START[] = {
    "<font color='#4CAF50'><b>Server started.</b></font> Port: %1, Threads: %2",
    "<font color='#4CAF50'><b>服务已启动</b></font>，端口：%1，并发线程数：%2"};
const char *SV_LOG_STOP[] = {
    "<font color='#F44336'><b>Server stopped</b></font>",
    "<font color='#F44336'><b>服务已停止</b></font>"};
const char *SV_LOG_REQ_PREFIX[] = {
    "Request received: ",
    "收到请求: "};
const char *SV_ERR_KEY[] = {"Error: Invalid API Key", "错误：API 密钥无效"};
const char *SV_ERR_FMT[] = {"Error: Invalid Response Format", "错误：响应格式无效"};
const char *SV_ERR_JSON[] = {"Error: JSON Parse Error", "错误：JSON 解析失败"};
const char *SV_NEW_TERM[] = {
    "<font color='#FF9800'>✨ New Term Discovered: </font>",
    "<font color='#FF9800'>✨ 发现新术语: </font>"};
const char *SV_RETRY_ATTEMPT[] = {"🔄 Retry translation (%1/%2): ", "🔄 重试翻译 (%1/%2): "};
const char *SV_RETRY_SUCCESS[] = {"<font color='#4CAF50'>✅ Retry successful</font>", "<font color='#4CAF50'>✅ 重试成功</font>"};
const char *SV_RETRY_FAILED[] = {"<font color='#F44336'>❌ Retry failed, skipping text</font>", "<font color='#F44336'>❌ 重试失败，跳过文本</font>"};
const char *SV_ABORTED[] = {"⛔ Translation Aborted", "⛔ 翻译已终止"};

struct EscapeMap
{
    QMap<QString, QString> map;
    int counter = 0;
};

// 冻结保护
QString TranslationServer::freezeEscapesLocal(const QString &input, EscapeMap &context)
{
    QString result = input;
    context.map.clear();
    context.counter = 0;
    static const QRegularExpression regex(R"(\{\{.*?\}\}|<[^>]+>)");
    int lastEnd = 0;
    QString newResult;
    QRegularExpressionMatchIterator i = regex.globalMatch(result);
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        newResult.append(result.mid(lastEnd, match.capturedStart() - lastEnd));
        QString original = match.captured(0);
        QString tokenKey = QString("[T_%1]").arg(context.counter++);
        context.map[tokenKey] = original;
        newResult.append(tokenKey);
        lastEnd = match.capturedEnd();
    }
    newResult.append(result.mid(lastEnd));
    return newResult;
}

// 解冻还原
QString TranslationServer::thawEscapesLocal(const QString &input, const EscapeMap &context)
{
    QString result = input;
    static const QRegularExpression tokenRegex(R"(\s*[\[<【{]\s*T_(\d+)\s*[\]>】}]\s*)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator i = tokenRegex.globalMatch(result);
    QString newResult;
    int lastEnd = 0;
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        newResult.append(result.mid(lastEnd, match.capturedStart() - lastEnd));
        QString key = QString("[T_%1]").arg(match.captured(1));
        if (context.map.contains(key))
            newResult.append(context.map[key]);
        else
            newResult.append(match.captured(0));
        lastEnd = match.capturedEnd();
    }
    newResult.append(result.mid(lastEnd));
    return newResult;
}

// 🔥 Unity 富文本 -> Qt HTML 转换器 (强化兼容版)
QString TranslationServer::unityToHtml(const QString &text)
{
    QString t = text;

    // 1. 去除转义符
    t.replace(R"(\=)", "=");

    // 2. 🛡️ 保护安全可渲染的标签，转换为不会被逃逸的安全标记
    t.replace("[LF]", "[[[LF]]]");
    t.replace(QRegularExpression(R"(<br\s*/?>)", QRegularExpression::CaseInsensitiveOption), "[[[BR]]]");

    static const QRegularExpression colorStart(R"-(<color\s*=\s*"?([^>"]+?)"?>)-", QRegularExpression::CaseInsensitiveOption);
    t.replace(colorStart, "[[[C:\\1]]]");
    t.replace(QRegularExpression(R"(</color>)", QRegularExpression::CaseInsensitiveOption), "[[[/C]]]");

    t.replace(QRegularExpression(R"(<(b|i|u)>)", QRegularExpression::CaseInsensitiveOption), "[[[\\1]]]");
    t.replace(QRegularExpression(R"(</(b|i|u)>)", QRegularExpression::CaseInsensitiveOption), "[[[/\\1]]]");

    // 3. 💥 核心：拦截全部破坏性未知标签（如 <line-height>, <align>），彻底转义为普通文本，防止它们弄爆 Qt 的 HTML 渲染树
    t.replace("&", "&amp;");
    t.replace("<", "&lt;");
    t.replace(">", "&gt;");

    // 4. 🎨 安全还原：把保护好的标记恢复为真正的 HTML
    t.replace("[[[LF]]]", "<span style='color:#FF5722; font-weight:bold;'>[LF]</span><br>");
    t.replace("[[[BR]]]", "<span style='color:#FF5722; font-weight:bold;'>[BR]</span><br>");
    
    static const QRegularExpression colorStartRes(R"(\[\[\[C:(.*?)\]\]\])");
    t.replace(colorStartRes, R"(<span style="color:\1;">)");
    t.replace("[[[/C]]]", "</span>");

    t.replace(QRegularExpression(R"(\[\[\[(b|i|u)\]\]\])"), "<\\1>");
    t.replace(QRegularExpression(R"(\[\[\[/(b|i|u)\]\]\])"), "</\\1>");

    // 5. 将所有被转义过的其他破坏性标签直接物理隐藏 (保持日志区的绝对简洁)
    static const QRegularExpression remainingTags(R"(&lt;/?[a-zA-Z0-9_\-]+[^&]*&gt;)");
    t.replace(remainingTags, "");

    return t;
}

// 彩虹生成器预留实现
QString TranslationServer::makeRainbow(const QString &text) {
    return text;
}

// ==========================================
// 🚨 终极标签克隆手术 (完全解封并强化) 🚨
// ==========================================
QString TranslationServer::repairTranslationResult(const QString& original, const QString& translated) {
    QString result = translated;

    // 1. 统一处理：将 LLM 发明的方括号标签全部转化为尖括号 (例如 [b] -> <b>)
    result.replace(QRegularExpression(R"(\[(/?)(b|i|u|size|color)[^\]]*\])", QRegularExpression::CaseInsensitiveOption), "<\\1\\2>");

    // 2. 🚨 白名单独裁：物理消灭不存在的标签 🚨
    QStringList checkTags = {"b", "i", "u", "size", "color"};
    for (const QString& tag : checkTags) {
        if (!original.contains("<" + tag, Qt::CaseInsensitive)) {
            QRegularExpression killExp(R"(</?)" + tag + R"((?:>|\s[^>]*>|\\?=[^>]*>))", QRegularExpression::CaseInsensitiveOption);
            result.remove(killExp);
        }
    }
    
    // 移除模型脑补的 XML 格式占位符和自闭合垃圾
    result.remove(QRegularExpression(R"(</?T_\d+>)", QRegularExpression::CaseInsensitiveOption));
    result.remove(QRegularExpression(R"(<[^>]+/>)"));

    // ==========================================================
    // 3. 🚨 结构化逐行克隆手术 (Line-by-Line Clone Shell)
    // 专门针对 Unity 中频繁出现的用 <br> 或 /n 分割的名牌+对话句式修复
    // ==========================================================
    QRegularExpression newlineRegex(R"(\[LF\]|\\n|\r?\n|<br\s*/?>)", QRegularExpression::CaseInsensitiveOption); 
    QStringList orgLines = original.split(newlineRegex);
    QStringList transLines = result.split(newlineRegex);

    if (orgLines.size() == transLines.size() && orgLines.size() > 0) {
        QString finalResult;
        
        QRegularExpressionMatchIterator matchIt = newlineRegex.globalMatch(original);
        QStringList separators;
        while (matchIt.hasNext()) separators.append(matchIt.next().captured(0));

        for (int i = 0; i < orgLines.size(); ++i) {
            QString oLine = orgLines[i];
            QString tLine = transLines[i];

            // 完美捕捉并锁死：即使像 <color=#f4b3c2><u color=#c7005c> 这种复杂的复合标签
            QRegularExpression prefixExp(R"(^(?:<[a-zA-Z/][^>]*>|\s)+)");
            QRegularExpressionMatch pMatch = prefixExp.match(oLine);
            QString prefix = pMatch.hasMatch() ? pMatch.captured(0) : "";

            // 完美捕捉并锁死：同理保护 </u></color> 等结尾长关闭标签
            QRegularExpression suffixExp(R"((?:<[a-zA-Z/][^>]*>|\s)+$)");
            QRegularExpressionMatch sMatch = suffixExp.match(oLine);
            QString suffix = sMatch.hasMatch() ? sMatch.captured(0) : "";

            tLine.remove(QRegularExpression(R"(^(?:<[a-zA-Z/][^>]*>|\s)+)"));
            tLine.remove(QRegularExpression(R"((?:<[a-zA-Z/][^>]*>|\s)+$)"));

            if(tLine.isEmpty() && !oLine.isEmpty()) tLine = oLine;

            finalResult += prefix + tLine + suffix;

            if (i < separators.size()) {
                finalResult += separators[i];
            }
        }
        return finalResult;
    }

    // 4. 降级方案 (Fallback Armor)
    QRegularExpression globalPrefixExp(R"(^(?:<[a-zA-Z/][^>]*>|\s|\[LF\]|\\n|\r?\n|<br\s*/?>)+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch gpMatch = globalPrefixExp.match(original);
    QString gp = gpMatch.hasMatch() ? gpMatch.captured(0) : "";

    QRegularExpression globalSuffixExp(R"((?:<[a-zA-Z/][^>]*>|\s|\[LF\]|\\n|\r?\n|<br\s*/?>)+$)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch gsMatch = globalSuffixExp.match(original);
    QString gs = gsMatch.hasMatch() ? gsMatch.captured(0) : "";

    result.remove(QRegularExpression(R"(^(?:<[a-zA-Z/][^>]*>|\s|\[LF\]|\\n|\r?\n|<br\s*/?>)+)", QRegularExpression::CaseInsensitiveOption));
    result.remove(QRegularExpression(R"((?:<[a-zA-Z/][^>]*>|\s|\[LF\]|\\n|\r?\n|<br\s*/?>)+$)", QRegularExpression::CaseInsensitiveOption));

    return gp + result + gs;
}

// ==========================================
// TranslationServer Implementation
// ==========================================

TranslationServer::TranslationServer(QObject *parent) : QObject(parent), m_running(false)
{
    m_stopRequested = false;
    m_isStopping = false;
    m_svr = nullptr;
    m_serverThread = nullptr;
    m_cleanupThread = nullptr;

    connect(this, &TranslationServer::logMessage, [](const QString &msg)
            { LogManager::instance().addLog(msg); });
}

TranslationServer::~TranslationServer()
{
    stopServer();
    if (m_cleanupThread && m_cleanupThread->joinable())
    {
        m_cleanupThread->join();
        delete m_cleanupThread;
        m_cleanupThread = nullptr;
    }
}

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
        GlossaryManager::instance().setFilePath(m_config.glossary_path);
}

AppConfig TranslationServer::getConfig()
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config;
}

void TranslationServer::startServer()
{
    if (m_running || m_isStopping)
        return;

    if (m_cleanupThread && m_cleanupThread->joinable())
    {
        m_cleanupThread->join();
        delete m_cleanupThread;
        m_cleanupThread = nullptr;
    }

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

    if (m_config.enable_batch && !glossaryPath.isEmpty())
    {
        QString hijackedFile = XuaConfigHijacker::autoDetectAndHijack(glossaryPath, port, threads, m_config.handle_rich_text, m_config.extract_newline);
        if (!hijackedFile.isEmpty())
        {
            QString logMsg = (lang == 0) ? QString("🔗 <font color='#2196F3'>Batch Mode ON</font>: Game config injected (%1)").arg(hijackedFile)
                                         : QString("🔗 <font color='#2196F3'>打包模式已开启</font>：游戏配置已智能接管 (%1)").arg(hijackedFile);
            emit logMessage(logMsg);
        }
    }
    else
    {
        emit logMessage((lang == 0) ? "🛡️ Standard Mode: Config.ini untouched." : "🛡️ 标准模式：保持游戏原生配置不动。");
    }

    emit serverStarted();
}

void TranslationServer::stopServer()
{
    if (!m_running || m_isStopping)
        return;

    m_stopRequested = true;
    m_isStopping = true;

    if (m_cleanupThread && m_cleanupThread->joinable())
    {
        m_cleanupThread->join();
        delete m_cleanupThread;
        m_cleanupThread = nullptr;
    }

    m_cleanupThread = new std::thread([this]()
                                      {
        QElapsedTimer stopTimer;
        stopTimer.start();

        if (m_svr) {
            m_svr->stop(); // 此处配合异步泵，不会再卡死！
        }

        if (m_serverThread && m_serverThread->joinable()) {
            m_serverThread->join();
            delete m_serverThread;
            m_serverThread = nullptr;
        }

        delete m_svr;
        m_svr = nullptr;

        int lang = 1;
        int port = 6800;
        bool isDebug = false;
        QString glossaryPath = "";
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            lang = m_config.language;
            glossaryPath = m_config.glossary_path;
            port = m_config.port;
            isDebug = m_config.enable_debug_mode;
        }

        if (!glossaryPath.isEmpty()) {
            QString restoredFile = XuaConfigHijacker::autoDetectAndRestore(glossaryPath, port);
            if (!restoredFile.isEmpty()) {
                QString logMsg = (lang == 0) ? QString("✅ Config routing cleared: %1").arg(restoredFile)
                                             : QString("✅ 游戏配置路由已清除：%1").arg(restoredFile);
                emit logMessage(logMsg);
            }
        }

        qint64 elapsed = stopTimer.elapsed();
        if (isDebug) {
            QString logMsg = QString(SV_LOG_STOP[lang]) + QString(" <span style='color:#FF4500; font-size:medium;'>[⏱️ %1 ms]</span>").arg(elapsed);
            emit logMessage(logMsg);
        } else {
            emit logMessage(SV_LOG_STOP[lang]);
        }
        
        m_running = false;
        m_isStopping = false;
        emit serverStopped(); });
}

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

    // ==========================================
    // Custom Handler
    // ==========================================
    auto customHandler =   [this](const httplib::Request &req, httplib::Response &res)
    {
        if (m_stopRequested.load(std::memory_order_relaxed))
        {
            res.status = 503;
            res.set_content("Service Unavailable", "text/plain");
            return;
        }

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

        text.replace("\r\n", "[LF]");
        text.replace("\n", "[LF]");

        QString logHtml = unityToHtml(text);
        QString prefix = QString("<b style='color:#00B0FF'>[Custom]</b> ") + QString(SV_LOG_REQ_PREFIX[langIdx]);

        if (isDebug)
            emit logMessage(prefix + logHtml);
        else
            emit logMessage(QString(SV_LOG_REQ_PREFIX[langIdx]) + logHtml);

        emit workStarted();
        QElapsedTimer timer;
        timer.start();

        if (m_stopRequested.load(std::memory_order_relaxed))
        {
            emit workFinished(false);
            res.status = 500;
            res.set_content("Failed", "text/plain");
            return;
        }

        QString result = performTranslation(text, QString::fromStdString(req.remote_addr));
        
        // 🛑 如果处理期间点下了停止，阻止最终的输出！
        if (m_stopRequested.load(std::memory_order_relaxed))
        {
            emit workFinished(false);
            res.status = 500;
            res.set_content("Failed", "text/plain");
            return;
        }

        QString resultHtml = unityToHtml(result);
        result.replace("[LF]", "\n");

        qint64 elapsed = timer.elapsed();
        emit workFinished(!result.isEmpty() && !m_stopRequested);

        if (result.isEmpty())
        {
            res.status = 500;
            res.set_content("Failed", "text/plain");
        }
        else
        {
            if (isDebug)
                emit logMessage(QString("  -> %1 <span style='color:#FF4500; font-size:medium;'>[⏱️ %2 ms]</span>").arg(resultHtml).arg(elapsed));
            else
                emit logMessage(QString("  -> %1").arg(resultHtml));
            res.set_content(result.toStdString(), "text/plain; charset=utf-8");
        }
    };

    m_svr->Get("/", customHandler);
    m_svr->Post("/", customHandler);

    // ==========================================
    // Google Handler
    // ==========================================
    auto googleHandler =  [this](const httplib::Request &req, httplib::Response &res)
    {
        if (m_stopRequested.load(std::memory_order_relaxed))
        {
            res.status = 503;
            res.set_content("[]", "application/json");
            return;
        }

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

        QStringList allOrigLines = text.split('\n');
        std::vector<int> validIndices;
        QStringList linesToTranslate;

        for (int i = 0; i < allOrigLines.size(); ++i)
        {
            QString line = allOrigLines[i].trimmed();
            if (containsTranslatableContent(line))
            {
                validIndices.push_back(i);
                linesToTranslate.push_back(allOrigLines[i]);
            }
        }

        QString batchResultText;
        if (linesToTranslate.isEmpty() || m_stopRequested.load(std::memory_order_relaxed))
        {
            batchResultText = "";
        }
        else
        {
            QString cleanPayload = linesToTranslate.join('\n');
            batchResultText = performTranslation(cleanPayload, QString::fromStdString(req.remote_addr));
        }

        // 🛑 如果已请求停止服务，直接截断！防止批处理排队导致的 UI 日志狂乱输出（ANR）
        if (m_stopRequested.load(std::memory_order_relaxed))
        {
            emit workFinished(false);
            res.status = 500;
            res.set_content("[]", "application/json");
            return;
        }

        qint64 elapsed = timer.elapsed();
        emit workFinished(!batchResultText.isEmpty() && !m_stopRequested);

        QStringList translatedLines;
        if (!batchResultText.isEmpty())
        {
            translatedLines = batchResultText.split('\n');
        }

        QStringList finalOutputLines;
        int transIdx = 0;

        QString prefix = QString("<b style='color:#FF9800'>[Google]</b> ") + QString(SV_LOG_REQ_PREFIX[langIdx]);

        for (int i = 0; i < allOrigLines.size(); ++i)
        {
            bool isValid = false;
            for (int valIdx : validIndices)
            {
                if (valIdx == i)
                {
                    isValid = true;
                    break;
                }
            }

            QString origL = allOrigLines[i];
            QString finalL;

            if (!isValid)
            {
                finalL = origL;
            }
            else
            {
                if (transIdx < translatedLines.size())
                {
                    finalL = translatedLines[transIdx];
                    if (finalL.isEmpty())
                        finalL = origL;
                }
                else
                {
                    finalL = origL;
                }
                transIdx++;
            }

            QString origHtml = unityToHtml(origL);
            if (isDebug)
                emit logMessage(prefix + origHtml);
            else
                emit logMessage(QString(SV_LOG_REQ_PREFIX[langIdx]) + origHtml);

            QString finalHtml = unityToHtml(finalL);
            if (isDebug && i == allOrigLines.size() - 1)
            {
                QString timingStr = (langIdx == 0) ? QString(" <span style='color:#FF00FF; font-size:medium;'>[📦 Batch Total: %1 ms]</span>").arg(elapsed)
                                                   : QString(" <span style='color:#FF00FF; font-size:medium;'>[📦 包总耗时: %1 ms]</span>").arg(elapsed);
                emit logMessage("  -> " + finalHtml + timingStr);
            }
            else
            {
                emit logMessage("  -> " + finalHtml);
            }

            finalOutputLines.push_back(finalL);
        }

        json innerArray = json::array();
        for (int i = 0; i < allOrigLines.size(); ++i)
        {
            QString origL = allOrigLines[i];
            QString transL = (i < finalOutputLines.size()) ? finalOutputLines[i] : origL;
            innerArray.push_back(json::array({transL.toStdString(), origL.toStdString(), nullptr, nullptr, 1}));
        }
        res.set_content(json::array({innerArray, nullptr, "ja"}).dump(), "application/json; charset=utf-8");
    };

    m_svr->Get("/translate_a/single", googleHandler);
    m_svr->Post("/translate_a/single", googleHandler);

    m_svr->listen("0.0.0.0", port);
}

bool TranslationServer::containsTranslatableContent(const QString &text)
{
    static const QRegularExpression hasLetter(R"(\p{L})");
    return text.contains(hasLetter);
}

QString TranslationServer::performTranslation(const QString &text, const QString &clientIP)
{
    if (!containsTranslatableContent(text))
        return text;

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
            emit logMessage(QString(SV_RETRY_ATTEMPT[langIdx]).arg(retryCount + 1).arg(MAX_RETRY_COUNT));
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

bool TranslationServer::isValidTranslationResult(const QString &result)
{
    return !result.isEmpty() &&
           !result.startsWith("Error", Qt::CaseInsensitive) &&
           !result.contains("翻译失败", Qt::CaseInsensitive) &&
           !result.contains("translation failed", Qt::CaseInsensitive) &&
           result.length() > 0;
}

// 🔥 终极单次请求翻译尝试：完美结合碎片化标签重组与内存防泄漏机制
QString TranslationServer::performSingleTranslationAttempt(const QString &text, const QString &clientIP)
{
    if (m_stopRequested.load(std::memory_order_relaxed))
        return "";

    // ==========================================
    // 🛠️ 预处理：物理粉碎干扰 LLM 翻译的碎片化标签 (<rotate>, <voffset>)
    // 让 LLM 能够看到完整通顺的句子！
    // ==========================================
    QString preText = text;
    bool hasRotate = false;
    QString rotateOpenTag = "";
    
    QRegularExpression rotFinder(R"(<rotate\s*\\?=\s*[^>]+>|<rotate>)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch rotMatch = rotFinder.match(preText);
    if(rotMatch.hasMatch()) {
        hasRotate = true;
        rotateOpenTag = rotMatch.captured(0);
    }

    // 无情抹除这些把字拆散的罪魁祸首
    preText.remove(QRegularExpression(R"(</?rotate[^>]*>)", QRegularExpression::CaseInsensitiveOption));
    preText.remove(QRegularExpression(R"(</?voffset[^>]*>)", QRegularExpression::CaseInsensitiveOption));

    AppConfig cfg;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        cfg = m_config;
    }

    QString apiKey = getNextApiKey();
    if (apiKey.isEmpty())
    {
        emit logMessage("<font color='#F44336'>❌ " + QString(SV_ERR_KEY[cfg.language]) + "</font>");
        return "";
    }

    EscapeMap escapeCtx;
    // 使用纯净版文本进行标签冻结
    QString processedText = freezeEscapesLocal(preText, escapeCtx);
    if (cfg.enable_glossary)
        processedText = RegexManager::instance().processPre(processedText);
    std::string clientId = generateClientId(clientIP.toStdString()).toStdString();

    QString finalSystemPrompt = cfg.system_prompt;
    bool performExtraction = false;

    finalSystemPrompt += "\n\n【Translation Protocol (STRICT)】:\n"
                         "0. 🛡️ PRIORITY: TAGS/VARS/Z-CODES > GRAMMAR > STYLE. Never break code structures.\n"
                         "1. 📤 OUTPUT: Return ONLY the translated result. NO explanations. NO markdown.\n"
                         "2. 🧱 IMMUTABLES (KEEP EXACTLY):\n"
                         "   - [T_0], [T_1] ... : Placeholder tokens.\n"
                         "   - [LF] : Line break.\n"
                         "   - {{A}}, {{B}} ... : Variables. NEVER translate letters inside.\n"
                         "3. 📦 CONTAINERS (TRANSLATE CONTENT, KEEP WRAPPERS):\n"
                         "   - Z-Codes: 'Z[A-Z]{2}Z ... Z[A-Z]{2}Z'. Keep markers, translate inside.\n"
                         "   - HTML: '<tag>text</tag>'. Keep tags, translate 'text'.\n"
                         "4. 💬 PUNCTUATION & FORMAT:\n"
                         "   - Convert punctuation in visible text ONLY.\n"
                         "   - Do NOT modify punctuation inside tags.\n"
                         "   - Preserve spacing around tags.\n"
                         "5. 🧠 TRANSLATION LOGIC:\n"
                         "   - Treat input as independent UI fragments.\n"
                         "6. 🚫 ANTI-HALLUCINATION (CRITICAL):\n"
                         "   - DO NOT add <size>, <color>, <b>, <i> or brackets like [size=...] if they are not in the input.\n"
                         "   - DO NOT try to fix or close tags. Just keep exactly what you see.\n"
                         "   - DO NOT invent speaker names. DO NOT output </T_0>.\n"
                         "7. 🚨 FINAL SAFETY CHECK:\n"
                         "   - All tags closed\n"
                         "   - All {{X}} preserved\n"
                         "   - No new Z-codes created\n";

    if (cfg.enable_glossary)
    {
        QString glossaryContext = GlossaryManager::instance().getContextPrompt(processedText);
        if (!glossaryContext.isEmpty())
            finalSystemPrompt += "\n" + glossaryContext;
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

    // ==========================================
    // 🛠️ 特性 1：底层网络解耦 & 内存回收确认 (Modern C++ RAII)
    // ==========================================
    static thread_local std::unique_ptr<QNetworkAccessManager> threadNam;
    if (!threadNam)
        threadNam = std::make_unique<QNetworkAccessManager>();

    QNetworkRequest request(QUrl(cfg.api_address + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    std::unique_ptr<QNetworkReply> reply(threadNam->post(request, QByteArray::fromStdString(payload.dump())));

    // ==========================================
    // 🔥 异步轮询事件泵 (Async Polling Event Pump)
    // 彻底解决原生 std::thread 中导致 stopServer 卡死的问题！
    // ==========================================
    QEventLoop loop;
    QElapsedTimer timer;
    timer.start();
    bool isTimeout = false;

    while (!reply->isFinished())
    {
        if (m_stopRequested.load(std::memory_order_relaxed))
        {
            reply->abort();
            threadNam->clearAccessCache();
            threadNam->clearConnectionCache();
            break;
        }

        if (timer.elapsed() > 40000)
        {
            isTimeout = true;
            reply->abort();
            break;
        }

        loop.processEvents(QEventLoop::AllEvents, 50);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (m_stopRequested.load(std::memory_order_relaxed))
        return "";

    if (isTimeout)
    {
        emit logMessage("<font color='#F44336'>❌ Request Timeout</font>");
        return ""; 
    }

    // --- ⬇️ 解析流程 ⬇️ ---
    QString resultText = "";
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
                QString cleanContent = QString::fromStdString(content);

                static const QRegularExpression thinkTag(R"(<think(?:ing)?>.*?</think(?:ing)?>)", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
                static const QRegularExpression thinkTagShort(R"(</?think(?:ing)?>)", QRegularExpression::CaseInsensitiveOption);
                static const QRegularExpression boldTag(R"(^\*\*|\*\*$)");

                cleanContent.remove(thinkTag);
                cleanContent.remove(thinkTagShort);
                cleanContent.replace(boldTag, "");
                cleanContent = cleanContent.trimmed();

                if (performExtraction)
                {
                    static const QRegularExpression reTm("<tm>\\s*(.*?)\\s*=\\s*(.*?)\\s*</tm>", QRegularExpression::DotMatchesEverythingOption);
                    static const QRegularExpression tokenRegex(R"(\[T_\d+\])");
                    static const QRegularExpression lfRegex(R"(\[LF\])");
                    static const QRegularExpression termCodeRegex("Z[A-Z]{2}Z");

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
                        if (k.isEmpty() || v.isEmpty() || k.contains(tokenRegex) || v.contains(tokenRegex) || k.contains(lfRegex) || v.contains(lfRegex) || k.contains(termCodeRegex) || v.contains(termCodeRegex))
                            isValidTerm = false;

                        if (isValidTerm && processedText.contains(k, Qt::CaseInsensitive))
                        {
                            GlossaryManager::instance().addNewTerm(k, v);
                            emit logMessage(QString(SV_NEW_TERM[cfg.language]) + "<b>" + k + "</b> = <b>" + v + "</b>");
                        }
                        reconstructionBuffer.append(v);
                        lastPos = match.capturedEnd();
                    }
                    reconstructionBuffer.append(cleanContent.mid(lastPos));
                    cleanContent = reconstructionBuffer;
                }

                static const QRegularExpression reTl("<tl>(.*?)</tl>", QRegularExpression::DotMatchesEverythingOption);
                QRegularExpressionMatch matchTl = reTl.match(cleanContent);
                if (matchTl.hasMatch())
                    resultText = matchTl.captured(1).trimmed();
                else
                    resultText = cleanContent.trimmed();

                resultText.remove("<tl>", Qt::CaseInsensitive);
                resultText.remove("</tl>", Qt::CaseInsensitive);
                
                resultText = thawEscapesLocal(resultText, escapeCtx);
                if (cfg.enable_glossary)
                    resultText = RegexManager::instance().processPost(resultText);

                // 2. 🚨执行终极标签克隆手术🚨：必须使用预处理后的干净文本(preText)作比对！
                resultText = repairTranslationResult(preText, resultText);

                // 3. 保留 Z-Code 安全检查机制
                static const QRegularExpression zTagRegex("Z[A-Z]{2}Z");
                QSet<QString> sourceTags;
                QRegularExpressionMatchIterator j = zTagRegex.globalMatch(processedText);
                while (j.hasNext())
                    sourceTags.insert(j.next().captured());

                QString finalCleaned = resultText;
                QRegularExpressionMatchIterator k = zTagRegex.globalMatch(resultText);
                QSet<QString> outputTags;
                while (k.hasNext())
                    outputTags.insert(k.next().captured());

                for (const QString &tag : outputTags)
                {
                    if (!sourceTags.contains(tag))
                        finalCleaned.replace(tag, "");
                }
                resultText = finalCleaned.trimmed();

                // ==========================================
                // 4. 🔄 Unity 竖排渲染标签重建 (Rotate Reconstruction)
                // 专门为翻译后的文本，逐个真实字符套回原本的旋转标签！
                // ==========================================
                if (hasRotate && !resultText.isEmpty()) {
                    QString rewrapped;
                    // 精准匹配：忽略已存在的 HTML 标签、忽略空白和换行，只给实体字符穿戴！
                    QRegularExpression tokenMatcher(R"(<[^>]+>|\[LF\]|\r?\n|\s+|.)", QRegularExpression::DotMatchesEverythingOption);
                    QRegularExpressionMatchIterator rit = tokenMatcher.globalMatch(resultText);
                    while (rit.hasNext()) {
                        QString token = rit.next().captured(0);
                        if (token.startsWith("<") || token.startsWith("[LF]") || token.trimmed().isEmpty()) {
                            rewrapped += token; // 保持标签和空格原封不动
                        } else {
                            rewrapped += rotateOpenTag + token + "</rotate>"; // 重建竖排渲染！
                        }
                    }
                    resultText = rewrapped;
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
                emit logMessage("<font color='#F44336'>❌ " + QString(SV_ERR_FMT[cfg.language]) + "</font>");
                resultText = "";
            }
        }
        catch (...)
        {
            emit logMessage("<font color='#F44336'>❌ " + QString(SV_ERR_JSON[cfg.language]) + "</font>");
            resultText = "";
        }
    }
    else
    {
        emit logMessage("<font color='#F44336'>❌ Network Error: " + reply->errorString() + "</font>");
        resultText = "";
    }
    
    return resultText;
}

QString TranslationServer::getNextApiKey()
{
    std::lock_guard<std::mutex> lock(m_keyMutex);
    if (m_apiKeys.empty())
        return "";
    QString key = m_apiKeys[m_currentKeyIndex];
    m_currentKeyIndex = (m_currentKeyIndex + 1) % m_apiKeys.size();
    return key;
}

QString TranslationServer::generateClientId(const std::string &ip)
{
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(ip), QCryptographicHash::Md5);
    return hash.toHex().left(8);
}

void TranslationServer::clearAllContexts()
{
    std::lock_guard<std::mutex> lock(m_contextMutex);
    m_contexts.clear();
    int langIdx = 1;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        langIdx = m_config.language;
    }
    QString msg = (langIdx == 0) ? "<font color='#9C27B0'>🧹 Context memory cleared.</font>"
                                 : "<font color='#9C27B0'>🧹 上下文记忆已清空。</font>";
    emit logMessage(msg);
}