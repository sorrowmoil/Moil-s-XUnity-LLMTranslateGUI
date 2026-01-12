#pragma once
#include <QObject>

// Token 统计管理器 / Manages token usage statistics
class TokenManager : public QObject {
    Q_OBJECT

public:
    explicit TokenManager(QObject *parent = nullptr);

    // 增加计数 / Add usage
    void addUsage(long long prompt, long long completion);

    // 获取数据 / Getters
    long long getTotal() const { return m_totalTokens; }

    // 重置 / Reset
    void reset();

signals:
    // 通知 UI 更新 / Notify UI to update
    void tokensUpdated(long long total, long long prompt, long long completion);

private:
    long long m_promptTokens = 0;
    long long m_completionTokens = 0;
    long long m_totalTokens = 0;
};