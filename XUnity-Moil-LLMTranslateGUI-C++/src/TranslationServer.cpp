#include "TranslationServer.h"
#include "json.hpp"
#include "GlossaryManager.h" 
#include "RegexManager.h"
#include <QEventLoop>
#include <QCryptographicHash>
#include <QRegularExpression> 
#include <QRandomGenerator>
#include <regex>              
#include <chrono>
#include <QTimer> 

using json = nlohmann::json;

// ==========================================
// 📝 Server Log Dictionary / 服务器日志字典
// ==========================================

// 服务器启动日志 / Server start log
const char* SV_LOG_START[] = { "Server started. Port: %1, Threads: %2", "服务已启动，端口：%1，并发线程数：%2" };
// 服务器停止日志 / Server stop log
const char* SV_LOG_STOP[] = { "Server stopped", "服务已停止" };
// 请求接收日志 / Request received log
const char* SV_LOG_REQ[] = { "Request received: ", "收到请求: " };
// API密钥错误 / API key error
const char* SV_ERR_KEY[] = { "Error: Invalid API Key", "错误：API 密钥无效" };
// 响应格式错误 / Response format error
const char* SV_ERR_FMT[] = { "Error: Invalid Response Format", "错误：响应格式无效" };
// JSON解析错误 / JSON parse error
const char* SV_ERR_JSON[] = { "Error: JSON Parse Error", "错误：JSON 解析失败" };
// 新术语发现日志 / New term discovered log
const char* SV_NEW_TERM[] = { "✨ New Term Discovered: ", "✨ 发现新术语: " };
// 重试尝试日志 / Retry attempt log
const char* SV_RETRY_ATTEMPT[] = { "🔄 Retry translation (%1/%2): ", "🔄 重试翻译 (%1/%2): " };
// 重试成功日志 / Retry success log
const char* SV_RETRY_SUCCESS[] = { "✅ Retry successful", "✅ 重试成功" };
// 重试失败日志 / Retry failed log
const char* SV_RETRY_FAILED[] = { "❌ Retry failed, skipping text", "❌ 重试失败，跳过文本" };
// 翻译终止日志 / Translation aborted log
const char* SV_ABORTED[] = { "⛔ Translation Aborted", "⛔ 翻译已终止" };

// <实验性> 转义映射结构体，用于保护特殊标签
// <Experimental> Escape mapping struct for protecting special tags
struct EscapeMap {
    QMap<QString, QString> map; // 占位符到原始内容的映射 / Placeholder to original content mapping
    int counter = 0; // 计数器用于生成唯一占位符 / Counter for generating unique placeholders
};

// ==========================================
// 🧊 冻结/解冻方法实现 (作为类成员函数)
// 🧊 Freeze/Thaw method implementation (as class member functions)
// ==========================================

/**
 * 冻结转义符 - 保护特殊标签不被LLM处理
 * Freeze escapes - protect special tags from being processed by LLM
 * @param input 输入文本 / Input text
 * @param context 转义映射上下文 / Escape mapping context
 * @return 处理后的文本 / Processed text
 */
