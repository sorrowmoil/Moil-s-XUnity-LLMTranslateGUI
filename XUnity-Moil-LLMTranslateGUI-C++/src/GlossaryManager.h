#pragma once
#include <QString>
#include <QMap>
#include <QReadWriteLock>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

// 术语表管理器类，负责加载、查询和更新翻译术语
// GlossaryManager class, responsible for loading, querying, and updating translation terms
class GlossaryManager {
public:
    // 获取单例实例
    // Get the singleton instance
    static GlossaryManager& instance() {
        static GlossaryManager instance;
        return instance;
    }

    // 设置文件路径并加载术语
    // Set file path and load terms
    void setFilePath(const QString& path) {
        // 使用写锁，确保在加载过程中独占访问
        // Use write lock to ensure exclusive access during loading
        QWriteLocker locker(&m_lock);
        m_filePath = path;
        loadTerms();
    }

    // 获取当前上下文相关的术语 (RAG 核心功能)
    // Get terms relevant to the current context (RAG Core function)
    QString getContextPrompt(const QString& text) {
        // 使用读锁，允许多个线程同时查询
        // Use read lock to allow multiple threads to query simultaneously
        QReadLocker locker(&m_lock);
        if (m_terms.isEmpty()) return "";

        QStringList foundTerms;
        // 简单遍历匹配 (对于几千条数据，C++ 处理极快)
        // Simple iteration match (C++ handles thousands of entries very quickly)
        QMapIterator<QString, QString> i(m_terms);
        while (i.hasNext()) {
            i.next();
            // 如果原文包含这个 Key (不区分大小写)
            // If the original text contains this Key (case-insensitive)
            if (text.contains(i.key(), Qt::CaseInsensitive)) {
                // 将匹配到的术语格式化为 "原文 = 译文"
                // Format the matched term as "Original = Translated"
                foundTerms << (i.key() + " = " + i.value());
            }
        }

        if (foundTerms.isEmpty()) return "";
        
        // 返回格式化的术语表提示词
        // Return the formatted glossary prompt
        return "【已知术语/Known Terms】:\n" + foundTerms.join("\n") + "\n";
    }

    // 添加新术语 (自进化/学习核心)
    // Add new term (Self-evolution/learning Core)
    void addNewTerm(const QString& key, const QString& value) {
        // 需要修改数据，使用写锁
        // Need to modify data, use write lock
        QWriteLocker locker(&m_lock);
        
        // 基础过滤：防止脏数据
        // Basic filtering: Prevent dirty data
        
        // 长度检查：Key 至少2个字符，Value 至少1个字符
        // Length check: Key at least 2 chars, Value at least 1 char
        if (key.length() < 2 || value.length() < 1) return;
        
        // 防止重复添加
        // Prevent duplicate additions
        if (m_terms.contains(key)) return; 
        
        // 格式检查：防止包含等号，破坏文件格式
        // Format check: Prevent containing equals sign, which breaks file format
        if (key.contains("=") || value.contains("=")) return; 
        
        // 防止包含换行符
        // Prevent containing newlines
        if (key.contains("\n") || value.contains("\n")) return;

        // 更新内存中的 Map
        // Update the Map in memory
        m_terms.insert(key, value);
        // 追加写入到文件
        // Append to file
        appendToFile(key, value);
    }

private:
    // 私有构造函数 (单例模式)
    // Private constructor (Singleton pattern)
    GlossaryManager() {}
    
    // 从文件加载术语到内存
    // Load terms from file into memory
    void loadTerms() {
        m_terms.clear();
        if (m_filePath.isEmpty()) return;

        QFile file(m_filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setEncoding(QStringConverter::Utf8); // 假设文件是 UTF-8 / Assume file is UTF-8
            while (!in.atEnd()) {
                QString line = in.readLine();
                // XUnity 格式通常是 Original=Translated
                // XUnity format is typically Original=Translated
                int idx = line.indexOf('=');
                if (idx > 0) {
                    QString key = line.left(idx).trimmed();
                    QString val = line.mid(idx + 1).trimmed();
                    // 确保键值都不为空
                    // Ensure both key and value are not empty
                    if (!key.isEmpty() && !val.isEmpty()) {
                        m_terms.insert(key, val);
                    }
                }
            }
        }
    }

    // 将单个术语追加到文件末尾
    // Append a single term to the end of the file
    void appendToFile(const QString& key, const QString& value) {
        if (m_filePath.isEmpty()) return;
        QFile file(m_filePath);
        // 以追加模式打开文件
        // Open file in append mode
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << key << "=" << value << "\n";
        }
    }

    QString m_filePath;
    QMap<QString, QString> m_terms;
    // 读写锁，保护 m_terms 和文件写入操作
    // Read-write lock to protect m_terms and file write operations
    mutable QReadWriteLock m_lock;
};