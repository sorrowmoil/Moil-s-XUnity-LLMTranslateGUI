#include "TokenManager.h"

/**
 * Constructor for TokenManager.
 * TokenManager的构造函数。
 * 
 * Initializes the token counters to zero and sets up the parent QObject.
 * 初始化令牌计数器为零，并设置父QObject。
 * 
 * @param parent Parent object (for Qt ownership).
 *              父对象（用于Qt所有权管理）。
 */
TokenManager::TokenManager(QObject *parent) : QObject(parent) {
}

/**
 * Add usage of tokens for a single request/response.
 * 添加单次请求/响应的令牌使用量。
 * 
 * This method accumulates prompt and completion tokens, updates the total,
 * and emits the tokensUpdated signal with the new totals.
 * 该方法累加提示令牌和完成令牌，更新总数，并发射带有新总数的tokensUpdated信号。
 * 
 * @param prompt    Number of tokens used in the prompt (input).
 *                  提示（输入）中使用的令牌数。
 * @param completion Number of tokens used in the completion (output).
 *                   完成（输出）中使用的令牌数。
 */
void TokenManager::addUsage(long long prompt, long long completion) {
    m_promptTokens += prompt;
    m_completionTokens += completion;
    m_totalTokens = m_promptTokens + m_completionTokens;

    // Emit signal with updated totals.
    // 发射带有更新后总数的信号。
    emit tokensUpdated(m_totalTokens, m_promptTokens, m_completionTokens);
}

/**
 * Reset all token counters to zero.
 * 将所有令牌计数器重置为零。
 * 
 * Useful when starting a new session or after a configuration change.
 * 在开始新会话或配置更改后使用。
 */
void TokenManager::reset() {
    m_promptTokens = 0;
    m_completionTokens = 0;
    m_totalTokens = 0;
    emit tokensUpdated(0, 0, 0);
}