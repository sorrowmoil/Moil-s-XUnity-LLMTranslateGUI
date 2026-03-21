#pragma once
#include <QString>
#include <QMap>
#include <QReadWriteLock>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

/**
 * Glossary manager class, responsible for loading, querying, and updating translation terms.
 * 术语表管理器类，负责加载、查询和更新翻译术语。
 * 
 * This class implements a singleton pattern. It maintains an in‑memory map of terms
 * (original → translation) loaded from a glossary file. It also provides thread‑safe
 * methods for querying terms relevant to a given text and for adding new terms.
 * 该类实现单例模式。它维护一个从术语文件加载的内存术语映射（原文 → 译文），
 * 并提供线程安全的方法来查询与给定文本相关的术语以及添加新术语。
 */
class GlossaryManager {
public:
    /**
     * Get the singleton instance.
     * 获取单例实例。
     * 
     * @return Reference to the unique GlossaryManager instance ; 唯一的 GlossaryManager 实例的引用
     */
    static GlossaryManager& instance() {
        static GlossaryManager instance;
        return instance;
    }

    /**
     * Set the glossary file path and load terms from it.
     * 设置术语表文件路径并从中加载术语。
     * 
     * This method acquires a write lock to ensure exclusive access while loading.
     * If the file path is empty or the file cannot be opened, the internal term map remains empty.
     * 该方法在加载期间获取写锁以确保独占访问。
     * 如果文件路径为空或文件无法打开，内部术语映射将保持为空。
     * 
     * @param path Path to the glossary file (e.g., "glossary.txt") ; 术语表文件路径（例如 "glossary.txt"）
     */
    void setFilePath(const QString& path) {
        QWriteLocker locker(&m_lock);          // Write lock for exclusive modification ; 写锁，用于独占修改
        m_filePath = path;
        loadTerms();
    }

    /**
     * Retrieve terms relevant to the given input text.
     * 获取与给定输入文本相关的术语。
     * 
     * This method performs a case‑insensitive scan of the term map and returns a formatted
     * string containing all terms whose original text appears in the input.
     * The read lock allows multiple threads to query concurrently.
     * 该方法对术语映射执行不区分大小写的扫描，并返回一个格式化字符串，
     * 其中包含所有原文出现在输入中的术语。
     * 读锁允许多个线程同时查询。
     * 
     * @param text Input text to match against term keys ; 用于匹配术语原文的输入文本
     * @return Formatted glossary prompt (empty string if no matches) ; 格式化的术语提示（若无匹配则返回空字符串）
     */
    QString getContextPrompt(const QString& text) {
        QReadLocker locker(&m_lock);            // Read lock for concurrent queries ; 读锁，允许并发查询
        if (m_terms.isEmpty()) return "";

        QStringList foundTerms;
        // Iterate through the term map and check if the key appears in the input text.
        // 遍历术语映射，检查原文键是否出现在输入文本中。
        QMapIterator<QString, QString> i(m_terms);
        while (i.hasNext()) {
            i.next();
            if (text.contains(i.key(), Qt::CaseInsensitive)) {
                // Format as "original = translation" ; 格式化为 "原文 = 译文"
                foundTerms << (i.key() + " = " + i.value());
            }
        }

        if (foundTerms.isEmpty()) return "";
        
        // Return a prompt section containing all matched terms ; 返回包含所有匹配术语的提示片段
        return "【已知术语/Known Terms】:\n" + foundTerms.join("\n") + "\n";
    }

