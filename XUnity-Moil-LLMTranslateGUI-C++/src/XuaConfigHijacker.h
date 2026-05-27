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
 * 🔥 XUA 配置智能接管工具 (精准还原 & 保险丝版)
 *
 * 核心修正：
 * 1. [精准还原锁]：Restore 时传入 port。只有当 Config.ini 中的 URL 严格等于 http://localhost:port 时，才判定为“我们的修改”，允许还原。
 * 2. [时光倒流]：还原 URL 时，优先去 .xua_bak 查找原始值。
 *    - 如果 .xua_bak 里有值 -> 恢复为原始 URL (完美复原)。
 *    - 如果 .xua_bak 里没值 -> [Google] 设为空，[Custom] 保持不变 (防止破坏 Custom)。
 * 3. [多行模式保护]：Restore 逻辑中不再触碰 EnableBatching，只处理 URL。
 * 4. [换行符强制接管]：劫持时检查 IgnoreWhitespaceInDialogue/NGUI，若不是 False 强制改 False，还原时恢复原样。
 */
class XuaConfigHijacker
{
public:
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

    // 辅助：从 INI 文件中提取特定 Section 的 Key 值
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

    // 🚀 启动：备份 -> 注入
    static QString autoDetectAndHijack(const QString &glossaryPath, int port, int maxThreads, bool handleRichText = false, bool extractNewline = true)
    {
        QString iniPath = deduceIniPath(glossaryPath);
        if (iniPath.isEmpty())
            return "";

        // 🛡️ 保险：仅在没有备份时备份，确保 .xua_bak 永远是最原始的纯净版
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

        // 换行符与字符限制标记
        bool hasIgnoreDialogue = false;
        bool hasIgnoreNGUI = false;
        bool hasMaxChars = false;
        bool hasHandleRichText = false;

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

            if (currentSection == "[Service]")
            {
                if (trimmed.startsWith("Endpoint=", Qt::CaseInsensitive))
                {
                    hasEndpoint = true;
                    QString val = trimmed.section('=', 1).trimmed();
                    if (val.startsWith("Custom", Qt::CaseInsensitive))
                    {
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
                // 🔥 智能换行符判断逻辑 🔥
                else if (trimmed.startsWith("IgnoreWhitespaceInDialogue=", Qt::CaseInsensitive))
                {
                    hasIgnoreDialogue = true;
                    if (extractNewline)
                    {
                        line = "IgnoreWhitespaceInDialogue=False";
                    }
                    else
                    {
                        line = "IgnoreWhitespaceInDialogue=True";
                    }
                }
                else if (trimmed.startsWith("IgnoreWhitespaceInNGUI=", Qt::CaseInsensitive))
                {
                    hasIgnoreNGUI = true;
                    if (extractNewline)
                    {
                        line = "IgnoreWhitespaceInNGUI=False";
                    }
                    else
                    {
                        line = "IgnoreWhitespaceInNGUI=True";
                    }
                }
                else if (trimmed.startsWith("MaxCharactersPerTranslation=", Qt::CaseInsensitive))
                {
                    hasMaxChars = true;
                    QString val = trimmed.section('=', 1).trimmed();
                    if (val.toInt() < 2500)
                    {
                        // 保护机制：如果当前长度太短，拉到安全值防止截断
                        line = "MaxCharactersPerTranslation=2500";
                    }
                }
                // 🔥 HandleRichText 劫持逻辑：HandleRichText=false 时开启文本渲染
                else if (trimmed.startsWith("HandleRichText=", Qt::CaseInsensitive))
                {
                    hasHandleRichText = true;
                    if (handleRichText)
                    {
                        // 勾选"文本处理"时，设置 HandleRichText=False 以开启文本渲染
                        line = "HandleRichText=False";
                    }
                    // 未勾选时保持原样
                }
            }

            lines.append(line);
        }
        file.close();

        // --- 查漏补缺 ---
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

        if (!hasBehaviourSection)
        {
            lines.append("");
            lines.append("[Behaviour]");
            lines.append("EnableBatching=True");
            lines.append(QString("MaxConcurrentTranslations=%1").arg(maxThreads));
            lines.append(extractNewline ? "IgnoreWhitespaceInDialogue=False" : "IgnoreWhitespaceInDialogue=True");
            lines.append(extractNewline ? "IgnoreWhitespaceInNGUI=False" : "IgnoreWhitespaceInNGUI=True");
            lines.append("MaxCharactersPerTranslation=2500");
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
            if (!hasIgnoreDialogue)
                for (int i = 0; i < lines.size(); ++i)
                    if (lines[i].trimmed().compare("[Behaviour]", Qt::CaseInsensitive) == 0)
                    {
                        lines.insert(i + 1, extractNewline ? "IgnoreWhitespaceInDialogue=False" : "IgnoreWhitespaceInDialogue=True");
                        break;
                    }
            if (!hasIgnoreNGUI)
                for (int i = 0; i < lines.size(); ++i)
                    if (lines[i].trimmed().compare("[Behaviour]", Qt::CaseInsensitive) == 0)
                    {
                        lines.insert(i + 1, extractNewline ? "IgnoreWhitespaceInNGUI=False" : "IgnoreWhitespaceInNGUI=True");
                        break;
                    }
            if (!hasMaxChars)
                for (int i = 0; i < lines.size(); ++i)
                    if (lines[i].trimmed().compare("[Behaviour]", Qt::CaseInsensitive) == 0)
                    {
                        lines.insert(i + 1, "MaxCharactersPerTranslation=2500");
                        break;
                    }
        }

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

    // 🛑 停止：精准还原
    static QString autoDetectAndRestore(const QString &glossaryPath, int port)
    {
        QString iniPath = deduceIniPath(glossaryPath);
        if (iniPath.isEmpty())
            return "";
        QString bakPath = iniPath + ".xua_bak";

        // 1. 尝试从备份中获取原始配置
        QString originalGoogleUrl = getIniValue(bakPath, "[Google]", "ServiceUrl");
        QString originalCustomUrl = getIniValue(bakPath, "[Custom]", "Url");

        // 获取原始的换行符和字符限制配置
        QString origIgnoreDialogue = getIniValue(bakPath, "[Behaviour]", "IgnoreWhitespaceInDialogue");
        QString origIgnoreNGUI = getIniValue(bakPath, "[Behaviour]", "IgnoreWhitespaceInNGUI");
        QString origMaxChars = getIniValue(bakPath, "[Behaviour]", "MaxCharactersPerTranslation");
        QString origHandleRichText = getIniValue(bakPath, "[Behaviour]", "HandleRichText");

        QFile file(iniPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return "";

        QStringList lines;
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        QString currentSection = "";

        // 🔒 锁的构建：只有完全匹配这个 String 才能被修改
        QString myHijackTarget = QString("http://localhost:%1").arg(port);

        while (!in.atEnd())
        {
            QString line = in.readLine();
            QString trimmed = line.trimmed();

            if (trimmed.startsWith("[") && trimmed.endsWith("]"))
            {
                currentSection = trimmed;
            }

            // [Google] 恢复
            if (currentSection == "[Google]" && trimmed.startsWith("ServiceUrl=", Qt::CaseInsensitive))
            {
                QString val = trimmed.section('=', 1).trimmed();
                if (val == myHijackTarget)
                {
                    if (!originalGoogleUrl.isEmpty())
                    {
                        line = "ServiceUrl=" + originalGoogleUrl;
                    }
                    else
                    {
                        line = "ServiceUrl=";
                    }
                }
            }
            // [Custom] 恢复
            else if (currentSection == "[Custom]" && trimmed.startsWith("Url=", Qt::CaseInsensitive))
            {
                QString val = trimmed.section('=', 1).trimmed();
                if (val == myHijackTarget)
                {
                    if (!originalCustomUrl.isEmpty())
                    {
                        line = "Url=" + originalCustomUrl;
                    }
                }
            }
            // [Behaviour] HandleRichText 等恢复
            else if (currentSection == "[Behaviour]")
            {
                if (trimmed.startsWith("HandleRichText=", Qt::CaseInsensitive))
                {
                    QString val = trimmed.section('=', 1).trimmed();
                    if (val.compare("False", Qt::CaseInsensitive) == 0 && !origHandleRichText.isEmpty())
                    {
                        line = "HandleRichText=" + origHandleRichText;
                    }
                    else if (val.compare("False", Qt::CaseInsensitive) == 0 && origHandleRichText.isEmpty())
                    {
                        line = "HandleRichText=True";
                    }
                }
                else if (trimmed.startsWith("IgnoreWhitespaceInDialogue=", Qt::CaseInsensitive))
                {
                    if (!origIgnoreDialogue.isEmpty())
                        line = "IgnoreWhitespaceInDialogue=" + origIgnoreDialogue;
                    else
                        line = "IgnoreWhitespaceInDialogue=True";
                }
                else if (trimmed.startsWith("IgnoreWhitespaceInNGUI=", Qt::CaseInsensitive))
                {
                    if (!origIgnoreNGUI.isEmpty())
                        line = "IgnoreWhitespaceInNGUI=" + origIgnoreNGUI;
                    else
                        line = "IgnoreWhitespaceInNGUI=True";
                }
            }

            lines.append(line);
        }
        file.close();

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

    static int hardRestoreFromBackup(const QString &glossaryPath)
    {
        QString iniPath = deduceIniPath(glossaryPath);
        if (iniPath.isEmpty())
            return 1;
        QString bakPath = iniPath + ".xua_bak";
        if (!QFile::exists(bakPath))
            return 2;
        // 1. 直接物理删除被劫持的 ini
        QFile::remove(iniPath);

        // 2. 将备份文件重命名回 Config.ini (相当于去掉了 .xua_bak 后缀)
        if (QFile::rename(bakPath, iniPath))
        {
            return 0; // 完美还原
        }
        return 3;
    }
};