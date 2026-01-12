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
// ✨ 新增：格式警告
const char* SV_WARN_TAG[] = {
    "⚠️ 格式警告：LLM 未返回 <tl> 标签，已自动清洗。",
    "⚠️ Format Warning: LLM missing <tl> tag, auto-cleaned."
};

TranslationServer::TranslationServer(QObject *parent) : QObject(parent), m_running(false) {}

TranslationServer::~TranslationServer() {
    stopServer();
}

void TranslationServer::updateConfig(const AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_keyMutex);
    m_config = config;
    
    // 重置 API Key 列表
    // Reset API Key list
    m_apiKeys.clear();
    QStringList keys = m_config.api_key.split(',', Qt::SkipEmptyParts);
    for(const auto& k : keys) m_apiKeys.push_back(k.trimmed());
    m_currentKeyIndex = 0;
    
    // 如果开启了术语表，加载文件
    // If glossary is enabled, load the file
    if (m_config.enable_glossary) {
        GlossaryManager::instance().setFilePath(m_config.glossary_path);
        RegexManager::instance().autoLoadFrom(m_config.glossary_path); // 同时也可能更新正则管理器
    }
}

void TranslationServer::startServer() {
    if (m_running) return;
    m_running = true;
    // 在新线程中启动 runServerLoop
    // Start runServerLoop in a new thread
    m_serverThread = new std::thread(&TranslationServer::runServerLoop, this);
    QString msg = QString(SV_LOG_START[m_config.language]).arg(m_config.port).arg(m_config.max_threads);
    emit logMessage(msg);
}

void TranslationServer::stopServer() {
    if (!m_running) return;
    m_running = false;
    // 停止 httplib 服务器
    // Stop httplib server
    if (m_svr) m_svr->stop();
    // 等待线程结束并回收资源
    // Wait for thread to finish and join
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
        delete m_serverThread;
        m_serverThread = nullptr;
    }
    delete m_svr;
    m_svr = nullptr;
    emit logMessage(SV_LOG_STOP[m_config.language]);
}

void TranslationServer::runServerLoop() {
    m_svr = new httplib::Server();
    int threads = m_config.max_threads;
    if (threads < 1) threads = 1;
    // 设置线程池大小
    // Set thread pool size
    m_svr->new_task_queue = [threads] { return new httplib::ThreadPool(threads); };

    // 定义 HTTP GET 路由
    // Define HTTP GET route
    m_svr->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        // XUnity 通常通过 GET 请求参数发送文本
        // XUnity usually sends text via GET request parameters
        if (!req.has_param("text")) { res.set_content("", "text/plain"); return; }
        std::string text_std = req.get_param_value("text");
        QString text = QString::fromStdString(text_std).trimmed();
        if (text.isEmpty()) { res.set_content("", "text/plain; charset=utf-8"); return; }

        emit logMessage(QString(SV_LOG_REQ[m_config.language]) + text);
        
        // 执行核心翻译逻辑
        // Execute core translation logic
        QString result = performTranslation(text, QString::fromStdString(req.remote_addr));
        
        // 返回结果给 XUnity
        // Return result to XUnity
        res.set_content(result.toStdString(), "text/plain; charset=utf-8");
    });
    
    // 开始监听端口（阻塞调用）
    // Start listening on port (blocking call)
    m_svr->listen("0.0.0.0", m_config.port);
}

