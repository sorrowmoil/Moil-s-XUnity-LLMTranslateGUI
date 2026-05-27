#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>
#include <deque>
#include <QDebug>

// 定义最大日志保留行数，防止内存溢出
#define MAX_LOG_HISTORY 3000

/**
 * LogManager - 中央日志管理器 (单例模式)
 * 作用：作为所有模块(Server/MainWindow/ModernWindow)的日志中转站。
 * 特性：线程安全、防止死循环、自动修剪旧日志。
 */
class LogManager : public QObject {
    Q_OBJECT

public:
    // 获取单例实例
    static LogManager& instance() {
        static LogManager _instance;
        return _instance;
    }

    /**
     * 添加日志 (线程安全)
     * 所有组件调用此函数来记录日志，而不是直接 emit 信号。
     */
    void addLog(const QString& msg) {
        QMutexLocker locker(&m_mutex);

        // 1. 存入缓冲区
        m_history.push_back(msg);

        // 2. 自动修剪 (保持缓冲区大小)
        if (m_history.size() > MAX_LOG_HISTORY) {
            m_history.pop_front();
        }

        // 3. 发送信号通知 UI 更新
        // 注意：信号是在锁释放后发送吗？不，在 Qt 中 emit 是瞬间的。
        // 为了防止死锁（如果槽函数里又调用 addLog），我们使用 Qt::QueuedConnection 或者依赖 Qt 的信号机制处理。
        // 但最安全的方式是：确保 UI 的槽函数只是“显示”，绝对不要在槽函数里再次调用 addLog。
        emit newLogAvailable(msg);
    }

    /**
     * 获取完整历史记录 (用于窗口初始化/切换时恢复)
     * 返回副本以保证线程安全
     */
    QStringList getHistory() {
        QMutexLocker locker(&m_mutex);
        QStringList list;
        for (const auto& line : m_history) {
            list << line;
        }
        return list;
    }

    /**
     * 清空日志
     */
    void clear() {
        QMutexLocker locker(&m_mutex);
        m_history.clear();
        emit logsCleared();
    }

signals:
    // 通知 UI 有新日志 (建议 UI 使用 Qt::QueuedConnection 连接此信号，虽然默认 Auto 也可以)
    void newLogAvailable(const QString& msg);
    
    // 通知 UI 清空屏幕
    void logsCleared();

private:
    // 私有构造，强制单例
    LogManager() {}
    ~LogManager() {}
    
    // 禁止拷贝
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    std::deque<QString> m_history;
    mutable QMutex m_mutex; // 保护 history 的互斥锁
};

// 方便的宏定义，让调用更简单
// 用法: LOG("Server started");
#define LOG(msg) LogManager::instance().addLog(msg)