QString TranslationServer::freezeEscapesLocal(const QString& input, EscapeMap& context) {
    QString result = input;
    context.map.clear(); // 清空映射 / Clear mapping
    context.counter = 0; // 重置计数器 / Reset counter
    
    // 正则表达式匹配需要保护的标签 / Regex to match tags that need protection
    // {{...}}, <...>, 以及常见的转义符 / {{...}}, <...>, and common escape characters
    QRegularExpression regex(R"(\{\{.*?\}\}|<[^>]+>|\\r\\n|\\n|\\r|\\t|\r\n|\n|\r|\t)");
    
    int offset = 0;
    QRegularExpressionMatchIterator i = regex.globalMatch(result);
    
    QString newResult; // 处理结果缓冲区 / Processed result buffer
    int lastEnd = 0; // 上一个匹配结束位置 / Last match end position
    
    // 遍历所有匹配项 / Iterate through all matches
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        
        // 1. 追加匹配项之前的内容 / Append content before the match
        newResult.append(result.mid(lastEnd, match.capturedStart() - lastEnd));
        
        // 2. 生成带空格的占位符 [T_x] / Generate placeholder with spaces [T_x]
        QString original = match.captured(0); // 原始匹配内容 / Original matched content
        QString tokenKey = QString("[T_%1]").arg(context.counter++); // 生成唯一占位符 / Generate unique placeholder
        QString tokenWithSpace = QString(" %1 ").arg(tokenKey); // 前后加空格防止被LLM吞噬 / Add spaces to prevent being consumed by LLM
        
        context.map[tokenKey] = original; // 保存映射关系 / Save mapping relationship
        
        newResult.append(tokenWithSpace); // 追加占位符 / Append placeholder
        
        lastEnd = match.capturedEnd(); // 更新结束位置 / Update end position
    }
    
    // 3. 追加剩余内容 / Append remaining content
    newResult.append(result.mid(lastEnd));
    
    return newResult;
}

/**
 * 解冻转义符 - 将占位符恢复为原始内容
 * Thaw escapes - restore placeholders to original content
 * @param input 输入文本 / Input text
 * @param context 转义映射上下文 / Escape mapping context
 * @return 恢复后的文本 / Restored text
 */
QString TranslationServer::thawEscapesLocal(const QString& input, const EscapeMap& context) {
    QString result = input;
    
    // 正则匹配 [T_数字] 及其周围可能存在的空白字符
    // Regex matches [T_number] and possible surrounding whitespace characters
    QRegularExpression tokenRegex(R"(\s*\[T_(\d+)\]\s*)");
    
    QRegularExpressionMatchIterator i = tokenRegex.globalMatch(result);
    
    QString newResult; // 恢复结果缓冲区 / Restored result buffer
    int lastEnd = 0; // 上一个匹配结束位置 / Last match end position
    
    // 遍历所有占位符 / Iterate through all placeholders
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        
        // 追加前文 / Append preceding text
        newResult.append(result.mid(lastEnd, match.capturedStart() - lastEnd));
        
        // 获取Key / Get Key
        QString key = QString("[T_%1]").arg(match.captured(1));
        
        // 还原内容 / Restore content
        if (context.map.contains(key)) {
            newResult.append(context.map[key]); // 恢复原始内容 / Restore original content
        } else {
            // 如果找不到（极少情况），就保留 Key 原样（但去掉多余空格）
            // If not found (rare case), keep the Key as is (but remove extra spaces)
            newResult.append(key);
        }
        
        lastEnd = match.capturedEnd(); // 更新结束位置 / Update end position
    }
    
    // 追加剩余文本 / Append remaining text
    newResult.append(result.mid(lastEnd));
    
    return newResult;
}

// ==========================================
// 🚀 TranslationServer 实现
// 🚀 TranslationServer Implementation
// ==========================================

/**
 * 构造函数 / Constructor
 * @param parent 父对象 / Parent object
 */
TranslationServer::TranslationServer(QObject *parent) : QObject(parent), m_running(false) {
    m_stopRequested = false; // 初始化停止请求标志 / Initialize stop request flag
    m_svr = nullptr; // 初始化HTTP服务器指针 / Initialize HTTP server pointer
    m_serverThread = nullptr; // 初始化服务器线程指针 / Initialize server thread pointer
}

/**
 * 析构函数 / Destructor
 */
TranslationServer::~TranslationServer() {
    stopServer(); // 确保服务器停止 / Ensure server stops
}

/**
 * 更新配置 / Update configuration
 * @param config 新的配置 / New configuration
 */
