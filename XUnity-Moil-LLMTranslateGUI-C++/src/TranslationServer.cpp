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
// Index 0: English, Index 1: Chinese
// 索引0: 英文, 索引1: 中文
// ==========================================

// 服务器启动日志
// Server start log
const char* SV_LOG_START[] = { "Server started. Port: %1, Threads: %2", "服务已启动，端口：%1，并发线程数：%2" };

// 服务器停止日志
// Server stop log
const char* SV_LOG_STOP[] = { "Server stopped", "服务已停止" };

// 请求接收日志
// Request received log
const char* SV_LOG_REQ[] = { "Request received: ", "收到请求: " };

// API密钥错误
// API key error
const char* SV_ERR_KEY[] = { "Error: Invalid API Key", "错误：API 密钥无效" };

// 响应格式错误
// Response format error
const char* SV_ERR_FMT[] = { "Error: Invalid Response Format", "错误：响应格式无效" };

// JSON解析错误
// JSON parse error
const char* SV_ERR_JSON[] = { "Error: JSON Parse Error", "错误：JSON 解析失败" };

// 新术语发现日志
// New term discovered log
const char* SV_NEW_TERM[] = { "✨ New Term Discovered: ", "✨ 发现新术语: " };

// 格式警告日志
// Format warning log
const char* SV_WARN_TAG[] = { "⚠️ Format Warning: LLM missing <tl> tag, auto-cleaned.", "⚠️ 格式警告：LLM 未返回 <tl> 标签，已自动清洗。" };

// 重试尝试日志
// Retry attempt log
const char* SV_RETRY_ATTEMPT[] = { "🔄 Retry translation (%1/%2): ", "🔄 重试翻译 (%1/%2): " };

// 重试成功日志
// Retry success log
const char* SV_RETRY_SUCCESS[] = { "✅ Retry successful", "✅ 重试成功" };

// 重试失败日志
// Retry failed log
const char* SV_RETRY_FAILED[] = { "❌ Retry failed, skipping text", "❌ 重试失败，跳过文本" };

// 翻译终止日志
// Translation aborted log
const char* SV_ABORTED[] = { "⛔ Translation Aborted", "⛔ 翻译已终止" };

/**
 * 构造函数
 * Constructor
 * @param parent 父对象指针 / Parent object pointer
 */
TranslationServer::TranslationServer(QObject *parent) : QObject(parent), m_running(false) {
    m_stopRequested = false; // 初始化停止请求标志为false / Initialize stop request flag to false
}

/**
 * 析构函数
 * Destructor
 */
TranslationServer::~TranslationServer() {
    stopServer(); // 确保服务器停止 / Ensure server stops
}

/**
 * 更新服务器配置
 * Update server configuration
 * @param config 应用配置对象 / Application configuration object
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
        // 1. 设置术语表文件路径，供 GlossaryManager 提取提示词使用 (RAG)
        // 1. Set glossary file path for GlossaryManager to extract prompts (RAG)
        GlossaryManager::instance().setFilePath(m_config.glossary_path);
        
        // 🔥 关键修复：禁止 RegexManager 自动加载该文件进行硬替换
        // 🔥 Critical fix: Prevent RegexManager from automatically loading the file for hard replacement
        // 这避免了 "Aira" 在发送给 LLM 前就被替换成 "艾拉"，导致语境破坏和提示词失效
        // This avoids "Aira" being replaced with "艾拉" before sending to LLM, which would break context and invalidate prompts
        // RegexManager::instance().autoLoadFrom(m_config.glossary_path); 
    }
}

/**
 * 启动翻译服务器
 * Start translation server
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
 * 停止翻译服务器
 * Stop translation server
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
 * 服务器主循环
 * Server main loop
 */
