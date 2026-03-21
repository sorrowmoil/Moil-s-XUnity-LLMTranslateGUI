#pragma once

#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QMap>

/**
 * XUA Configuration Hijacker – precise restoration with backup.
 * XUA 配置接管工具 – 带备份的精准还原版本。
 *
 * This class provides static methods to automatically detect and modify the game's
 * AutoTranslator configuration file (Config.ini) to redirect translation requests
 * to this local server (batch mode). It also ensures that only modifications made
 * by this tool are reverted, leaving user‑defined settings intact.
 * 该类提供静态方法，自动检测并修改游戏的 AutoTranslator 配置文件（Config.ini），
 * 将翻译请求重定向到本地服务器（打包模式）。同时确保仅还原由本工具所做的修改，
 * 保留用户自定义的设置。
 */
class XuaConfigHijacker
{
public:
    /**
     * Deduce the path to the game's AutoTranslator configuration file based on the glossary path.
     * 根据术语表路径推断游戏的 AutoTranslator 配置文件路径。
     *
     * The glossary file (_Substitutions.txt) is typically located in the game's
     * AutoTranslator folder. By going up three directories we reach the game root,
     * where we search for common configuration file names.
     * 术语表文件（_Substitutions.txt）通常位于游戏的 AutoTranslator 文件夹中。
     * 向上三级目录可到达游戏根目录，然后搜索常见的配置文件名称。
     *
     * @param glossaryPath Path to the glossary file (e.g., _Substitutions.txt).
     *                     术语表文件路径（如 _Substitutions.txt）。
     * @return Full path to the configuration file if found, otherwise an empty string.
     *         如果找到配置文件则返回完整路径，否则返回空字符串。
     */
    static QString deduceIniPath(const QString &glossaryPath)
    {
        if (glossaryPath.isEmpty())
            return "";
        QFileInfo fi(glossaryPath);
        QDir dir = fi.absoluteDir();
        if (!dir.cdUp())
            return "";
        if (!dir.cdUp())
            return "";
        if (!dir.cdUp())
            return "";
        QString basePath = dir.absolutePath();

        QStringList candidates = {
            "/Config.ini", "/config.ini",
            "/config/AutoTranslator.ini", "/config/AutoTranslatorConfig.ini"};
        for (const QString &cand : candidates)
        {
            QString fullPath = basePath + cand;
            if (QFile::exists(fullPath))
                return fullPath;
        }
        return "";
    }