void TranslationServer::updateConfig(const AppConfig& config) {
    // 🔥 同时锁定 KeyMutex 和 ConfigMutex / Lock both KeyMutex and ConfigMutex
    std::lock_guard<std::mutex> keyLock(m_keyMutex); 
    std::lock_guard<std::mutex> cfgLock(m_configMutex); 
    
    m_config = config; // 更新配置 / Update configuration
    
    // 解析API密钥（支持逗号分隔的多个密钥） / Parse API keys (supports multiple comma-separated keys)
    m_apiKeys.clear(); // 清空现有密钥列表 / Clear existing key list
    QStringList keys = m_config.api_key.split(',', Qt::SkipEmptyParts); // 分割密钥字符串 / Split key string
    for(const auto& k : keys) m_apiKeys.push_back(k.trimmed()); // 添加清理后的密钥 / Add cleaned keys
    m_currentKeyIndex = 0; // 重置密钥索引 / Reset key index
    
    // 更新术语表管理器 / Update glossary manager
    if (m_config.enable_glossary) {
        GlossaryManager::instance().setFilePath(m_config.glossary_path);
    }
}

/**
 * 🔥 获取当前配置副本（线程安全） / Get current configuration copy (thread-safe)
 * @return 当前配置 / Current configuration
 */
AppConfig TranslationServer::getConfig() {
    std::lock_guard<std::mutex> lock(m_configMutex); // 加锁保证线程安全 / Lock for thread safety
    return m_config; // 返回配置副本 / Return configuration copy
}

/**
 * 启动服务器 / Start server
 */
void TranslationServer::startServer() {
    if (m_running) return; // 如果已在运行则返回 / Return if already running
    m_running = true; // 设置运行标志 / Set running flag
    m_stopRequested = false; // 重置停止请求标志 / Reset stop request flag
    
    // 创建服务器线程 / Create server thread
    m_serverThread = new std::thread(&TranslationServer::runServerLoop, this);
    
    // 获取配置信息用于日志 / Get configuration for logging
    int lang = 1; // 语言 / Language
    int port = 6800; // 端口 / Port
    int threads = 1; // 线程数 / Thread count
    {
         std::lock_guard<std::mutex> lock(m_configMutex); // 加锁读取配置 / Lock to read configuration
         lang = m_config.language;
         port = m_config.port;
         threads = m_config.max_threads;
    }
    
    // 发送启动日志 / Send start log
    emit logMessage(QString(SV_LOG_START[lang]).arg(port).arg(threads));
}

/**
 * 停止服务器 / Stop server
 */
void TranslationServer::stopServer() {
    if (!m_running) return; // 如果未运行则返回 / Return if not running
    
    m_stopRequested = true; // 设置停止请求标志 / Set stop request flag
    m_running = false; // 清除运行标志 / Clear running flag
    
    if (m_svr) m_svr->stop(); // 停止HTTP服务器 / Stop HTTP server
    
    // 等待服务器线程结束 / Wait for server thread to finish
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
        delete m_serverThread;
        m_serverThread = nullptr;
    }
    
    delete m_svr; // 删除HTTP服务器实例 / Delete HTTP server instance
    m_svr = nullptr;
    
    // 获取语言设置用于日志 / Get language setting for logging
    int lang = 1;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        lang = m_config.language;
    }
    
    // 发送停止日志 / Send stop log
    emit logMessage(SV_LOG_STOP[lang]);
}

/**
 * 服务器主循环 / Server main loop
 */