    /**
     * Add a new term to the glossary (both in memory and persistently).
     * 向术语表添加新术语（同时更新内存和持久化文件）。
     * 
     * This method validates the term before insertion: length checks, duplicate prevention,
     * and format restrictions (no equals sign or newline). If valid, it inserts the term
     * into the in‑memory map and appends it to the glossary file.
     * A write lock is used to protect both data structures.
     * 该方法在插入前对术语进行验证：长度检查、防止重复以及格式限制（无等号或换行符）。
     * 如果有效，则将术语插入内存映射并追加到术语表文件。
     * 使用写锁保护两个数据结构。
     * 
     * @param key   Original text (source language) ; 原文（源语言）
     * @param value Translated text (target language) ; 译文（目标语言）
     */
    void addNewTerm(const QString& key, const QString& value) {
        QWriteLocker locker(&m_lock);            // Write lock for modification ; 写锁，用于修改

        // Basic validation to maintain data integrity ; 基本验证以保持数据完整性
        // Length checks: key at least 2 characters, value at least 1 character ; 长度检查：键至少2个字符，值至少1个字符
        if (key.length() < 2 || value.length() < 1) return;
        // Prevent duplicate entries ; 防止重复条目
        if (m_terms.contains(key)) return;
        // Avoid breaking the file format (no equals sign in key/value) ; 避免破坏文件格式（键/值中不能有等号）
        if (key.contains("=") || value.contains("=")) return;
        // Avoid newlines that would corrupt the line‑based storage ; 避免换行符破坏基于行的存储
        if (key.contains("\n") || value.contains("\n")) return;

        // Update the in‑memory map ; 更新内存映射
        m_terms.insert(key, value);
        // Persist the new term by appending to the file ; 通过追加到文件来持久化新术语
        appendToFile(key, value);
    }

private:
    // Private constructor for singleton ; 单例模式的私有构造函数
    GlossaryManager() {}

    /**
     * Load all terms from the glossary file into the in‑memory map.
     * 从术语表文件加载所有术语到内存映射。
     * 
     * The file is expected to be UTF‑8 encoded with one term per line in the format:
     *   Original=Translated
     * Lines not matching this pattern are silently ignored.
     * 期望文件为 UTF‑8 编码，每行一个术语，格式为：
     *   原文=译文
     * 不符合此模式的行将被静默忽略。
     */
    void loadTerms() {
        m_terms.clear();
        if (m_filePath.isEmpty()) return;

        QFile file(m_filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setEncoding(QStringConverter::Utf8);   // Assume UTF‑8 encoding ; 假设 UTF‑8 编码
            while (!in.atEnd()) {
                QString line = in.readLine();
                // Find the first equals sign separating key and value ; 找到分隔键和值的第一个等号
                int idx = line.indexOf('=');
                if (idx > 0) {
                    QString key = line.left(idx).trimmed();
                    QString val = line.mid(idx + 1).trimmed();
                    // Ensure both parts are non‑empty ; 确保两部分均非空
                    if (!key.isEmpty() && !val.isEmpty()) {
                        m_terms.insert(key, val);
                    }
                }
            }
        }
    }

    /**
     * Append a single term to the glossary file.
     * 向术语表文件追加单个术语。
     * 
     * The term is written as a new line in the format "key=value". The file is opened in append mode,
     * so existing content is preserved.
     * 术语以新行的形式写入，格式为“key=value”。文件以追加模式打开，因此现有内容得以保留。
     * 
     * @param key   Original text ; 原文
     * @param value Translated text ; 译文
     */
    void appendToFile(const QString& key, const QString& value) {
        if (m_filePath.isEmpty()) return;
        QFile file(m_filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << key << "=" << value << "\n";
        }
    }

    QString m_filePath;                     // Path to the glossary file ; 术语表文件路径
    QMap<QString, QString> m_terms;          // In‑memory map of terms (original → translation) ; 内存术语映射（原文 → 译文）

    /**
     * Read‑write lock to protect concurrent access to m_terms and file operations.
     * 读写锁，用于保护对 m_terms 和文件操作的并发访问。
     * 
     * Methods that only read (e.g., getContextPrompt) acquire a read lock, while methods
     * that modify (setFilePath, addNewTerm) acquire a write lock.
     * 仅读取的方法（如 getContextPrompt）获取读锁，而修改的方法（setFilePath、addNewTerm）获取写锁。
     */
    mutable QReadWriteLock m_lock;
};