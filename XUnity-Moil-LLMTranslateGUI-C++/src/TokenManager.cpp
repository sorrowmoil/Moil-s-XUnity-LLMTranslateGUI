#include "TokenManager.h"
#include "TranslationServer.h"

#include <QReadLocker>
#include <QWriteLocker>

#include <algorithm>
#include <limits>

TokenManager::TokenManager(QObject *parent) : QObject(parent) { }

TokenManager &TokenManager::instance() { static TokenManager manager; return manager; }

void TokenManager::attachTo(TranslationServer &server)
{
    const auto directUnique = static_cast<Qt::ConnectionType>(static_cast<int>(Qt::DirectConnection) | static_cast<int>(Qt::UniqueConnection));
    connect(&server, &TranslationServer::tokenUsageReceived, this, &TokenManager::addUsage, directUnique);
    connect(&server, &TranslationServer::tokenUsageUnavailable, this, &TokenManager::recordUnreportedResponse, directUnique);
}

long long TokenManager::saturatingAdd(long long current, long long increment)
{
    if (increment <= 0) return current;
    const long long maximum = std::numeric_limits<long long>::max();
    if (current > maximum - increment) return maximum;
    return current + increment;
}

TokenUsageSnapshot TokenManager::snapshot() const { QReadLocker locker(&m_lock); return m_statistics; }

long long TokenManager::getPrompt() const { return snapshot().promptTokens; }

long long TokenManager::getCompletion() const { return snapshot().completionTokens; }

long long TokenManager::getTotal() const { return snapshot().totalTokens; }

void TokenManager::addUsage(long long prompt, long long completion, long long providerTotal)
{
    prompt = std::max(0LL, prompt);
    completion = std::max(0LL, completion);
    const long long componentTotal = saturatingAdd(prompt, completion);
    const long long effectiveTotal = providerTotal >= 0 ? std::max(providerTotal, componentTotal) : componentTotal;

    TokenUsageSnapshot current;
    {
        QWriteLocker locker(&m_lock);
        m_statistics.promptTokens = saturatingAdd(m_statistics.promptTokens, prompt);
        m_statistics.completionTokens = saturatingAdd(m_statistics.completionTokens, completion);
        m_statistics.totalTokens = saturatingAdd(m_statistics.totalTokens, effectiveTotal);
        m_statistics.reportedResponses = saturatingAdd(m_statistics.reportedResponses, 1);
        current = m_statistics;
    }

    emit tokensUpdated(current.totalTokens, current.promptTokens, current.completionTokens);
    emit coverageUpdated(current.reportedResponses, current.unreportedResponses);
}

void TokenManager::recordUnreportedResponse()
{
    TokenUsageSnapshot current;
    {
        QWriteLocker locker(&m_lock);
        m_statistics.unreportedResponses = saturatingAdd(m_statistics.unreportedResponses, 1);
        current = m_statistics;
    }

    emit tokensUpdated(current.totalTokens, current.promptTokens, current.completionTokens);
    emit coverageUpdated(current.reportedResponses, current.unreportedResponses);
}

void TokenManager::reset()
{
    {
        QWriteLocker locker(&m_lock);
        m_statistics = TokenUsageSnapshot{};
    }
    emit tokensUpdated(0, 0, 0);
    emit coverageUpdated(0, 0);
}

void TokenManager::publishSnapshot()
{
    const TokenUsageSnapshot current = snapshot();
    emit tokensUpdated(current.totalTokens, current.promptTokens, current.completionTokens);
    emit coverageUpdated(current.reportedResponses, current.unreportedResponses);
}