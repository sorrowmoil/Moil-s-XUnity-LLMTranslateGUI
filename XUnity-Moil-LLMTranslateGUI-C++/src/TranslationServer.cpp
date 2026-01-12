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
#include <QTimer> // For setting network request timeout / 用于设置网络请求超时

using json = nlohmann::json;

// ==========================================
// 📝 后台日志字典 (Server Log Dictionary)
// ==========================================
const char* SV_LOG_START[] = {"服务已启动，端口：%1，并发线程数：%2", "Server started. Port: %1, Threads: %2"};
const char* SV_LOG_STOP[] = {"服务已停止", "Server stopped"};
const char* SV_LOG_REQ[] = {"收到请求: ", "Request received: "};
const char* SV_ERR_KEY[] = {"错误：API 密钥无效", "Error: Invalid API Key"};
const char* SV_ERR_FMT[] = {"错误：响应格式无效", "Error: Invalid Response Format"};
const char* SV_ERR_JSON[] = {"错误：JSON 解析失败", "Error: JSON Parse Error"};
const char* SV_NEW_TERM[] = {"✨ 发现新术语: ", "✨ New Term Discovered: "};
// LLM missing <tl> tag warning / LLM 缺少 <tl> 标签的警告
const char* SV_WARN_TAG[] = {
    "⚠️ 格式警告：LLM 未返回 <tl> 标签，已自动清洗。",
    "⚠️ Format Warning: LLM missing <tl> tag, auto-cleaned."
};

// Retry Messages / 重试信息
const char* SV_RETRY_ATTEMPT[] = {
    "🔄 重试翻译 (%1/%2): ",
    "🔄 Retry translation (%1/%2): "
};
const char* SV_RETRY_SUCCESS[] = {
    "✅ 重试成功",
    "✅ Retry successful"
};
const char* SV_RETRY_FAILED[] = {
    "❌ 重试失败，跳过文本",
    "❌ Retry failed, skipping text"
};


TranslationServer::TranslationServer(QObject *parent) : QObject(parent), m_running(false) {}
// Constructor / 构造函数

TranslationServer::~TranslationServer() {
    stopServer(); // Ensure server is stopped and threads are cleaned up / 确保服务器停止并清理线程
}

/**
 * @brief Updates runtime configuration
 * @brief 更新运行时配置
 */
void TranslationServer::updateConfig(const AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_keyMutex); // Lock to protect shared config / 锁保护共享配置
    m_config = config;
    
    // Reset API Key list / 重置 API Key 列表
    m_apiKeys.clear();
    QStringList keys = m_config.api_key.split(',', Qt::SkipEmptyParts);
    for(const auto& k : keys) m_apiKeys.push_back(k.trimmed());
    m_currentKeyIndex = 0; // Reset index / 重置索引
    
    // Load glossary and regex / 如果开启了术语表，加载文件
    if (m_config.enable_glossary) {
        GlossaryManager::instance().setFilePath(m_config.glossary_path);
        RegexManager::instance().autoLoadFrom(m_config.glossary_path); 
    }
}

/**
 * @brief Starts the HTTP listener thread
 * @brief 启动 HTTP 监听线程
 */
void TranslationServer::startServer() {
    if (m_running) return;
    m_running = true;
    // Start runServerLoop in a new thread / 在新线程中启动 runServerLoop
    m_serverThread = new std::thread(&TranslationServer::runServerLoop, this);
    QString msg = QString(SV_LOG_START[m_config.language]).arg(m_config.port).arg(m_config.max_threads);
    emit logMessage(msg);
}

/**
 * @brief Stops the HTTP listener thread and cleans up resources
 * @brief 停止 HTTP 监听线程并清理资源
 */
void TranslationServer::stopServer() {
    if (!m_running) return;
    m_running = false;
    // Stop httplib server / 停止 httplib 服务器
    if (m_svr) m_svr->stop();
    // Wait for the thread to finish and clean up / 等待线程结束并回收资源
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
        delete m_serverThread;
        m_serverThread = nullptr;
    }
    delete m_svr;
    m_svr = nullptr;
    emit logMessage(SV_LOG_STOP[m_config.language]);
}