void TranslationServer::runServerLoop() {
    m_svr = new httplib::Server(); // 创建HTTP服务器实例 / Create HTTP server instance
    
    // 获取线程数配置 / Get thread count configuration
    int threads = 1;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        threads = m_config.max_threads;
    }
    if (threads < 1) threads = 1; // 确保至少一个线程 / Ensure at least one thread
    
    // 设置线程池 / Set thread pool
    m_svr->new_task_queue = [threads] { return new httplib::ThreadPool(threads); };

    // 设置GET请求处理 / Set GET request handler
    m_svr->Get("/",  [this](const httplib::Request& req, httplib::Response& res) {
        // 检查是否有text参数 / Check if text parameter exists
        if (!req.has_param("text")) { 
            res.set_content("", "text/plain"); 
            return; 
        }
        
        // 获取并清理文本参数 / Get and clean text parameter
        std::string text_std = req.get_param_value("text");
        QString text = QString::fromStdString(text_std).trimmed();
        
        // 检查文本是否为空 / Check if text is empty
        if (text.isEmpty()) { 
            res.set_content("", "text/plain; charset=utf-8"); 
            return; 
        }

        // 🔥 获取语言设置需要加锁 / Get language setting with lock
        int langIdx = 1;
        {
             std::lock_guard<std::mutex> lock(m_configMutex);
             langIdx = m_config.language;
        }

        // 记录请求日志 / Log request
        QString logText = text;
        logText.replace("\n", "[LF]"); // 替换换行符以便显示 / Replace newlines for display
        emit logMessage(QString(SV_LOG_REQ[langIdx]) + logText);
        
        emit workStarted(); // 发送工作开始信号 / Emit work started signal

        // 执行翻译 / Perform translation
        QString result = performTranslation(text, QString::fromStdString(req.remote_addr));
        
        // 发送工作完成信号 / Emit work finished signal
        if (!m_stopRequested) {
            bool success = !result.isEmpty();
            emit workFinished(success); 
        } else {
            emit workFinished(false); 
        }

        // 设置响应 / Set response
        if (result.isEmpty()) {
            res.status = 500; // 服务器错误 / Server error
            res.set_content("Translation Failed", "text/plain"); 
        } else {
            res.set_content(result.toStdString(), "text/plain; charset=utf-8");
        }
    });
    
    // 获取端口配置 / Get port configuration
    int port = 6800;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        port = m_config.port;
    }
    
    // 启动服务器监听 / Start server listening
    m_svr->listen("0.0.0.0", port);
}

/**
 * 执行翻译（包含重试机制） / Perform translation (with retry mechanism)
 * @param text 要翻译的文本 / Text to translate
 * @param clientIP 客户端IP地址 / Client IP address
 * @return 翻译结果 / Translation result
 */
QString TranslationServer::performTranslation(const QString& text, const QString& clientIP) {
    QString resultText = ""; // 结果文本 / Result text
    int retryCount = 0; // 重试次数 / Retry count
    const int MAX_RETRY_COUNT = 5; // 最大重试次数 / Maximum retry count
    const int RETRY_DELAY_MS = 1000; // 重试延迟（毫秒） / Retry delay (ms)

    // 获取当前语言配置用于日志 / Get current language configuration for logging
    int langIdx = 1;
    {
         std::lock_guard<std::mutex> lock(m_configMutex);
         langIdx = m_config.language;
    }
    
    // 重试循环 / Retry loop
    while (retryCount < MAX_RETRY_COUNT) {
        // 检查是否被请求停止 / Check if stop requested
        if (m_stopRequested) {
            emit logMessage(SV_ABORTED[langIdx]); // 发送终止日志 / Send abort log
            return "";
        }

        // 如果是重试，记录日志并延迟 / If retry, log and delay
        if (retryCount > 0) {
            QString retryMsg = QString(SV_RETRY_ATTEMPT[langIdx])
                                  .arg(retryCount + 1)
                                  .arg(MAX_RETRY_COUNT);
            emit logMessage(retryMsg);
            
            // 延迟等待 / Delay wait
            for (int i = 0; i < RETRY_DELAY_MS / 100; ++i) {
                if (m_stopRequested) return "";
                QThread::msleep(100);
            }
        }
        
        // 调用单次翻译尝试 / Call single translation attempt
        QString attemptResult = performSingleTranslationAttempt(text, clientIP); 
        
        if (m_stopRequested) return ""; // 再次检查停止请求 / Check stop request again

        // 验证结果 / Validate result
        if (isValidTranslationResult(attemptResult)) {
            if (retryCount > 0) emit logMessage(SV_RETRY_SUCCESS[langIdx]); // 重试成功日志 / Retry success log
            resultText = attemptResult;
            break; // 成功，退出循环 / Success, exit loop
        }
        
        retryCount++; // 增加重试计数 / Increment retry count
        
        // 达到最大重试次数 / Reached maximum retry count
        if (retryCount >= MAX_RETRY_COUNT) {
            emit logMessage(SV_RETRY_FAILED[langIdx]); // 重试失败日志 / Retry failed log
            resultText = ""; // 返回空结果 / Return empty result
        }
    }
    return resultText;
}

