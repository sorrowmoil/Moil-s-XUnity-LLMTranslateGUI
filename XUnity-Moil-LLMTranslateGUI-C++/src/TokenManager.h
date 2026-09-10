#pragma once

#include <QObject>
#include <QReadWriteLock>

class TranslationServer;

// 避开 Windows SDK 中的 TokenStatistics 枚举常量。
struct TokenUsageSnapshot {
    long long promptTokens = 0;
    long long completionTokens = 0;
    long long totalTokens = 0;
    long long reportedResponses = 0;      // 成功读取到供应商 usage 信息的响应数量
    long long unreportedResponses = 0;    // 请求成功返回，但供应商没有提供 usage 的响应数量
};

class TokenManager final : public QObject {
    Q_OBJECT

public:
    static TokenManager &instance();
    ~TokenManager() override = default;

    // 接入 TranslationServer，使用 UniqueConnection 避免重复连接和重复累计。
    void attachTo(TranslationServer &server);

    TokenUsageSnapshot snapshot() const;
    long long getPrompt() const;
    long long getCompletion() const;
    long long getTotal() const;

public slots:
    void addUsage(long long prompt, long long completion, long long providerTotal);
    void recordUnreportedResponse();
    void reset();
    void publishSnapshot();

signals:
    void tokensUpdated(long long total, long long prompt, long long completion);
    void coverageUpdated(long long reportedResponses, long long unreportedResponses);

private:
    explicit TokenManager(QObject *parent = nullptr);
    static long long saturatingAdd(long long current, long long increment);

    mutable QReadWriteLock m_lock;
    TokenUsageSnapshot m_statistics;

    Q_DISABLE_COPY(TokenManager)
};