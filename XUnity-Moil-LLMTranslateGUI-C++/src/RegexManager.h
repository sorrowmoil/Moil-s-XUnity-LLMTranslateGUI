#pragma once
#include <QString>
#include <QList>
#include <QPair>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

struct RegexRule {
    QRegularExpression pattern;
    QString replacement;
};

class RegexManager {
public:
    static RegexManager& instance() {
        static RegexManager instance;
        return instance;
    }

    // 根据 _Substitutions.txt 的路径，自动寻找同级目录下的正则文件
    void autoLoadFrom(const QString& substitutionPath) {
        if (substitutionPath.isEmpty()) return;

        QFileInfo fileInfo(substitutionPath);
        QDir dir = fileInfo.dir();

        // 加载预处理
        loadRules(dir.filePath("_Preprocessors.txt"), m_preRules);
        // 加载后处理
        loadRules(dir.filePath("_Postprocessors.txt"), m_postRules);
    }

    // 执行预处理
    QString processPre(QString text) {
        for (const auto& rule : m_preRules) {
            text.replace(rule.pattern, rule.replacement);
        }
        return text;
    }

    // 执行后处理
    QString processPost(QString text) {
        for (const auto& rule : m_postRules) {
            text.replace(rule.pattern, rule.replacement);
        }
        return text;
    }

private:
    RegexManager() {}

    void loadRules(const QString& path, QList<RegexRule>& rules) {
        rules.clear();
        QFile file(path);
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 文件不存在是正常的，很多游戏没有正则文件
            return;
        }

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.isEmpty() || line.startsWith(";")) continue; // 跳过空行和注释

            // XUnity 格式: Regex=Replacement
            // 注意：正则本身可能包含 = 号，所以只分割第一个 =
            int idx = line.indexOf('=');
            if (idx >= 0) {
                QString patternStr = line.left(idx);
                QString replaceStr = line.mid(idx + 1);

                // 修正：XUnity 使用 $1 代表捕获组，Qt 使用 \1
                // 简单替换一下，提高兼容性
                replaceStr.replace("$", "\\");

                QRegularExpression regex(patternStr);
                if (regex.isValid()) {
                    rules.append({regex, replaceStr});
                }
            }
        }
        qDebug() << "Loaded" << rules.size() << "rules from" << path;
    }

    QList<RegexRule> m_preRules;
    QList<RegexRule> m_postRules;
};