QString TranslationServer::performTranslation(const QString& text, const QString& clientIP) {
    // 1. 获取 API Key
    QString apiKey = getNextApiKey();
    if (apiKey.isEmpty()) {
        QString err = SV_ERR_KEY[m_config.language];
        emit logMessage("❌ " + err); 
        return err;
    }

    // 2. 正则预处理 (Regex Pre-processing)
    QString processedText = text;
    if (m_config.enable_glossary) {
        // 在调用 LLM 前，先用本地正则修正一些已知错误
        // Use local regex to fix known errors before calling LLM
        processedText = RegexManager::instance().processPre(text);
    }

    std::string clientId = generateClientId(clientIP.toStdString()).toStdString();
    
    QString finalSystemPrompt = m_config.system_prompt;
    bool performExtraction = false; 

    // 3. RAG & 自进化逻辑 (RAG & Self-Evolution Logic)
    if (m_config.enable_glossary) {
        // 检索术语表
        // Retrieve glossary terms
        QString glossaryContext = GlossaryManager::instance().getContextPrompt(processedText);
        if (!glossaryContext.isEmpty()) {
            finalSystemPrompt += "\n\n" + glossaryContext;
        }

        // 随机触发术语提取 (33% 概率)
        // Randomly trigger term extraction (33% chance)
        // 只有文本较长时才触发，避免短句误判
        if (processedText.length() > 8 && QRandomGenerator::global()->bounded(100) < 33) {
            performExtraction = true;
            // 注入指令：要求 LLM 将翻译放在 <tl> 中，新术语放在 <tm> 中
            // Inject instructions: Ask LLM to put translation in <tl> and new terms in <tm>
            finalSystemPrompt += "\n\n【Instruction】:\n"
                                 "1. Put translation in <tl>...</tl> tags.\n"
                                 "2. If you find NEW proper nouns (names, places) NOT in Known Terms, "
                                 "extract them in <tm>Original=Translated</tm> tags (one per line).\n"
                                 "3. Only extract proper nouns, NO verbs/common nouns.";
        }
    }

    // 4. 构建消息历史 (Build Message History)
    json messages = json::array();
    messages.push_back({{"role", "system"}, {"content", finalSystemPrompt.toStdString()}});

    // 锁住上下文 Map
    // Lock context map
    std::lock_guard<std::mutex> lock(m_contextMutex);
    if (m_contexts.find(clientId) == m_contexts.end()) {
        m_contexts[clientId] = {std::deque<std::pair<QString, QString>>(), m_config.context_num};
    }
    Context& ctx = m_contexts[clientId];
    
    // 动态调整上下文长度配置
    if (ctx.max_len != m_config.context_num) {
        ctx.max_len = m_config.context_num;
        while (ctx.history.size() > ctx.max_len) ctx.history.pop_front();
    }
    
    // 添加历史记录
    // Add history records
    for (const auto& pair : ctx.history) {
        messages.push_back({{"role", "user"}, {"content", pair.first.toStdString()}});
        messages.push_back({{"role", "assistant"}, {"content", pair.second.toStdString()}});
    }

    QString currentUserContent = m_config.pre_prompt + processedText;
    messages.push_back({{"role", "user"}, {"content", currentUserContent.toStdString()}});

    // 5. 准备 API 请求 (Prepare API Request)
    json payload;
    payload["model"] = m_config.model_name.toStdString();
    payload["messages"] = messages;
    payload["temperature"] = m_config.temperature;

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(m_config.api_address + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    // 6. 发送请求并等待结果 (Sync wait for Async Qt)
    // 注意：因为我们在子线程中，不能直接依赖 Qt 主事件循环，所以需要局部 QEventLoop
    // Note: Since we are in a worker thread, we use local QEventLoop to wait for async reply
    QNetworkReply* reply = manager.post(request, QByteArray::fromStdString(payload.dump()));
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec(); // 阻塞直到请求完成 / Block until request finishes

    QString resultText;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseBytes = reply->readAll();
        try {
            json response = json::parse(responseBytes.toStdString());
            if (response.contains("choices") && !response["choices"].empty()) {
                std::string content = response["choices"][0]["message"]["content"];
                QString rawContent = QString::fromStdString(content);

                // 7. 解析结果 (Parse Result)
                if (performExtraction) {
                    // 模式 A: 提取 XML 标签内容
                    // Mode A: Extract XML tag content
                    QRegularExpression reTl("<tl>(.*?)</tl>", QRegularExpression::DotMatchesEverythingOption);
                    QRegularExpressionMatch matchTl = reTl.match(rawContent);
                    if (matchTl.hasMatch()) {
                        resultText = matchTl.captured(1).trimmed();
                    } else {
                        // ⚠️ 格式警告：LLM 未遵循指令，尝试清理标签直接使用
                        // Warning: LLM ignored instruction, try to clean tags and use directly
                        resultText = rawContent;
                        resultText.remove(QRegularExpression("<[^>]*>"));
                        emit logMessage(SV_WARN_TAG[m_config.language]); 
                    }

                    // 提取新术语并保存
                    // Extract new terms and save
                    QRegularExpression reTm("<tm>(.*?)</tm>");
                    QRegularExpressionMatchIterator i = reTm.globalMatch(rawContent);
                    while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();
                        QString termLine = match.captured(1).trimmed();
                        int eqIdx = termLine.indexOf('=');
                        if (eqIdx > 0) {
                            QString k = termLine.left(eqIdx).trimmed();
                            QString v = termLine.mid(eqIdx + 1).trimmed();
                            // 再次确认原文中是否真的包含这个词，防止幻觉
                            if (processedText.contains(k, Qt::CaseInsensitive)) {
                                GlossaryManager::instance().addNewTerm(k, v);
                                emit logMessage(QString(SV_NEW_TERM[m_config.language]) + k + " = " + v);
                            }
                        }
                    }
                } else {
                    // 模式 B: 普通翻译
                    // Mode B: Normal translation
                    resultText = rawContent;
                    
                    // ✨ 特性支持：移除 DeepSeek 等推理模型的 <think> 过程
                    // Feature: Remove <think> process for reasoning models like DeepSeek
                    std::regex think_regex("<think>.*?</think>", std::regex_constants::ECMAScript | std::regex_constants::icase);
                    std::string filtered = std::regex_replace(resultText.toStdString(), think_regex, "");
                    resultText = QString::fromStdString(filtered).trimmed();
                }

                // 8. 正则后处理 (Regex Post-processing)
                if (m_config.enable_glossary) {
                    resultText = RegexManager::instance().processPost(resultText);
                }

                emit logMessage("  -> " + resultText); 

                // 更新历史记录
                // Update history
                ctx.history.push_back({currentUserContent, resultText});
                while (ctx.history.size() > ctx.max_len) ctx.history.pop_front();
            } else {
                resultText = SV_ERR_FMT[m_config.language];
                emit logMessage("❌ " + resultText + " (API Response: " + QString::fromStdString(responseBytes.toStdString()) + ")");
            }
        } catch (...) {
            resultText = SV_ERR_JSON[m_config.language];
            emit logMessage("❌ " + resultText + " (Raw: " + QString::fromStdString(responseBytes.toStdString()) + ")");
        }
    } else {
        resultText = "Error: " + reply->errorString();
        emit logMessage("❌ 网络请求失败: " + resultText);
    }

    reply->deleteLater();
    return resultText;
}

QString TranslationServer::getNextApiKey() {
    std::lock_guard<std::mutex> lock(m_keyMutex);
    if (m_apiKeys.empty()) return "";
    QString key = m_apiKeys[m_currentKeyIndex];
    // 循环移动索引
    // Circularly move index
    m_currentKeyIndex = (m_currentKeyIndex + 1) % m_apiKeys.size();
    return key;
}

QString TranslationServer::generateClientId(const std::string& ip) {
    // 使用 MD5 简化 IP 地址，保护隐私并缩短 ID
    // Use MD5 to simplify IP address, protect privacy and shorten ID
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(ip), QCryptographicHash::Md5);
    return hash.toHex().left(8);
}