    /**
     * Helper: retrieve a value from a specific section and key in an INI file.
     * 辅助函数：从 INI 文件的指定节和键获取值。
     *
     * @param filePath    Path to the INI file.
     *                    INI 文件路径。
     * @param sectionName Section name in brackets, e.g., "[Service]".
     *                    节名称（带方括号），如 "[Service]"。
     * @param keyName     Key name, e.g., "Endpoint".
     *                    键名，如 "Endpoint"。
     * @return The trimmed value if found, otherwise an empty string.
     *         如果找到则返回去除空白的值，否则返回空字符串。
     */
    static QString getIniValue(const QString &filePath, const QString &sectionName, const QString &keyName)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return "";

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        QString currentSection = "";
        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            if (line.startsWith("[") && line.endsWith("]"))
            {
                currentSection = line;
                continue;
            }
            if (currentSection.compare(sectionName, Qt::CaseInsensitive) == 0)
            {
                if (line.startsWith(keyName + "=", Qt::CaseInsensitive))
                {
                    return line.section('=', 1).trimmed();
                }
            }
        }
        return "";
    }

    /**
     * Start: backup the original config, then inject our settings.
     * 启动：备份原始配置，然后注入我们的设置。
     *
     * This method modifies the configuration to enable batch mode by:
     *   - Setting Endpoint=GoogleTranslate (or leaving Custom if it was Custom)
     *   - Setting FallbackEndpoint empty
     *   - Setting ServiceUrl under [Google] to http://localhost:port
     *   - Setting Url under [Custom] to http://localhost:port
     *   - Enabling EnableBatching and setting MaxConcurrentTranslations
     * A backup is created as ".xua_bak" if it does not already exist.
     * 该方法通过以下方式修改配置以启用多行模式：
     *   - 设置 Endpoint=GoogleTranslate（如果原是 Custom 则保留）
     *   - 清空 FallbackEndpoint
     *   - 将 [Google] 下的 ServiceUrl 设为 http://localhost:port
     *   - 将 [Custom] 下的 Url 设为 http://localhost:port
     *   - 启用 EnableBatching 并设置 MaxConcurrentTranslations
     * 如果备份文件 ".xua_bak" 不存在，则创建之。
     *
     * @param glossaryPath Path to the glossary file.
     *                     术语表文件路径。
     * @param port         Port on which the local server listens.
     *                     本地服务器监听的端口。
     * @param maxThreads   Maximum concurrent translations setting.
     *                     最大并发翻译数设置。
     * @return The base name of the modified file (e.g., "Config.ini") if successful,
     *         otherwise an empty string.
     *         如果成功，返回被修改文件的基本名（如 "Config.ini"），否则返回空字符串。
     */
    static QString autoDetectAndHijack(const QString &glossaryPath, int port, int maxThreads)
    {
        QString iniPath = deduceIniPath(glossaryPath);
        if (iniPath.isEmpty())
            return "";

        // Create a backup only if it does not already exist.
        // 仅当备份不存在时才创建。
        QString bakPath = iniPath + ".xua_bak";
        if (!QFile::exists(bakPath))
        {
            QFile::copy(iniPath, bakPath);
        }

        QFile file(iniPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return "";

        QStringList lines;
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        QString currentSection = "";

        // Flags to track which sections/keys are already present.
        // 跟踪哪些节/键已经存在的标志。
        bool hasServiceSection = false;
        bool hasGoogleSection = false;
        bool hasCustomSection = false;
        bool hasBehaviourSection = false;

        bool hasEndpoint = false;
        bool hasFallbackEndpoint = false;
        bool hasGoogleUrl = false;
        bool hasCustomUrl = false;
        bool hasBatching = false;
        bool hasMaxConcurrent = false;

        while (!in.atEnd())
        {
            QString line = in.readLine();
            QString trimmed = line.trimmed();

            if (trimmed.startsWith("[") && trimmed.endsWith("]"))
            {
                currentSection = trimmed;
                if (currentSection.compare("[Service]", Qt::CaseInsensitive) == 0)
                    hasServiceSection = true;
                if (currentSection.compare("[Google]", Qt::CaseInsensitive) == 0)
                    hasGoogleSection = true;
                if (currentSection.compare("[Custom]", Qt::CaseInsensitive) == 0)
                    hasCustomSection = true;
                if (currentSection.compare("[Behaviour]", Qt::CaseInsensitive) == 0)
                    hasBehaviourSection = true;
            }

            // Modify lines according to the desired configuration.
            // 根据期望的配置修改行内容。
            if (currentSection == "[Service]")
            {
                if (trimmed.startsWith("Endpoint=", Qt::CaseInsensitive))
                {
                    hasEndpoint = true;
                    QString val = trimmed.section('=', 1).trimmed();
                    if (val.startsWith("Custom", Qt::CaseInsensitive))
                    {
                        // Keep "Custom" as is; we only change other endpoints.
                        // 保持 "Custom" 不变；我们只修改其他端点。
                        line = trimmed;
                    }
                    else
                    {
                        line = "Endpoint=GoogleTranslate";
                    }
                }
                else if (trimmed.startsWith("FallbackEndpoint=", Qt::CaseInsensitive))
                {
                    line = "FallbackEndpoint=";
                    hasFallbackEndpoint = true;
                }
            }
            else if (currentSection == "[Google]" && trimmed.startsWith("ServiceUrl=", Qt::CaseInsensitive))
            {
                line = QString("ServiceUrl=http://localhost:%1").arg(port);
                hasGoogleUrl = true;
            }
            else if (currentSection == "[Custom]" && trimmed.startsWith("Url=", Qt::CaseInsensitive))
            {
                line = QString("Url=http://localhost:%1").arg(port);
                hasCustomUrl = true;
            }
            else if (currentSection == "[Behaviour]")
            {
                if (trimmed.startsWith("EnableBatching=", Qt::CaseInsensitive))
                {
                    line = "EnableBatching=True";
                    hasBatching = true;
                }
                else if (trimmed.startsWith("MaxConcurrentTranslations=", Qt::CaseInsensitive))
                {
                    line = QString("MaxConcurrentTranslations=%1").arg(maxThreads);
                    hasMaxConcurrent = true;
                }
            }

            lines.append(line);
        }
        file.close();

        // --- Add missing sections and keys ---
        // --- 添加缺失的节和键 ---

        // [Service]
        if (!hasServiceSection)
        {
            lines.insert(0, "[Service]");
            lines.insert(1, "Endpoint=GoogleTranslate");
            lines.insert(2, "FallbackEndpoint=");
            lines.insert(3, "");
        }
        else
        {
            if (!hasEndpoint)
            {
                for (int i = 0; i < lines.size(); ++i)
                    if (lines[i].trimmed().compare("[Service]", Qt::CaseInsensitive) == 0)
                    {
                        lines.insert(i + 1, "Endpoint=GoogleTranslate");
                        break;
                    }
            }
            if (!hasFallbackEndpoint)
            {
                for (int i = 0; i < lines.size(); ++i)
                    if (lines[i].trimmed().compare("[Service]", Qt::CaseInsensitive) == 0)
                    {
                        lines.insert(i + 1, "FallbackEndpoint=");
                        break;
                    }
            }
        }

        // [Behaviour]
        if (!hasBehaviourSection)
        {
            lines.append("");
            lines.append("[Behaviour]");
            lines.append("EnableBatching=True");
            lines.append(QString("MaxConcurrentTranslations=%1").arg(maxThreads));
        }
        else
        {
            if (!hasBatching)
                for (int i = 0; i < lines.size(); ++i)
                    if (lines[i].trimmed().compare("[Behaviour]", Qt::CaseInsensitive) == 0)
                    {
                        lines.insert(i + 1, "EnableBatching=True");
                        break;
                    }
            if (!hasMaxConcurrent)
                for (int i = 0; i < lines.size(); ++i)
                    if (lines[i].trimmed().compare("[Behaviour]", Qt::CaseInsensitive) == 0)
                    {
                        lines.insert(i + 1, QString("MaxConcurrentTranslations=%1").arg(maxThreads));
                        break;
                    }
        }

        // [Google]
        if (!hasGoogleSection)
        {
            lines.append("");
            lines.append("[Google]");
            lines.append(QString("ServiceUrl=http://localhost:%1").arg(port));
        }
        else if (!hasGoogleUrl)
        {
            for (int i = 0; i < lines.size(); ++i)
                if (lines[i].trimmed().compare("[Google]", Qt::CaseInsensitive) == 0)
                {
                    lines.insert(i + 1, QString("ServiceUrl=http://localhost:%1").arg(port));
                    break;
                }
        }

        // [Custom]
        if (!hasCustomSection)
        {
            lines.append("");
            lines.append("[Custom]");
            lines.append(QString("Url=http://localhost:%1").arg(port));
        }
        else if (!hasCustomUrl)
        {
            for (int i = 0; i < lines.size(); ++i)
                if (lines[i].trimmed().compare("[Custom]", Qt::CaseInsensitive) == 0)
                {
                    lines.insert(i + 1, QString("Url=http://localhost:%1").arg(port));
                    break;
                }
        }

        // Write back the modified content.
        // 将修改后的内容写回文件。
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            for (const QString &l : lines)
                out << l << "\n";
            file.close();
        }

        return QFileInfo(iniPath).fileName();
    }

    /**
     * Stop: precisely restore only the modifications made by this tool.
     * 停止：精准还原仅由本工具所做的修改。
     *
     * This method reads the original values from the backup file (.xua_bak).
     * It only restores a URL if it exactly matches the hijacked target
     * (http://localhost:port). If the backup contains an original value,
     * that value is restored; otherwise the URL is cleared (for Google) or
     * left empty (for Custom). Other settings (e.g., EnableBatching) are
     * left untouched.
     * 该方法从备份文件 (.xua_bak) 中读取原始值。仅当 URL 完全等于被劫持的目标
     * (http://localhost:port) 时才进行还原。如果备份中有原始值，则恢复该值；
     * 否则清空 URL（对于 Google）或置空（对于 Custom）。其他设置（如 EnableBatching）保持不变。
     *
     * @param glossaryPath Path to the glossary file.
     *                     术语表文件路径。
     * @param port         Port that was used during hijack (to identify our modification).
     *                     劫持时使用的端口（用于识别我们的修改）。
     * @return The base name of the modified file if successful, otherwise an empty string.
     *         如果成功，返回被修改文件的基本名，否则返回空字符串。
     */
    static QString autoDetectAndRestore(const QString &glossaryPath, int port)
    {
        QString iniPath = deduceIniPath(glossaryPath);
        if (iniPath.isEmpty())
            return "";
        QString bakPath = iniPath + ".xua_bak";

        // 1. Try to obtain the original URLs from the backup.
        // 1. 尝试从备份中获取原始 URL。
        QString originalGoogleUrl = getIniValue(bakPath, "[Google]", "ServiceUrl");
        QString originalCustomUrl = getIniValue(bakPath, "[Custom]", "Url");

        QFile file(iniPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return "";

        QStringList lines;
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        QString currentSection = "";

        // The exact string we look for to identify our own modifications.
        // 用于识别我们自身修改的精确字符串。
        QString myHijackTarget = QString("http://localhost:%1").arg(port);

        while (!in.atEnd())
        {
            QString line = in.readLine();
            QString trimmed = line.trimmed();

            if (trimmed.startsWith("[") && trimmed.endsWith("]"))
            {
                currentSection = trimmed;
            }

            // --- Restoration logic ---

            // Restore [Google] ServiceUrl
            if (currentSection == "[Google]" && trimmed.startsWith("ServiceUrl=", Qt::CaseInsensitive))
            {
                QString val = trimmed.section('=', 1).trimmed();
                // Only modify if the value matches our hijacked target.
                // 仅当值匹配我们的劫持目标时才修改。
                if (val == myHijackTarget)
                {
                    if (!originalGoogleUrl.isEmpty())
                    {
                        line = "ServiceUrl=" + originalGoogleUrl; // Restore backup value
                    }
                    else
                    {
                        line = "ServiceUrl="; // Backup had no value, clear it (default Google behaviour)
                    }
                }
            }
            // Restore [Custom] Url
            else if (currentSection == "[Custom]" && trimmed.startsWith("Url=", Qt::CaseInsensitive))
            {
                QString val = trimmed.section('=', 1).trimmed();
                // Check if it's our hijacked target.
                // 检查是否为我们劫持的目标。
                if (val == myHijackTarget)
                {
                    if (!originalCustomUrl.isEmpty())
                    {
                        line = "Url=" + originalCustomUrl; // Restore backup value (original Custom port)
                    }
                    else
                    {
                        // Important: if backup had no value, we set it empty.
                        // 重要：如果备份中没有值，则置空。
                        line = "Url=";
                    }
                }
                // If the value does not match myHijackTarget, we leave it untouched.
                // 如果值不匹配 myHijackTarget，我们保持不变。
            }

            lines.append(line);
        }
        file.close();

        // Write back the possibly restored lines.
        // 将可能已还原的行写回文件。
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            for (const QString &l : lines)
                out << l << "\n";
            file.close();
        }

        return QFileInfo(iniPath).fileName();
    }
};