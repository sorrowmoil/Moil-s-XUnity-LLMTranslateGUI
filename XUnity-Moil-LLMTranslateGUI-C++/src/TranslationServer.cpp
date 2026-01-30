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
// 📝 Server Log Dictionary
// 📝 服务器日志字典
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

// ==========================================
// 🧊 Helper Structs & Functions
// 🧊 辅助结构体和函数
// ==========================================

// <实验性> 定义一个结构体来保存替换映射，确保线程安全
// <Experimental> Define a struct to store replacement mappings, ensuring thread safety
struct EscapeMap {
    QMap<QString, QString> map; // 占位符到原始内容的映射 / Placeholder to original content mapping
    int counter = 0; // 计数器，用于生成唯一占位符 / Counter for generating unique placeholders
};

// <实验性> 静态辅助函数：执行冻结（加空格策略 - Physical Isolation）
// <Experimental> Static helper function: Execute freezing (space addition strategy - Physical Isolation)
static QString freezeEscapesLocal(const QString& input, EscapeMap& context) {
    QString result = input;
    context.map.clear();
    context.counter = 0;

    // 定义需要保护的模式 / Define patterns that need protection
    // 1. {{...}} 模板变量 (非贪婪匹配) / {{...}} template variables (non-greedy matching)
    // 2. <...> Unity/XML 标签 (非贪婪匹配) / <...> Unity/XML tags (non-greedy matching)
    // 3. 字面量转义符 (\r\n, \n, \r, \t) - 注意双反斜杠转义 / Literal escape characters (\r\n, \n, \r, \t) - note double backslash escaping
    // 4. ASCII 控制符 (实际的换行等) / ASCII control characters (actual newlines, etc.)
    // 注意：正则顺序很重要，先长后短，先特殊后通用
    // Note: Regex order is important, long before short, special before general
    QRegularExpression regex(R"(\{\{.*?\}\}|<[^>]+>|\\r\\n|\\n|\\r|\\t|\r\n|\n|\r|\t)");
    
    int offset = 0;
    QRegularExpressionMatchIterator i = regex.globalMatch(result);
    
    // 我们构建一个新的字符串以避免原地替换导致的索引混乱
    // We construct a new string to avoid index confusion caused by in-place replacement
    QString newResult;
    int lastEnd = 0;
    
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        
        // 1. 追加匹配项之前的内容 / Append content before the match
        newResult.append(result.mid(lastEnd, match.capturedStart() - lastEnd));
        
        // 2. 生成带空格的占位符 [T_x] / Generate placeholder with spaces [T_x]
        // 前后加空格是为了强制让 LLM 认为这是一个独立的 Token，避免被"优化"掉
        // Adding spaces before and after forces LLM to treat this as an independent Token, avoiding being "optimized" away
        QString original = match.captured(0);
        QString tokenKey = QString("[T_%1]").arg(context.counter++); 
        QString tokenWithSpace = QString(" %1 ").arg(tokenKey);
        
        context.map[tokenKey] = original; // Map 中只存纯 Key / Map stores only pure Key
        
        newResult.append(tokenWithSpace);
        
        lastEnd = match.capturedEnd();
    }
    
    // 3. 追加剩余内容 / Append remaining content
    newResult.append(result.mid(lastEnd));
    
    return newResult;
}

// <实验性> 静态辅助函数：执行解冻（智能去空格）
// <Experimental> Static helper function: Execute thawing (intelligent space removal)
static QString thawEscapesLocal(const QString& input, const EscapeMap& context) {
    QString result = input;
    
    // 正则匹配 [T_数字] 及其周围可能存在的空白字符
    // Regex matches [T_number] and any surrounding whitespace characters
    // \s* 会吃掉 freeze 时加入的空格，也会吃掉 LLM 可能无意中添加的空格
    // \s* will consume spaces added during freezing, and also spaces LLM may inadvertently add
    QRegularExpression tokenRegex(R"(\s*\[T_(\d+)\]\s*)");
    
    QRegularExpressionMatchIterator i = tokenRegex.globalMatch(result);
    
    QString newResult;
    int lastEnd = 0;
    
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        
        // 追加前文 / Append preceding text
        newResult.append(result.mid(lastEnd, match.capturedStart() - lastEnd));
        
        // 获取 Key / Get Key
        QString key = QString("[T_%1]").arg(match.captured(1));
        
        // 还原内容 / Restore content
        if (context.map.contains(key)) {
            newResult.append(context.map[key]);
        } else {
            // 如果找不到（极少情况），就保留 Key 原样（去掉多余空格）
            // If not found (rare case), keep Key as is (remove extra spaces)
            newResult.append(key);
        }
        
        lastEnd = match.capturedEnd();
    }
    
    newResult.append(result.mid(lastEnd));
    
    return newResult;
}