/**
 * 验证翻译结果有效性 / Validate translation result validity
 * @param result 翻译结果 / Translation result
 * @return 是否有效 / Whether valid
 */
bool TranslationServer::isValidTranslationResult(const QString& result) {
    return !result.isEmpty() && // 非空 / Not empty
           !result.startsWith("Error", Qt::CaseInsensitive) && // 不以Error开头 / Not start with "Error"
           !result.contains("翻译失败", Qt::CaseInsensitive) && // 不包含"翻译失败" / Not contain "翻译失败"
           !result.contains("translation failed", Qt::CaseInsensitive) && // 不包含"translation failed" / Not contain "translation failed"
           result.length() > 0; // 长度大于0 / Length greater than 0
}

/**
 * 执行单次翻译尝试 / Perform single translation attempt
 * @param text 要翻译的文本 / Text to translate
 * @param clientIP 客户端IP地址 / Client IP address
 * @return 翻译结果 / Translation result
 */
QString TranslationServer::performSingleTranslationAttempt(const QString& text, const QString& clientIP) {
    if (m_stopRequested) return ""; // 检查停止请求 / Check stop request

    // 🔥 获取本次尝试的配置快照 (热重载核心) / Get configuration snapshot for this attempt (hot reload core)
    // 每次尝试时都重新读取 m_config，这样如果在重试期间用户点击了"Reload"，下一次重试就会使用新配置
    // Each attempt re-reads m_config, so if user clicks "Reload" during retry, next retry will use new configuration
    AppConfig cfg;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        cfg = m_config;
    }

    // 获取API密钥 / Get API key
    QString apiKey = getNextApiKey();
    if (apiKey.isEmpty()) {
        emit logMessage("❌ " + QString(SV_ERR_KEY[cfg.language])); // API密钥错误日志 / API key error log
        return "";
    }

    // ========== 第1步：局部冻结 ========== / Step 1: Local freeze
    EscapeMap escapeCtx;
    // 使用成员函数调用冻结转义符 / Use member function to freeze escapes
    QString processedText = freezeEscapesLocal(text, escapeCtx);
    
    // 如果启用术语表，进行预处理 / If glossary enabled, preprocess
    if (cfg.enable_glossary) {
         processedText = RegexManager::instance().processPre(processedText);
    }

    // 生成客户端ID / Generate client ID
    std::string clientId = generateClientId(clientIP.toStdString()).toStdString();
    
    // 构建系统提示词 / Build system prompt
    QString finalSystemPrompt = cfg.system_prompt;
    bool performExtraction = false; // 是否执行术语提取 / Whether to perform term extraction

   // 添加翻译规则 / Add translation rules
   finalSystemPrompt += "\n\n【Translation Rules】:\n"
                     "1. 🛑 PRESERVE TAGS: You will see tags like '[T_0]', '[T_1]'.\n"
                     "   - These replace newlines or code. Keep them EXACTLY as is.\n"
                     "   - Input: \"Hello [T_0] World\"\n"
                     "   - Output: \"你好 [T_0] 世界\"\n"
                     "2. 🛑 NO CLEANUP: Do NOT remove the tags.\n"
                     "3. 🔰 TERM CODES: Keep 'Z[A-Z]{2}Z' (e.g., 'ZMCZ') codes exactly as is.\n"
                     "4. Translate the text BETWEEN the tags naturally.\n"
                     "5. Output ONLY the translated result.\n";
                     
    // 如果启用术语表，添加上下文 / If glossary enabled, add context
    if (cfg.enable_glossary) {
        QString glossaryContext = GlossaryManager::instance().getContextPrompt(processedText);
        if (!glossaryContext.isEmpty()) {
            finalSystemPrompt += "\n" + glossaryContext;
        }

        // 如果文本较长，执行术语提取 / If text is long, perform term extraction
        if (text.length() > 5) { 
            performExtraction = true;
            finalSystemPrompt += "\n【Term Extraction】:\n"
                                 "1. Wrap translation in <tl>...</tl>.\n"
                                 "2. If you find Proper Nouns (Names) NOT in glossary, append <tm>Src=Trgt</tm> AFTER the translation.\n" // 强调追加在后面 / Emphasize appending after
                                 "3. Keep <tm> tags OUTSIDE of <tl> tags.\n"; // 强调不要嵌套 / Emphasize no nesting
        }
    }

    // 构建消息数组 / Build messages array
    json messages = json::array();
    messages.push_back({{"role", "system"}, {"content", finalSystemPrompt.toStdString()}});

    // 添加上下文历史 / Add context history
    {
        std::lock_guard<std::mutex> lock(m_contextMutex);
        Context& ctx = m_contexts[clientId]; 
        if (ctx.max_len != cfg.context_num) ctx.max_len = cfg.context_num; // 更新上下文长度 / Update context length
        while (ctx.history.size() > ctx.max_len) ctx.history.pop_front(); // 清理超出限制的历史 / Clean history beyond limit
        
        // 添加历史消息 / Add history messages
        for (const auto& pair : ctx.history) {
            messages.push_back({{"role", "user"}, {"content", pair.first.toStdString()}});
            messages.push_back({{"role", "assistant"}, {"content", pair.second.toStdString()}});
        }
    }

    // 添加当前用户消息 / Add current user message
    QString currentUserContent = cfg.pre_prompt + processedText;
    messages.push_back({{"role", "user"}, {"content", currentUserContent.toStdString()}});

    // 构建请求负载 / Build request payload
    json payload;
    payload["model"] = cfg.model_name.toStdString();
    payload["messages"] = messages;
    payload["temperature"] = cfg.temperature;

    // 发送网络请求 / Send network request
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(cfg.api_address + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    request.setTransferTimeout(45000); // 设置传输超时 / Set transfer timeout

    QNetworkReply* reply = manager.post(request, QByteArray::fromStdString(payload.dump()));
    
    // 设置事件循环和定时器 / Set up event loop and timers
    QEventLoop loop;
    QTimer checkTimer;
    checkTimer.setInterval(100); // 100ms检查一次停止请求 / Check stop request every 100ms
    
    // 定时检查停止请求 / Timer to check stop request
    QObject::connect(&checkTimer, &QTimer::timeout, [&](){
        if (m_stopRequested) {
            reply->abort(); // 如果请求停止，中止请求 / If stop requested, abort request
            loop.quit();
        }
    });
    checkTimer.start();

    // 超时定时器 / Timeout timer
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    // 连接回复完成信号 / Connect reply finished signal
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    timeoutTimer.start(40000); // 40秒超时 / 40 second timeout
    loop.exec(); // 执行事件循环 / Execute event loop

    QString resultText = ""; // 结果文本 / Result text

    // 检查是否被请求停止 / Check if stop requested
    if (m_stopRequested) {
        reply->deleteLater();
        return ""; 
    }

    // 检查是否超时 / Check if timed out
    if (!timeoutTimer.isActive()) {
        emit logMessage("❌ Request Timeout"); // 超时日志 / Timeout log
        reply->abort();
        reply->deleteLater();
        return ""; 
    }
    timeoutTimer.stop(); // 停止超时定时器 / Stop timeout timer
    checkTimer.stop(); // 停止检查定时器 / Stop check timer

    // 处理回复 / Process reply
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseBytes = reply->readAll();
        try {
            json response = json::parse(responseBytes.toStdString());

            // 提取token使用量 / Extract token usage
            if (response.contains("usage")) {
                int p = response["usage"].value("prompt_tokens", 0);
                int c = response["usage"].value("completion_tokens", 0);
                if (p > 0 || c > 0) emit tokenUsageReceived(p, c); // 发送token使用量信号 / Emit token usage signal
            }

            // 提取回复内容 / Extract response content
            if (response.contains("choices") && !response["choices"].empty()) {
                std::string content = response["choices"][0]["message"]["content"];
                QString rawContent = QString::fromStdString(content);

                // 清理内容（移除think标签） / Clean content (remove think tags)
                QString cleanContent = rawContent;
                cleanContent.remove(QRegularExpression("<think>.*?</think>", QRegularExpression::DotMatchesEverythingOption));

                // 如果执行术语提取 / If performing term extraction
                if (performExtraction) {
                    QRegularExpression reTm("<tm>\\s*(.*?)\\s*=\\s*(.*?)\\s*</tm>", QRegularExpression::DotMatchesEverythingOption);
                    QRegularExpression tokenRegex(R"(\[T_\d+\])"); // 匹配占位符 / Match placeholders
                    QRegularExpression termCodeRegex("Z[A-Z]{2}Z"); // 匹配术语代码 / Match term codes

                    // 重构字符串 / Reconstruct string
                    QString reconstructionBuffer;
                    int lastPos = 0;
                    
                    QRegularExpressionMatchIterator i = reTm.globalMatch(cleanContent);
                    while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();
                        QString k = match.captured(1).trimmed(); // 原文 / Source text
                        QString v = match.captured(2).trimmed(); // 译文 / Target text
                        
                        // 1. 追加上一个匹配点到当前匹配点之间的普通文本
                        // 1. Append ordinary text between last match and current match
                        reconstructionBuffer.append(cleanContent.mid(lastPos, match.capturedStart() - lastPos));
                        
                        // 2. 处理术语逻辑 / Process term logic
                        bool isValidTerm = true;
                        if (k.isEmpty() || v.isEmpty()) isValidTerm = false;
                        else if (k.contains(tokenRegex) || v.contains(tokenRegex)) isValidTerm = false;
                        else if (k.contains(termCodeRegex) || v.contains(termCodeRegex)) isValidTerm = false;
                        
                        if (isValidTerm) {
                            if (processedText.contains(k, Qt::CaseInsensitive)) {
                                GlossaryManager::instance().addNewTerm(k, v); // 添加新术语 / Add new term
                                emit logMessage(QString(SV_NEW_TERM[cfg.language]) + k + " = " + v); // 新术语日志 / New term log
                            }
                        }

                        // 3. 关键修复：追加"译文(v)"，而不是留空
                        // 3. Key fix: Append "translation(v)" instead of leaving empty
                        reconstructionBuffer.append(v);

                        lastPos = match.capturedEnd();
                    }
                    
                    // 4. 追加剩余文本 / 4. Append remaining text
                    reconstructionBuffer.append(cleanContent.mid(lastPos));
                    
                    // 用重构后的文本替换原文本 / Replace original text with reconstructed text
                    cleanContent = reconstructionBuffer;
                }

                // 提取翻译结果 / Extract translation result
                QRegularExpression reTl("<tl>(.*?)</tl>", QRegularExpression::DotMatchesEverythingOption);
                QRegularExpressionMatch matchTl = reTl.match(cleanContent);
                
                if (matchTl.hasMatch()) {
                    resultText = matchTl.captured(1).trimmed(); // 提取tl标签内的内容 / Extract content inside tl tags
                } else {
                    resultText = cleanContent.trimmed(); // 使用完整内容 / Use full content
                }

                // 移除残留的tl标签 / Remove leftover tl tags
                resultText.remove("<tl>", Qt::CaseInsensitive);
                resultText.remove("</tl>", Qt::CaseInsensitive);

                // ========== 第2步：局部解冻 ========== / Step 2: Local thaw
                // 使用成员函数调用解冻转义符 / Use member function to thaw escapes
                resultText = thawEscapesLocal(resultText, escapeCtx);

                // 如果启用术语表，进行后处理 / If glossary enabled, postprocess
                if (cfg.enable_glossary) {
                    resultText = RegexManager::instance().processPost(resultText);
                }

                // 记录翻译结果 / Log translation result
                emit logMessage("  -> " + resultText); 

                // 如果结果有效，添加上下文历史 / If result valid, add to context history
                if (isValidTranslationResult(resultText)) {
                    std::lock_guard<std::mutex> lock(m_contextMutex);
                    Context& ctx = m_contexts[clientId];
                    ctx.history.push_back({currentUserContent, resultText});
                    while (ctx.history.size() > ctx.max_len) ctx.history.pop_front(); // 保持历史长度限制 / Maintain history length limit
                } else {
                    resultText = ""; // 无效结果 / Invalid result
                }
            } else {
                emit logMessage("❌ " + QString(SV_ERR_FMT[cfg.language])); // 格式错误日志 / Format error log
                resultText = ""; 
            }
        } catch (...) {
            emit logMessage("❌ " + QString(SV_ERR_JSON[cfg.language])); // JSON解析错误日志 / JSON parse error log
            resultText = ""; 
        }
    } else {
        // 网络错误 / Network error
        emit logMessage("❌ Network Error: " + reply->errorString());
        resultText = ""; 
    }

    reply->deleteLater(); // 清理回复对象 / Clean up reply object
    return resultText; 
}