/**
 * @brief httplib Server Main Loop
 * @brief httplib 服务器主循环
 */
void TranslationServer::runServerLoop() {
    m_svr = new httplib::Server();
    int threads = m_config.max_threads;
    if (threads < 1) threads = 1;
    // Set thread pool size / 设置线程池大小
    m_svr->new_task_queue = [threads] { return new httplib::ThreadPool(threads); };

    // Define HTTP GET route / 定义 HTTP GET 路由
    m_svr->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        // Check for 'text' parameter / 检查 'text' 参数
        if (!req.has_param("text")) { res.set_content("", "text/plain"); return; }
        std::string text_std = req.get_param_value("text");
        QString text = QString::fromStdString(text_std).trimmed();
        if (text.isEmpty()) { res.set_content("", "text/plain; charset=utf-8"); return; }

        emit logMessage(QString(SV_LOG_REQ[m_config.language]) + text);
        
        // Execute core translation logic (includes retry) / 执行核心翻译逻辑（包含重试）
        QString result = performTranslation(text, QString::fromStdString(req.remote_addr));
        
        // Core Fix: Set HTTP status code based on result validity
        // 核心修复：根据结果是否为空来设置 HTTP 状态码
        if (result.isEmpty()) {
            res.status = 500; // Return 500 status code for failure / 返回 500 错误码，通知 XUnity 翻译失败
            res.set_content("Translation Failed", "text/plain"); 
        } else {
            // Return result with default 200 OK status / 返回结果给 XUnity，状态码默认为 200 (成功)
            res.set_content(result.toStdString(), "text/plain; charset=utf-8");
        }
    });
    
    // Start listening on port / 开始监听端口（阻塞调用）
    m_svr->listen("0.0.0.0", m_config.port);
}

/**
 * @brief Core translation function, includes retry logic
 * @brief 核心翻译函数，包含重试逻辑
 * @details Attempts translation up to MAX_RETRY_COUNT times
 * @details 尝试 MAX_RETRY_COUNT 次，直到成功或达到最大次数
 */
QString TranslationServer::performTranslation(const QString& text, const QString& clientIP) {
    QString resultText = "";
    int retryCount = 0;
    const int MAX_RETRY_COUNT = 5;
    const int RETRY_DELAY_MS = 1000;
    
    // Retry loop / 重试循环
    while (retryCount < MAX_RETRY_COUNT) {
        if (retryCount > 0) {
            // Log retry information / 记录重试信息
            QString retryMsg = QString(SV_RETRY_ATTEMPT[m_config.language])
                                  .arg(retryCount + 1)
                                  .arg(MAX_RETRY_COUNT) + text;
            emit logMessage(retryMsg);
            
            // Retry delay (blocks current thread) / 重试延迟（阻塞当前线程）
            QThread::msleep(RETRY_DELAY_MS);
        }
        
        // Perform a single translation attempt / 执行单次翻译尝试
        QString attemptResult = performSingleTranslationAttempt(text, clientIP);
        
        // Check if the result is valid / 检查结果是否有效
        if (isValidTranslationResult(attemptResult)) {
            if (retryCount > 0) {
                emit logMessage(SV_RETRY_SUCCESS[m_config.language]);
            }
            resultText = attemptResult;
            break; // Success, exit retry loop / 成功，退出重试循环
        }
        
        retryCount++;
        
        // If all retries failed / 如果所有重试都失败
        if (retryCount >= MAX_RETRY_COUNT) {
            emit logMessage(SV_RETRY_FAILED[m_config.language]);
            resultText = ""; // Ensure empty string is returned / 确保返回空字符串
        }
    }
    
    return resultText;
}