// ==========================================
// 🚀 TranslationServer Implementation
// 🚀 TranslationServer 实现
// ==========================================

/**
 * 构造函数 / Constructor
 */
TranslationServer::TranslationServer(QObject *parent) : QObject(parent), m_running(false) {
    m_stopRequested = false; 
    m_svr = nullptr; // 初始化HTTP服务器指针为nullptr / Initialize HTTP server pointer to nullptr
    m_serverThread = nullptr; // 初始化服务器线程指针为nullptr / Initialize server thread pointer to nullptr
}

/**
 * 析构函数 / Destructor
 */
TranslationServer::~TranslationServer() {
    stopServer(); // 确保服务器停止 / Ensure server stops
}

/**
 * 更新服务器配置 / Update server configuration
 */
void TranslationServer::updateConfig(const AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_keyMutex); // 加锁保护API密钥列表 / Lock to protect API key list
    
    m_config = config; // 更新配置 / Update configuration
    
    // 解析API密钥（支持逗号分隔的多个密钥） / Parse API keys (support comma-separated multiple keys)
    m_apiKeys.clear();
    QStringList keys = m_config.api_key.split(',', Qt::SkipEmptyParts);
    for(const auto& k : keys) m_apiKeys.push_back(k.trimmed());
    m_currentKeyIndex = 0; // 重置密钥索引 / Reset key index
    
    // 如果启用了术语表 / If glossary is enabled
    if (m_config.enable_glossary) {
        GlossaryManager::instance().setFilePath(m_config.glossary_path);
    }
}

/**
 * 启动翻译服务器 / Start translation server
 */
void TranslationServer::startServer() {
    if (m_running) return; // 如果已在运行则直接返回 / Return if already running
    m_running = true;
    m_stopRequested = false; // 重置停止请求标志 / Reset stop request flag
    
    // 创建并启动服务器线程 / Create and start server thread
    m_serverThread = new std::thread(&TranslationServer::runServerLoop, this);
    
    // 发送启动日志 / Send start log
    QString msg = QString(SV_LOG_START[m_config.language]).arg(m_config.port).arg(m_config.max_threads);
    emit logMessage(msg);
}

/**
 * 停止翻译服务器 / Stop translation server
 */
void TranslationServer::stopServer() {
    if (!m_running) return; // 如果未运行则直接返回 / Return if not running
    
    m_stopRequested = true; // 设置停止请求标志 / Set stop request flag
    m_running = false;
    
    // 停止HTTP服务器 / Stop HTTP server
    if (m_svr) m_svr->stop();
    
    // 等待服务器线程结束 / Wait for server thread to finish
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
        delete m_serverThread;
        m_serverThread = nullptr;
    }
    
    // 清理服务器实例 / Clean up server instance
    delete m_svr;
    m_svr = nullptr;
    
    // 发送停止日志 / Send stop log
    emit logMessage(SV_LOG_STOP[m_config.language]);
}

/**
 * 服务器主循环 / Server main loop
 */