/**
 * 获取下一个API密钥（轮询） / Get next API key (round-robin)
 * @return API密钥 / API key
 */
QString TranslationServer::getNextApiKey() {
    std::lock_guard<std::mutex> lock(m_keyMutex); // 加锁保证线程安全 / Lock for thread safety
    if (m_apiKeys.empty()) return ""; // 如果没有密钥，返回空 / If no keys, return empty
    QString key = m_apiKeys[m_currentKeyIndex]; // 获取当前密钥 / Get current key
    m_currentKeyIndex = (m_currentKeyIndex + 1) % m_apiKeys.size(); // 更新索引（轮询） / Update index (round-robin)
    return key;
}

/**
 * 生成客户端ID / Generate client ID
 * @param ip 客户端IP地址 / Client IP address
 * @return 客户端ID / Client ID
 */
QString TranslationServer::generateClientId(const std::string& ip) {
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(ip), QCryptographicHash::Md5); // 使用MD5哈希 / Use MD5 hash
    return hash.toHex().left(8); // 返回前8个字符 / Return first 8 characters
}

/**
 * 清除所有客户端上下文 / Clear all client contexts
 */
void TranslationServer::clearAllContexts() {
    std::lock_guard<std::mutex> lock(m_contextMutex); // 加锁保证线程安全 / Lock for thread safety
    m_contexts.clear(); // 清空所有上下文 / Clear all contexts
    
    // 获取语言设置用于日志 / Get language setting for logging
    int langIdx = 1;
    {
         std::lock_guard<std::mutex> lock(m_configMutex);
         langIdx = m_config.language;
    }
    
    // 发送清除日志 / Send clear log
    QString msg = (langIdx == 0) ? "🧹 Context memory cleared." : "🧹 上下文记忆已清空。";
    emit logMessage(msg); 
}