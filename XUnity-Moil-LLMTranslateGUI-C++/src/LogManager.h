#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>
#include <deque>
#include <QDebug>

// Maximum number of log lines to keep in history to prevent memory overflow.
// 最大日志保留行数，防止内存溢出。
#define MAX_LOG_HISTORY 3000

/**
 * LogManager - Central log manager (singleton pattern)
 * 中央日志管理器（单例模式）
 *
 * Acts as a central hub for log messages from all modules (Server, MainWindow, ModernWindow).
 * Provides thread‑safe logging, automatic history trimming, and signals to update UI components.
 * 作为所有模块（Server、MainWindow、ModernWindow）的日志中转站。
 * 提供线程安全的日志记录、自动历史修剪以及更新UI组件的信号。
 */
class LogManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Get the singleton instance.
     * 获取单例实例。
     *
     * @return Reference to the unique LogManager instance.
     *         唯一的LogManager实例的引用。
     */
    static LogManager &instance()
    {
        static LogManager _instance;
        return _instance;
    }

    /**
     * Add a log message (thread‑safe).
     * 添加日志（线程安全）。
     *
     * All components call this function to record logs, instead of emitting signals directly.
     * The message is stored in a history buffer, the buffer is trimmed if it exceeds MAX_LOG_HISTORY,
     * and a signal newLogAvailable is emitted to notify any UI.
     * 所有组件调用此函数来记录日志，而不是直接发射信号。
     * 消息存入历史缓冲区，若缓冲区超过MAX_LOG_HISTORY则自动修剪，
     * 并发射newLogAvailable信号通知UI更新。
     *
     * @param msg The log message to add. / 要添加的日志消息。
     */
    void addLog(const QString &msg)
    {
        QMutexLocker locker(&m_mutex); // Lock to protect the history deque / 加锁保护历史双端队列

        // 1. Store the message in the history buffer.
        // 1. 将消息存入历史缓冲区。
        m_history.push_back(msg);

        // 2. Trim the buffer if it exceeds the maximum size.
        // 2. 若缓冲区超出最大大小则修剪。
        if (m_history.size() > MAX_LOG_HISTORY)
        {
            m_history.pop_front();
        }

        // 3. Emit a signal to notify any connected UI.
        // 3. 发射信号通知连接的UI。
        // Note: The signal is emitted while still holding the lock, but it is safe as long as
        //       the connected slot does not call back into addLog (which would deadlock).
        //       The typical UI slot only appends the message to a text display and does not
        //       call addLog again.
        // 注意：信号在持有锁时发射，但只要连接的槽函数不回调addLog（否则会导致死锁）就是安全的。
        //      典型的UI槽函数仅将消息追加到文本显示区，不会再次调用addLog。
        emit newLogAvailable(msg);
    }

    /**
     * Get a copy of the complete log history.
     * 获取完整历史记录的副本。
     *
     * Used when initializing or switching windows to restore the log display.
     * Returns a copy to ensure thread safety.
     * 用于窗口初始化或切换时恢复日志显示。
     * 返回副本以保证线程安全。
     *
     * @return QStringList containing all stored log lines.
     *         包含所有存储日志行的QStringList。
     */
    QStringList getHistory()
    {
        QMutexLocker locker(&m_mutex); // Lock to protect while copying / 加锁保护复制过程
        QStringList list;
        for (const auto &line : m_history)
        {
            list << line;
        }
        return list;
    }

    /**
     * Clear all logs from the history buffer.
     * 清空历史缓冲区中的所有日志。
     *
     * Emits logsCleared signal to inform UI to clear its display.
     * 发射logsCleared信号通知UI清空显示。
     */
    void clear()
    {
        QMutexLocker locker(&m_mutex); // Lock to protect modification / 加锁保护修改
        m_history.clear();
        emit logsCleared();
    }

signals:
    /**
     * Signal emitted when a new log message is available.
     * 当有新日志消息可用时发射的信号。
     *
     * It is recommended that UI components connect to this signal using Qt::QueuedConnection,
     * although the default auto connection also works. The connected slot must not call
     * addLog again to avoid deadlocks.
     * 建议UI组件使用Qt::QueuedConnection连接此信号，虽然默认的自动连接也能工作。
     * 连接的槽函数绝不能再次调用addLog，以免死锁。
     *
     * @param msg The new log message. / 新日志消息。
     */
    void newLogAvailable(const QString &msg);

    /**
     * Signal emitted when the log history has been cleared.
     * 当日志历史被清空时发射的信号。
     *
     * UI components can connect to this to clear their display.
     * UI组件可连接此信号以清空其显示。
     */
    void logsCleared();

private:
    // Private constructor to enforce singleton.
    // 私有构造函数，强制单例。
    LogManager() {}
    // Private destructor (singleton manages its own lifetime).
    // 私有析构函数（单例管理自身生命周期）。
    ~LogManager() {}

    // Disable copy constructor and assignment operator.
    // 禁止拷贝构造函数和赋值操作符。
    LogManager(const LogManager &) = delete;
    LogManager &operator=(const LogManager &) = delete;

    std::deque<QString> m_history; ///< History buffer storing log lines. / 存储日志行的历史缓冲区。
    mutable QMutex m_mutex;        ///< Mutex protecting access to m_history. / 保护m_history访问的互斥锁。
};

/**
 * Convenience macro for simpler logging.
 * 方便日志记录的宏，简化调用。
 *
 * Usage: LOG("Server started");
 * 用法：LOG("Server started");
 */
#define LOG(msg) LogManager::instance().addLog(msg)