void TranslationServer::runServerLoop() {
    m_svr = new httplib::Server(); // 创建HTTP服务器实例 / Create HTTP server instance
    
    // 设置线程池大小 / Set thread pool size
    int threads = m_config.max_threads;
    if (threads < 1) threads = 1;
    m_svr->new_task_queue = [threads] { return new httplib::ThreadPool(threads); };

    // 定义GET请求处理函数 / Define GET request handler
    m_svr->Get("/",  [this](const httplib::Request& req, httplib::Response& res) {
        // 检查是否有text参数 / Check if text parameter exists
        if (!req.has_param("text")) { 
            res.set_content("", "text/plain"); 
            return; 
        }
        
        // 获取并清理文本 / Get and clean text
        std::string text_std = req.get_param_value("text");
        QString text = QString::fromStdString(text_std).trimmed();
        
        if (text.isEmpty()) { 
            res.set_content("", "text/plain; charset=utf-8"); 
            return; 
        }

        // 日志显示优化：替换真实换行符以便在单行日志中查看
        // Log display optimization: Replace actual newlines for single-line log viewing
        QString logText = text;
        logText.replace("\n", "[LF]");
        emit logMessage(QString(SV_LOG_REQ[m_config.language]) + logText);
        
        // 发出工作开始信号 / Emit work started signal
        emit workStarted(); 

        // 执行翻译 / Perform translation
        QString result = performTranslation(text, QString::fromStdString(req.remote_addr));
        
        // 如果未请求停止，发送工作完成信号 / If stop not requested, send work finished signal
        if (!m_stopRequested) {
            bool success = !result.isEmpty();
            emit workFinished(success); 
        } else {
            emit workFinished(false); 
        }

        // 设置响应内容 / Set response content
        if (result.isEmpty()) {
            res.status = 500; // 内部服务器错误 / Internal server error
            res.set_content("Translation Failed", "text/plain"); 
        } else {
            res.set_content(result.toStdString(), "text/plain; charset=utf-8");
        }
    });
    
    // 启动HTTP服务器监听 / Start HTTP server listening
    m_svr->listen("0.0.0.0", m_config.port);
}

/**
 * 执行翻译（带重试机制） / Perform translation (with retry mechanism)
 */
QString TranslationServer::performTranslation(const QString& text, const QString& clientIP) {
    QString resultText = "";
    int retryCount = 0;
    const int MAX_RETRY_COUNT = 5; // 最大重试次数 / Maximum retry count
    const int RETRY_DELAY_MS = 1000; // 重试延迟（毫秒） / Retry delay (milliseconds)
    
    // 重试循环 / Retry loop
    while (retryCount < MAX_RETRY_COUNT) {
        // 检查是否请求停止 / Check if stop requested
        if (m_stopRequested) {
            emit logMessage(SV_ABORTED[m_config.language]);
            return "";
        }

        // 如果不是第一次尝试，记录重试信息 / If not first attempt, log retry info
        if (retryCount > 0) {
            QString retryMsg = QString(SV_RETRY_ATTEMPT[m_config.language])
                                  .arg(retryCount + 1)
                                  .arg(MAX_RETRY_COUNT);
            emit logMessage(retryMsg);
            
            // 重试延迟 / Retry delay
            for (int i = 0; i < RETRY_DELAY_MS / 100; ++i) {
                if (m_stopRequested) return "";
                QThread::msleep(100);
            }
        }
        
        // 执行单次翻译尝试 / Perform single translation attempt
        QString attemptResult = performSingleTranslationAttempt(text, clientIP);
        
        // 再次检查是否请求停止 / Check again if stop requested
        if (m_stopRequested) return "";

        // 验证翻译结果是否有效 / Validate if translation result is valid
        if (isValidTranslationResult(attemptResult)) {
            if (retryCount > 0) emit logMessage(SV_RETRY_SUCCESS[m_config.language]);
            resultText = attemptResult;
            break; // 成功，退出循环 / Success, break loop
        }
        
        retryCount++; // 增加重试计数 / Increment retry count
        
        // 达到最大重试次数 / Maximum retry count reached
        if (retryCount >= MAX_RETRY_COUNT) {
            emit logMessage(SV_RETRY_FAILED[m_config.language]);
            resultText = ""; // 清空结果 / Clear result
        }
    }
    return resultText;
}

/**
 * 验证翻译结果是否有效 / Validate if translation result is valid
 */
