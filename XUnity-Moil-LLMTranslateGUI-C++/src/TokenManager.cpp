#include "TokenManager.h"

TokenManager::TokenManager(QObject *parent) : QObject(parent) {
}

void TokenManager::addUsage(long long prompt, long long completion) {
    m_promptTokens += prompt;
    m_completionTokens += completion;
    m_totalTokens = m_promptTokens + m_completionTokens;

    // 发出信号 / Emit signal
    emit tokensUpdated(m_totalTokens, m_promptTokens, m_completionTokens);
}

void TokenManager::reset() {
    m_promptTokens = 0;
    m_completionTokens = 0;
    m_totalTokens = 0;
    emit tokensUpdated(0, 0, 0);
}