void TranslationServer::runServerLoop() {
    m_svr = new httplib::Server(); // 创建HTTP服务器实例 / Create HTTP server instance
    
    // 设置线程池大小 / Set thread pool size
    int threads = m_config.max_threads;
    if (threads < 1) threads = 1;
    m_svr->new_task_queue = [threads] { return new httplib::ThreadPool(threads); };

    // 定义GET请求处理函数 / Define GET request handler
    m_svr->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
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

        // 记录接收到的请求 / Log received request
        emit logMessage(QString(SV_LOG_REQ[m_config.language]) + text);
        
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
 * 执行翻译（带重试机制）
 * Perform translation (with retry mechanism)
 * @param text 待翻译文本 / Text to be translated
 * @param clientIP 客户端IP地址 / Client IP address
 * @return 翻译结果 / Translation result
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
                                  .arg(MAX_RETRY_COUNT) + text;
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
 * 验证翻译结果是否有效
 * Validate if translation result is valid
 * @param result 翻译结果 / Translation result
 * @return 是否有效 / Whether result is valid
 */
bool TranslationServer::isValidTranslationResult(const QString& result) {
    return !result.isEmpty() && 
           !result.startsWith("Error", Qt::CaseInsensitive) &&
           !result.contains("翻译失败", Qt::CaseInsensitive) &&
           !result.contains("translation failed", Qt::CaseInsensitive) &&
           result.length() > 0;
}

/**
 * 执行单次翻译尝试
 * Perform single translation attempt
 * @param text 待翻译文本 / Text to be translated
 * @param clientIP 客户端IP地址 / Client IP address
 * @return 翻译结果 / Translation result
 */
QString TranslationServer::performSingleTranslationAttempt(const QString& text, const QString& clientIP) {
    if (m_stopRequested) return ""; // 检查是否请求停止 / Check if stop requested

    // 获取下一个API密钥 / Get next API key
    QString apiKey = getNextApiKey();
    if (apiKey.isEmpty()) {
        emit logMessage("❌ " + QString(SV_ERR_KEY[m_config.language]));
        return "";
    }

    // 🔥 关键修复：确保发给 LLM 的是原文 (Raw Text)
    // 🔥 Critical fix: Ensure raw text is sent to LLM
    // 之前如果这里执行了 RegexManager::processPre 且该 Manager 加载了术语表
    // 就会发生 "Aira" -> "艾拉" 的硬替换，破坏语境。
    // Previously if RegexManager::processPre was called here and the manager loaded the glossary,
    // "Aira" would be hard-replaced with "艾拉", breaking context.
    // 现在我们仅依赖 GlossaryManager 的 Prompt 提示。
    // Now we rely only on GlossaryManager's Prompt hints.
    QString processedText = text;
    
    /* 
       注意：如果你有除了 _Substitutions.txt 以外的正则表达式清理需求，
       请确保 RegexManager 不会加载 _Substitutions.txt，或者在这里恢复 processPre 调用。
       鉴于目前的配置结构，禁用了 autoLoadFrom 后，调用 processPre 是安全的（不会有规则），
       但为了保险，我们暂时保留 processedText = text。
       
       Note: If you have regex cleaning needs beyond _Substitutions.txt,
       ensure RegexManager does not load _Substitutions.txt, or restore processPre call here.
       Given the current configuration structure, after disabling autoLoadFrom, 
       calling processPre is safe (no rules will be loaded),
       but for safety, we keep processedText = text for now.
    */
    if (m_config.enable_glossary) {
         processedText = RegexManager::instance().processPre(text);
    }

    // 生成客户端ID / Generate client ID
    std::string clientId = generateClientId(clientIP.toStdString()).toStdString();
    
    // 构建系统提示词 / Build system prompt
    QString finalSystemPrompt = m_config.system_prompt;
    bool performExtraction = false; // 是否执行术语提取 / Whether to perform term extraction

    // 基础格式规则 / Basic format rules
    finalSystemPrompt += "\n\n【Format Rules】:\n"
                         "- Preserve ALL escape characters (\\n, \\r) and formatting.\n"
                         "- Do NOT explain. Just output the translation.\n";

    // 如果启用了术语表 / If glossary is enabled
    if (m_config.enable_glossary) {
        // 🔥 关键修复：使用原始 text 查找术语
        // 🔥 Critical fix: Use original text to find terms
        // 确保即使 processedText 被修改，也能基于原文找到 "Aira" 这样的关键词
        // Ensure that even if processedText is modified, keywords like "Aira" can be found based on original text
        QString glossaryContext = GlossaryManager::instance().getContextPrompt(text);
        if (!glossaryContext.isEmpty()) {
            finalSystemPrompt += "\n" + glossaryContext;
        }

        // 依然保持全量提取，但加上长度限制避免短词干扰
        // Still maintain full extraction, but add length limit to avoid short word interference
        if (text.length() > 2) { 
            performExtraction = true;
            finalSystemPrompt += "\n【Advanced Instruction】:\n"
                                 "1. Wrap the final translation inside <tl> and </tl> tags.\n"
                                 "   Example: <tl>你好，世界。</tl>\n"
                                 "2. IDENTIFY and EXTRACT Proper Nouns (Names, Places, Skills) NOT in the glossary using <tm>Original = Translated</tm>.\n"
                                 "   Example: <tm>Excalibur = 誓约胜利之剑</tm>\n";
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
    request.setTransferTimeout(35000); // 设置传输超时 / Set transfer timeout

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
    
    timeoutTimer.start(30000); // 30秒超时 / 30 second timeout
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

                // 如果启用了术语提取 / If term extraction is enabled
                if (performExtraction) {
                    // 提取新术语 / Extract new terms
                    QRegularExpression reTm("<tm>\\s*(.*?)\\s*=\\s*(.*?)\\s*</tm>", QRegularExpression::DotMatchesEverythingOption);
                    QRegularExpressionMatchIterator i = reTm.globalMatch(rawContent);
                    while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();
                        QString k = match.captured(1).trimmed(); // 原始术语 / Original term
                        QString v = match.captured(2).trimmed(); // 翻译术语 / Translated term
                        
                        // 检查原始术语是否存在于文本中 / Check if original term exists in text
                        if (!k.isEmpty() && !v.isEmpty() && processedText.contains(k, Qt::CaseInsensitive)) {
                            GlossaryManager::instance().addNewTerm(k, v); // 添加到术语表 / Add to glossary
                            emit logMessage(QString(SV_NEW_TERM[m_config.language]) + k + " = " + v);
                        }
                    }

                    // 清理新术语标签 / Clean new term tags
                    QString cleanContent = rawContent;
                    cleanContent.remove(reTm); 

                    // 提取翻译结果标签 / Extract translation result tags
                    QRegularExpression reTl("<tl>(.*?)</tl>", QRegularExpression::DotMatchesEverythingOption);
                    QRegularExpressionMatch matchTl = reTl.match(cleanContent);
                    
                    if (matchTl.hasMatch()) {
                        resultText = matchTl.captured(1).trimmed(); // 提取翻译内容 / Extract translation content
                    } else {
                        // 信任模式：保留原文格式 (处理 <dash=6> 等情况)
                        // Trust mode: Preserve original format (handles cases like <dash=6>)
                        resultText = cleanContent.trimmed(); 
                    }
                } else {
                    // 未启用术语提取时直接使用原始内容 / Use raw content when term extraction is disabled
                    resultText = rawContent;
                    
                    // 过滤思考标签 / Filter think tags
                    std::regex think_regex("<think>.*?</think>", std::regex_constants::ECMAScript | std::regex_constants::icase);
                    std::string filtered = std::regex_replace(resultText.toStdString(), think_regex, "");
                    resultText = QString::fromStdString(filtered).trimmed();
                }

                // 恢复转义字符 / Restore escape characters
                resultText.replace("\\n", "\n");
                resultText.replace("\\r", "\r");

                // 后处理：如果启用术语表，应用后处理规则 / Post-processing: If glossary enabled, apply post-processing rules
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
                QString err = SV_ERR_FMT[m_config.language];
                emit logMessage("❌ " + err);
                resultText = ""; 
            }
        } catch (...) {
            // JSON解析错误 / JSON parse error
            QString err = SV_ERR_JSON[m_config.language];
            emit logMessage("❌ " + err);
            resultText = ""; 
        }
    } else {
        // 网络错误 / Network error
        QString errStr = reply->errorString();
        emit logMessage("❌ Network Error: " + errStr);
        resultText = ""; 
    }

    reply->deleteLater(); // 清理回复对象 / Clean up reply object
    return resultText; 
}

/**
 * 获取下一个API密钥（轮询机制）
 * Get next API key (round-robin mechanism)
 * @return API密钥 / API key
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
 * @param ip 客户端IP地址 / Client IP address
 * @return 客户端ID / Client ID
 */
QString TranslationServer::generateClientId(const std::string& ip) {
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(ip), QCryptographicHash::Md5);
    return hash.toHex().left(8); // 取前8个字符作为ID / Take first 8 characters as ID
}

/**
 * 清除所有客户端的上下文记忆
 * Clear context memory for all clients
 */
void TranslationServer::clearAllContexts() {
    std::lock_guard<std::mutex> lock(m_contextMutex); 
    m_contexts.clear(); // 清空所有上下文 / Clear all contexts
    QString msg = (m_config.language == 0) ? "🧹 Context memory cleared." : "🧹 上下文记忆已清空。";
    emit logMessage(msg); // 发送清除完成消息 / Send clear completion message
}