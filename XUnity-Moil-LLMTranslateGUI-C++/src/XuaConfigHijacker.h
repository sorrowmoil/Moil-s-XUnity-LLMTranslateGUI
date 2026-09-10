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

    // 🚀 启动：备份 -> 注入 (带全套底层引擎控制)
    static QString autoDetectAndHijack(const QString &glossaryPath, int port, int maxThreads, bool handleRichText, bool extractNewline, const QString &hijackFromLang,
                                       const QString &hijackToLang, const QString &hijackEndpoint, bool hijackTextGetter, bool enableImGui, bool enableUGui,
                                       bool enableUIElements, bool enableNGUI, bool enableTextMeshPro, bool enableTextMesh, bool enableFairyGUI)
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
        while (!in.atEnd())
        {
            QString line = in.readLine();
            QString trimmed = line.trimmed();

            if (trimmed.startsWith("[") && trimmed.endsWith("]"))
            {
                currentSection = trimmed;
                lines.append(line);
                continue;
            }

            // --- 💉 拦截修改逻辑 ---
            if (currentSection == "[General]")
            {
                if (trimmed.startsWith("Language=", Qt::CaseInsensitive))
                {
                    line = "Language=" + hijackToLang;
                }
                else if (trimmed.startsWith("FromLanguage=", Qt::CaseInsensitive))
                {
                    line = "FromLanguage=" + hijackFromLang;
                }
            }
            else if (currentSection == "[Service]")
            {
                if (trimmed.startsWith("Endpoint=", Qt::CaseInsensitive))
                {
                    // 使用用户在高级设置里选好的 Endpoint
                    line = "Endpoint=" + hijackEndpoint;
                }
                else if (trimmed.startsWith("FallbackEndpoint=", Qt::CaseInsensitive))
                {
                    line = "FallbackEndpoint="; // 防止降级到其它插件破坏逻辑
                }
            }
            else if (currentSection == "[Google]" && trimmed.startsWith("ServiceUrl=", Qt::CaseInsensitive))
            {
                line = QString("ServiceUrl=http://localhost:%1").arg(port);
            }
            else if (currentSection == "[Custom]" && trimmed.startsWith("Url=", Qt::CaseInsensitive))
            {
                line = QString("Url=http://localhost:%1").arg(port);
            }
            else if (currentSection == "[Behaviour]")
            {
                if (trimmed.startsWith("EnableBatching=", Qt::CaseInsensitive))
                {
                    line = "EnableBatching=True"; // 如果进了这个函数说明开了多行模式
                }
                else if (trimmed.startsWith("MaxConcurrentTranslations=", Qt::CaseInsensitive))
                {
                    line = QString("MaxConcurrentTranslations=%1").arg(maxThreads);
                }
                else if (trimmed.startsWith("IgnoreWhitespaceInDialogue=", Qt::CaseInsensitive))
                {
                    line = extractNewline ? "IgnoreWhitespaceInDialogue=False" : "IgnoreWhitespaceInDialogue=True";
                }
                else if (trimmed.startsWith("IgnoreWhitespaceInNGUI=", Qt::CaseInsensitive))
                {
                    line = extractNewline ? "IgnoreWhitespaceInNGUI=False" : "IgnoreWhitespaceInNGUI=True";
                }
                else if (trimmed.startsWith("MaxCharactersPerTranslation=", Qt::CaseInsensitive))
                {
                    QString val = trimmed.section('=', 1).trimmed();
                    if (val.toInt() < 2500)
                        line = "MaxCharactersPerTranslation=2500";
                }
                else if (trimmed.startsWith("HandleRichText=", Qt::CaseInsensitive))
                {
                    line = handleRichText ? "HandleRichText=False" : "HandleRichText=True";
                }
                else if (trimmed.startsWith("TextGetterCompatibilityMode=", Qt::CaseInsensitive))
                {
                    // 💉 强制注入文本获取兼容模式
                    line = "TextGetterCompatibilityMode=" + QString(hijackTextGetter ? "True" : "False");
                }
            }
            else if (currentSection.compare("[TextFrameworks]", Qt::CaseInsensitive) == 0)
            {
                if (trimmed.startsWith("EnableIMGUI=", Qt::CaseInsensitive))
                {
                    line = QString("EnableIMGUI=%1").arg(enableImGui ? "True" : "False");
                }
                else if (trimmed.startsWith("EnableUGUI=", Qt::CaseInsensitive))
                {
                    line = QString("EnableUGUI=%1").arg(enableUGui ? "True" : "False");
                }
                else if (trimmed.startsWith("EnableUIElements=", Qt::CaseInsensitive))
                {
                    line = QString("EnableUIElements=%1").arg(enableUIElements ? "True" : "False");
                }
                else if (trimmed.startsWith("EnableNGUI=", Qt::CaseInsensitive))
                {
                    line = QString("EnableNGUI=%1").arg(enableNGUI ? "True" : "False");
                }
                else if (trimmed.startsWith("EnableTextMeshPro=", Qt::CaseInsensitive))
                {
                    line = QString("EnableTextMeshPro=%1").arg(enableTextMeshPro ? "True" : "False");
                }
                else if (trimmed.startsWith("EnableTextMesh=", Qt::CaseInsensitive))
                {
                    line = QString("EnableTextMesh=%1").arg(enableTextMesh ? "True" : "False");
                }
                else if (trimmed.startsWith("EnableFairyGUI=", Qt::CaseInsensitive))
                {
                    line = QString("EnableFairyGUI=%1").arg(enableFairyGUI ? "True" : "False");
                }
            }
            
            lines.append(line);
        }
        file.close();

        // 覆盖写入修改后的文件
        QFile outFile(iniPath);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            QTextStream out(&outFile);
            out.setEncoding(QStringConverter::Utf8);
            for (const QString &l : lines)
            {
                out << l << "\n";
            }
            outFile.close();
            return iniPath;
        }
        return "";
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
        QString originalLanguage = getIniValue(bakPath, "[General]", "Language");
        QString originalFromLanguage = getIniValue(bakPath, "[General]", "FromLanguage");
        QString originalEndpoint = getIniValue(bakPath, "[Service]", "Endpoint");
        QString originalFallbackEndpoint = getIniValue(bakPath, "[Service]", "FallbackEndpoint");
        QString originalTextGetter = getIniValue(bakPath, "[Behaviour]", "TextGetterCompatibilityMode");

        // 获取原始的换行符和字符限制配置
        QString origIgnoreDialogue = getIniValue(bakPath, "[Behaviour]", "IgnoreWhitespaceInDialogue");
        QString origIgnoreNGUI = getIniValue(bakPath, "[Behaviour]", "IgnoreWhitespaceInNGUI");
        QString origMaxChars = getIniValue(bakPath, "[Behaviour]", "MaxCharactersPerTranslation");
        QString origHandleRichText = getIniValue(bakPath, "[Behaviour]", "HandleRichText");

        QString originalEnableTextMesh = getIniValue(bakPath, "[TextFrameworks]", "EnableTextMesh");

        QString originalEnableImGui = getIniValue(bakPath, "[TextFrameworks]", "EnableIMGUI");

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

            // [General] 语言配置恢复
            if (currentSection.compare("[General]", Qt::CaseInsensitive) == 0)
            {
                if (trimmed.startsWith("Language=", Qt::CaseInsensitive))
                {
                    if (!originalLanguage.isEmpty())
                    {
                        line = "Language=" + originalLanguage;
                    }
                }
                else if (trimmed.startsWith("FromLanguage=", Qt::CaseInsensitive))
                {
                    if (!originalFromLanguage.isEmpty())
                    {
                        line = "FromLanguage=" + originalFromLanguage;
                    }
                }
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
            // [Service] 端点配置恢复
            else if (currentSection.compare("[Service]", Qt::CaseInsensitive) == 0)
            {
                if (trimmed.startsWith("Endpoint=", Qt::CaseInsensitive))
                {
                    if (!originalEndpoint.isEmpty())
                    {
                        line = "Endpoint=" + originalEndpoint;
                    }
                }
                else if (trimmed.startsWith(
                             "FallbackEndpoint=",
                             Qt::CaseInsensitive))
                {
                    // 备份中为空时，也必须恢复成空值
                    line = "FallbackEndpoint=" + originalFallbackEndpoint;
                }
            }
            // [TextFrameworks] 配置恢复
            else if (currentSection.compare("[TextFrameworks]", Qt::CaseInsensitive) == 0)
            {
                if (trimmed.startsWith("EnableTextMesh=", Qt::CaseInsensitive))
                {
                    line = "EnableTextMesh=" +
                           (originalEnableTextMesh.isEmpty()
                                ? QStringLiteral("False")
                                : originalEnableTextMesh);
                }
                else if (trimmed.startsWith("EnableIMGUI=", Qt::CaseInsensitive))
                {
                    line = "EnableIMGUI=" +
                           (originalEnableImGui.isEmpty()
                                ? QStringLiteral("False")
                                : originalEnableImGui);
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