bool TranslationServer::isValidTranslationResult(const QString& result) {
    return !result.isEmpty() && 
           !result.startsWith("Error", Qt::CaseInsensitive) &&
           !result.contains("翻译失败", Qt::CaseInsensitive) &&
           !result.contains("translation failed", Qt::CaseInsensitive) &&
           result.length() > 0;
}

/**
 * 执行单次翻译尝试 / Perform single translation attempt
 */
QString TranslationServer::performSingleTranslationAttempt(const QString& text, const QString& clientIP) {
    if (m_stopRequested) return ""; // 检查是否请求停止 / Check if stop requested

    // 获取下一个API密钥 / Get next API key
    QString apiKey = getNextApiKey();
    if (apiKey.isEmpty()) {
        emit logMessage("❌ " + QString(SV_ERR_KEY[m_config.language]));
        return "";
    }

    // ========== 第1步：局部冻结（线程安全 + 物理隔离） ==========
    // ========== Step 1: Local freezing (thread-safe + physical isolation) ==========
    // <实验性> 创建冻结上下文 / <Experimental> Create freeze context
    EscapeMap escapeCtx;
    // <实验性> 冻结文本中的特殊字符 / <Experimental> Freeze special characters in text
    QString processedText = freezeEscapesLocal(text, escapeCtx);
    
    // 预处理 (RegexManager) / Preprocessing (RegexManager)
    if (m_config.enable_glossary) {
         processedText = RegexManager::instance().processPre(processedText);
    }

    // 生成客户端ID / Generate client ID
    std::string clientId = generateClientId(clientIP.toStdString()).toStdString();
    
    // 构建系统提示词 / Build system prompt
    QString finalSystemPrompt = m_config.system_prompt;
    bool performExtraction = false; // 是否执行术语提取 / Whether to perform term extraction

    // ==========================================
    // 🛠️ CAN MODIFICATION: 极简流 Prompt
    // ==========================================
    // <实验性> 调整后的系统提示词，重点要求保留占位符
    // <Experimental> Adjusted system prompt focusing on preserving placeholders
   finalSystemPrompt += "\n\n【Translation Rules】:\n"
                     "1. 🛑 PRESERVE TAGS: You will see tags like '[T_0]', '[T_1]'.\n"
                     "   - These replace newlines or code. Keep them EXACTLY as is.\n"
                     "   - Input: \"Hello [T_0] World\"\n"
                     "   - Output: \"你好 [T_0] 世界\"\n"
                     "2. 🛑 NO CLEANUP: Do NOT remove the tags.\n"
                     "3. 🔰 TERM CODES: Keep 'Z[A-Z]{2}Z' (e.g., 'ZMCZ') codes exactly as is.\n"
                     "4. Translate the text BETWEEN the tags naturally.\n"
                     "5. Output ONLY the translated result.\n";
                     
    // 如果启用了术语表 / If glossary is enabled
    if (m_config.enable_glossary) {
        QString glossaryContext = GlossaryManager::instance().getContextPrompt(processedText);
        if (!glossaryContext.isEmpty()) {
            finalSystemPrompt += "\n" + glossaryContext;
        }

        // 只有纯文本长度够长才提取，避免提取占位符
        // Only extract when pure text is long enough, avoid extracting placeholders
        if (text.length() > 5) { 
            performExtraction = true;
            finalSystemPrompt += "\n【Term Extraction】:\n"
                                 "1. Wrap translation in <tl>...</tl>.\n"
                                 "2. If you find Proper Nouns (Names) NOT in glossary, output <tm>Src=Trgt</tm>.\n";
        }
    }

    // 构建消息数组 / Build messages array
    json messages = json::array();
    messages.push_back({{"role", "system"}, {"content", finalSystemPrompt.toStdString()}});

    // 管理对话上下文 / Manage conversation context
    std::lock_guard<std::mutex> lock(m_contextMutex);
    Context& ctx = m_contexts[clientId]; 
    if (ctx.max_len != m_config.context_num) ctx.max_len = m_config.context_num;
    while (ctx.history.size() > ctx.max_len) ctx.history.pop_front();
    
    // 添加历史对话到消息数组 / Add conversation history to messages array
    for (const auto& pair : ctx.history) {
        messages.push_back({{"role", "user"}, {"content", pair.first.toStdString()}});
        messages.push_back({{"role", "assistant"}, {"content", pair.second.toStdString()}});
    }

    // 添加当前用户消息 / Add current user message
    QString currentUserContent = m_config.pre_prompt + processedText;
    messages.push_back({{"role", "user"}, {"content", currentUserContent.toStdString()}});

    // 构建请求载荷 / Build request payload
    json payload;
    payload["model"] = m_config.model_name.toStdString();
    payload["messages"] = messages;
    payload["temperature"] = m_config.temperature;

    // 创建网络请求 / Create network request
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(m_config.api_address + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    request.setTransferTimeout(45000); // 设置传输超时 / Set transfer timeout

    // 发送POST请求 / Send POST request
    QNetworkReply* reply = manager.post(request, QByteArray::fromStdString(payload.dump()));
    
    // 设置事件循环和定时器 / Set up event loop and timers
    QEventLoop loop;
    QTimer checkTimer;
    checkTimer.setInterval(100);
    
    // 定期检查停止请求 / Periodically check stop request
    QObject::connect(&checkTimer, &QTimer::timeout, [&](){
        if (m_stopRequested) {
            reply->abort(); // 中止请求 / Abort request
            loop.quit();
        }
    });
    checkTimer.start();

    // 设置超时定时器 / Set timeout timer
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    // 请求完成时退出事件循环 / Exit event loop when request completes
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    timeoutTimer.start(40000); // 40秒超时 / 40 second timeout
    loop.exec(); // 执行事件循环 / Execute event loop

    QString resultText = ""; // 初始化结果文本 / Initialize result text

    // 检查是否请求停止 / Check if stop requested
    if (m_stopRequested) {
        reply->deleteLater();
        return ""; 
    }

    // 检查是否超时 / Check if timed out
    if (!timeoutTimer.isActive()) {
        emit logMessage("❌ Request Timeout");
        reply->abort();
        reply->deleteLater();
        return ""; 
    }
    timeoutTimer.stop(); // 停止超时定时器 / Stop timeout timer
    checkTimer.stop(); // 停止检查定时器 / Stop check timer

    // 处理响应 / Process response
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseBytes = reply->readAll();
        try {
            json response = json::parse(responseBytes.toStdString());

            // 提取token使用量 / Extract token usage
            if (response.contains("usage")) {
                int p = response["usage"].value("prompt_tokens", 0);
                int c = response["usage"].value("completion_tokens", 0);
                if (p > 0 || c > 0) emit tokenUsageReceived(p, c);
            }

            // 提取响应内容 / Extract response content
            if (response.contains("choices") && !response["choices"].empty()) {
                std::string content = response["choices"][0]["message"]["content"];
                QString rawContent = QString::fromStdString(content);

                // 移除 DeepSeek 的 <think> 标签 / Remove DeepSeek's <think> tags
                QString cleanContent = rawContent;
                cleanContent.remove(QRegularExpression("<think>.*?</think>", QRegularExpression::DotMatchesEverythingOption));

                // 术语提取逻辑 (如果启用) / Term extraction logic (if enabled)
                if (performExtraction) {
                    QRegularExpression reTm("<tm>\\s*(.*?)\\s*=\\s*(.*?)\\s*</tm>", QRegularExpression::DotMatchesEverythingOption);
                    
                    // 占位符过滤 / Placeholder filtering
                    QRegularExpression tokenRegex(R"(\[T_\d+\])"); // 匹配 [T_0] / Matches [T_0]
                    QRegularExpression termCodeRegex("Z[A-Z]{2}Z"); // 匹配 ZMCZ 等 / Matches ZMCZ, etc.

                    QRegularExpressionMatchIterator i = reTm.globalMatch(cleanContent);
                    while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();
                        QString k = match.captured(1).trimmed(); // 原始术语 / Original term
                        QString v = match.captured(2).trimmed(); // 翻译术语 / Translated term
                        
                        if (k.isEmpty() || v.isEmpty()) continue;
                        
                        // 如果术语包含占位符或代码，则忽略
                        // If term contains placeholder or code, ignore
                        if (k.contains(tokenRegex) || v.contains(tokenRegex)) continue;
                        if (k.contains(termCodeRegex) || v.contains(termCodeRegex)) continue;

                        // 检查原始术语是否存在于文本中 / Check if original term exists in text
                        if (processedText.contains(k, Qt::CaseInsensitive)) {
                            GlossaryManager::instance().addNewTerm(k, v); // 添加到术语表 / Add to glossary
                            emit logMessage(QString(SV_NEW_TERM[m_config.language]) + k + " = " + v);
                        }
                    }
                    cleanContent.remove(reTm); // 移除提取标签 / Remove extraction tags
                }

                // 翻译结果提取 (优先级：标签内 > 全文) / Translation result extraction (priority: inside tags > full text)
                QRegularExpression reTl("<tl>(.*?)</tl>", QRegularExpression::DotMatchesEverythingOption);
                QRegularExpressionMatch matchTl = reTl.match(cleanContent);
                
                if (matchTl.hasMatch()) {
                    resultText = matchTl.captured(1).trimmed(); // 从标签中提取 / Extract from tags
                } else {
                    resultText = cleanContent.trimmed(); // 使用全文 / Use full text
                }

                // 清理残留的 <tl> 标签 / Clean up residual <tl> tags
                resultText.remove("<tl>", Qt::CaseInsensitive);
                resultText.remove("</tl>", Qt::CaseInsensitive);

                // ========== 第2步：局部解冻（智能移除空格） ==========
                // ========== Step 2: Local thawing (intelligent space removal) ==========
                // <实验性> 解冻占位符，恢复原始特殊字符
                // <Experimental> Thaw placeholders, restore original special characters
                resultText = thawEscapesLocal(resultText, escapeCtx);

                // 后处理 / Post-processing
                if (m_config.enable_glossary) {
                    resultText = RegexManager::instance().processPost(resultText);
                }

                // 记录翻译结果 / Log translation result
                emit logMessage("  -> " + resultText); 

                // 如果结果有效，保存到历史记录 / If result is valid, save to history
                if (isValidTranslationResult(resultText)) {
                    ctx.history.push_back({currentUserContent, resultText});
                    while (ctx.history.size() > ctx.max_len) ctx.history.pop_front();
                } else {
                    resultText = ""; // 清空无效结果 / Clear invalid result
                }
            } else {
                // 响应格式错误 / Response format error
                emit logMessage("❌ " + QString(SV_ERR_FMT[m_config.language]));
                resultText = ""; 
            }
        } catch (...) {
            // JSON解析错误 / JSON parse error
            emit logMessage("❌ " + QString(SV_ERR_JSON[m_config.language]));
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
 * 获取下一个API密钥（轮询机制） / Get next API key (round-robin mechanism)
 */
QString TranslationServer::getNextApiKey() {
    std::lock_guard<std::mutex> lock(m_keyMutex); 
    if (m_apiKeys.empty()) return ""; // 如果没有密钥，返回空字符串 / If no keys, return empty string
    QString key = m_apiKeys[m_currentKeyIndex];
    m_currentKeyIndex = (m_currentKeyIndex + 1) % m_apiKeys.size(); // 循环索引 / Cycle index
    return key;
}

/**
 * 生成客户端唯一标识符（基于IP地址的MD5哈希）
 * Generate client unique identifier (MD5 hash based on IP address)
 */
QString TranslationServer::generateClientId(const std::string& ip) {
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(ip), QCryptographicHash::Md5);
    return hash.toHex().left(8); // 取前8个字符作为ID / Take first 8 characters as ID
}

/**
 * 清除所有客户端的上下文记忆 / Clear context memory for all clients
 */
void TranslationServer::clearAllContexts() {
    std::lock_guard<std::mutex> lock(m_contextMutex); 
    m_contexts.clear(); // 清空所有上下文 / Clear all contexts
    QString msg = (m_config.language == 0) ? "🧹 Context memory cleared." : "🧹 上下文记忆已清空。";
    emit logMessage(msg); // 发送清除完成消息 / Send clear completion message
}