/**
 * @brief Helper function: Checks if the translation result is valid
 * @brief 辅助函数：检查翻译结果是否有效
 * @details Filters empty strings, "Error" strings, and common failure messages
 * @details 过滤空字符串、以 "Error" 开头的字符串以及常见的失败提示
 */
bool TranslationServer::isValidTranslationResult(const QString& result) {
    // Must not be empty, and must not start with "Error" (case insensitive)
    // 必须不为空，并且不以 "Error" 开头 (不区分大小写)
    // Also checks for common Chinese/English failure phrases / 检查中文/英文的失败提示
    return !result.isEmpty() && 
           !result.startsWith("Error", Qt::CaseInsensitive) &&
           !result.contains("翻译失败", Qt::CaseInsensitive) &&
           !result.contains("translation failed", Qt::CaseInsensitive) &&
           result.length() > 0;
}

/**
 * @brief Performs a single translation attempt (no retry)
 * @brief 执行单次翻译尝试（无重试）
 * @details Contains the core network request and parsing logic
 * @details 核心网络请求和解析逻辑
 */
QString TranslationServer::performSingleTranslationAttempt(const QString& text, const QString& clientIP) {
    // 1. Get API Key / 获取 API Key
    QString apiKey = getNextApiKey();
    if (apiKey.isEmpty()) {
        QString err = SV_ERR_KEY[m_config.language];
        emit logMessage("❌ " + err + " (No API Key Available)");
        return ""; // API Key error, return empty / API Key 错误，返回空
    }

    // 2. Regex Pre-processing / 正则预处理
    QString processedText = text;
    if (m_config.enable_glossary) {
        processedText = RegexManager::instance().processPre(text);
    }

    // Generate client ID for context management / 生成客户端 ID 用于上下文管理
    std::string clientId = generateClientId(clientIP.toStdString()).toStdString();
    
    QString finalSystemPrompt = m_config.system_prompt;
    bool performExtraction = false; // Flag to enable term extraction / 启用术语提取的标志

    // 3. RAG & Self-evolution Logic / RAG & 自进化逻辑 (Build glossary context and instructions)
    // 构建术语上下文和指令
    if (m_config.enable_glossary) {
        QString glossaryContext = GlossaryManager::instance().getContextPrompt(processedText);
        if (!glossaryContext.isEmpty()) {
            finalSystemPrompt += "\n\n" + glossaryContext;
        }

        // Randomly enable term extraction mode (approx 33% chance)
        // 随机启用术语提取模式 (约 33% 几率)
        if (processedText.length() > 8 && QRandomGenerator::global()->bounded(100) < 33) {
            performExtraction = true;
            finalSystemPrompt += "\n\n【Instruction】:\n"
                                 "1. Put translation in <tl>...</tl> tags.\n"
                                 "2. If you find NEW proper nouns (names, places) NOT in Known Terms, "
                                 "extract them in <tm>Original=Translated</tm> tags (one per line).\n"
                                 "3. Only extract proper nouns, NO verbs/common nouns.";
        }
    }

    // 4. Build Message History (Context Memory) / 构建消息历史 (上下文记忆)
    json messages = json::array();
    messages.push_back({{"role", "system"}, {"content", finalSystemPrompt.toStdString()}});

    // Context lock protection / 上下文锁保护
    std::lock_guard<std::mutex> lock(m_contextMutex);
    // Initialize context if not exists / 如果上下文不存在则初始化
    if (m_contexts.find(clientId) == m_contexts.end()) {
        m_contexts[clientId] = {std::deque<std::pair<QString, QString>>(), m_config.context_num};
    }
    Context& ctx = m_contexts[clientId];
    
    // Check and update max context length / 检查并更新上下文最大长度
    if (ctx.max_len != m_config.context_num) {
        ctx.max_len = m_config.context_num;
        while (ctx.history.size() > ctx.max_len) ctx.history.pop_front();
    }
    
    // Add history to the request / 将历史记录添加到请求中
    for (const auto& pair : ctx.history) {
        messages.push_back({{"role", "user"}, {"content", pair.first.toStdString()}});
        messages.push_back({{"role", "assistant"}, {"content", pair.second.toStdString()}});
    }

    QString currentUserContent = m_config.pre_prompt + processedText;
    messages.push_back({{"role", "user"}, {"content", currentUserContent.toStdString()}});

    // 5. Prepare API Request Payload / 准备 API 请求 Payload
    json payload;
    payload["model"] = m_config.model_name.toStdString();
    payload["messages"] = messages;
    payload["temperature"] = m_config.temperature;

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(m_config.api_address + "/chat/completions"));
    // Set headers / 设置头部
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    // Set timeout (Qt 6.x) / 设置超时 (Qt 6.x)
    request.setTransferTimeout(30000); // 30 seconds timeout / 30秒超时

    // 6. Send Request and Wait for Result (Using QEventLoop for synchronous call simulation)
    // 发送请求并等待结果 (使用 QEventLoop 模拟同步调用)
    QNetworkReply* reply = manager.post(request, QByteArray::fromStdString(payload.dump()));
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    
    // Connect timeout signal and finished signal / 连接超时信号，并连接请求完成信号
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    timer.start(30000); // Start 30 seconds timer / 启动 30 秒超时计时器
    loop.exec(); // Block and wait / 阻塞等待

    QString resultText = ""; // Translation result, default empty / 翻译结果，默认空

    // Check for timeout / 检查是否超时
    if (!timer.isActive()) {
        emit logMessage("❌ 请求超时 (Request Timeout)");
        reply->abort(); // Abort request / 终止请求
        reply->deleteLater();
        return ""; // Timeout returns empty / 超时返回空
    }
    timer.stop(); // Stop timer / 停止计时器

    // 7. Process Network Response / 处理网络响应
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseBytes = reply->readAll();
        try {
            json response = json::parse(responseBytes.toStdString());

            
            // Check for "usage" field to emit token usage / 检查 "usage" 字段以发送 Token 使用情况
            if (response.contains("usage")) {
                int p = response["usage"].value("prompt_tokens", 0);
                int c = response["usage"].value("completion_tokens", 0);
                if (p > 0 || c > 0) emit tokenUsageReceived(p, c);

            }

            // Check for "choices" field and if not empty / 检查是否存在 "choices" 字段且不为空
            if (response.contains("choices") && !response["choices"].empty()) {
                std::string content = response["choices"][0]["message"]["content"];
                QString rawContent = QString::fromStdString(content);

                // 8. Parse/Extract Result / 解析/提取结果
                if (performExtraction) {
                    // Extract <tl> tag content / 提取 <tl> 标签内容
                    QRegularExpression reTl("<tl>(.*?)</tl>", QRegularExpression::DotMatchesEverythingOption);
                    QRegularExpressionMatch matchTl = reTl.match(rawContent);
                    if (matchTl.hasMatch()) {
                        resultText = matchTl.captured(1).trimmed();
                    } else {
                        // Attempt cleaning if tag is missing / 尝试清洗非标签内容
                        resultText = rawContent;
                        resultText.remove(QRegularExpression("<[^>]*>")); // Remove all tags / 移除所有标签
                        emit logMessage(SV_WARN_TAG[m_config.language]); 
                    }

                    // Extract new terms <tm> / 提取新术语 <tm>
                    QRegularExpression reTm("<tm>(.*?)</tm>");
                    QRegularExpressionMatchIterator i = reTm.globalMatch(rawContent);
                    while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();
                        QString termLine = match.captured(1).trimmed();
                        int eqIdx = termLine.indexOf('=');
                        if (eqIdx > 0) {
                            QString k = termLine.left(eqIdx).trimmed();
                            QString v = termLine.mid(eqIdx + 1).trimmed();
                            // Only save if the original text contains the term / 只有原文包含该术语，才保存
                            if (processedText.contains(k, Qt::CaseInsensitive)) {
                                GlossaryManager::instance().addNewTerm(k, v);
                                emit logMessage(QString(SV_NEW_TERM[m_config.language]) + k + " = " + v);
                            }
                        }
                    }
                } else {
                    // Mode B: Normal translation (remove <think> tag) / 模式 B: 普通翻译（移除 <think> 标签）
                    resultText = rawContent;
                    std::regex think_regex("<think>.*?</think>", std::regex_constants::ECMAScript | std::regex_constants::icase);
                    std::string filtered = std::regex_replace(resultText.toStdString(), think_regex, "");
                    resultText = QString::fromStdString(filtered).trimmed();
                }

                // 9. Regex Post-processing / 正则后处理
                if (m_config.enable_glossary) {
                    resultText = RegexManager::instance().processPost(resultText);
                }

                emit logMessage("  -> " + resultText); 

                // Only save valid translation result to context / 只有通过校验的翻译结果才保存到上下文
                bool isValidResult = isValidTranslationResult(resultText);

                if (isValidResult) {
                    // Save to context history / 保存到上下文历史
                    ctx.history.push_back({currentUserContent, resultText});
                    while (ctx.history.size() > ctx.max_len) ctx.history.pop_front();
                } else {
                    // If result is invalid, force empty / 如果结果被判定为无效，强制清空，不返回
                    resultText = ""; 
                }
            } else {
                // Response JSON missing choices field (Format Error) / 响应 JSON 中缺少 choices 字段 (格式错误)
                QString err = SV_ERR_FMT[m_config.language];
                emit logMessage("❌ " + err + " (API Response: " + QString::fromStdString(responseBytes.toStdString()) + ")");
                resultText = ""; // Format error, return empty / 格式错误，返回空
            }
        } catch (const std::exception& e) {
            // JSON parsing exception / JSON 解析异常
            QString err = SV_ERR_JSON[m_config.language];
            emit logMessage("❌ " + err + " (Exception: " + QString(e.what()) + ")");
            resultText = ""; // JSON error, return empty / JSON 错误，返回空
        } catch (...) {
            // Other unknown parsing error / 其他未知解析错误
            QString err = SV_ERR_JSON[m_config.language];
            emit logMessage("❌ " + err + " (Unknown parsing error)");
            resultText = ""; // JSON error, return empty / JSON 错误，返回空
        }
    } else {
        // Network Error Handling (e.g., 429 Too Many Requests) / 网络错误处理 (如 429 Too Many Requests)
        QString errStr = reply->errorString();
        // Get HTTP status code / 获取 HTTP 状态码
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        QString errorMsg = "❌ 网络请求失败: ";
        if (httpStatus > 0) {
            errorMsg += QString("HTTP %1 - ").arg(httpStatus);
        }
        errorMsg += errStr;
        
        emit logMessage(errorMsg);
        resultText = ""; // Network error, return empty / 网络错误，返回空
    }

    reply->deleteLater();
    return resultText; // Return empty string to trigger retry or 500 status code / 返回空字符串以触发重试或 500 状态码
}

/**
 * @brief Get the next available API key (Round-Robin strategy)
 * @brief 获取下一个可用的 API 密钥 (轮询策略)
 */
QString TranslationServer::getNextApiKey() {
    std::lock_guard<std::mutex> lock(m_keyMutex); // Lock to protect key rotation / 锁保护密钥轮询
    if (m_apiKeys.empty()) return ""; // No keys, return empty / 没有密钥，返回空
    QString key = m_apiKeys[m_currentKeyIndex];
    // Circularly move index / 循环移动索引
    m_currentKeyIndex = (m_currentKeyIndex + 1) % m_apiKeys.size();
    return key;
}

/**
 * @brief Generate a simplified Client ID based on IP hash
 * @brief 基于 IP 地址哈希生成简化的客户端 ID
 */
QString TranslationServer::generateClientId(const std::string& ip) {
    // Hash IP using MD5 and take the first 8 hex characters / 使用 MD5 哈希 IP 并取前 8 位十六进制字符
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(ip), QCryptographicHash::Md5);
    return hash.toHex().left(8);
}