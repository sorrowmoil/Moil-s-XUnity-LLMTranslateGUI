#ifndef DEPENDENCYINSTALLER_H
#define DEPENDENCYINSTALLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QString>

// 🚀 原生静默下载与解压引擎
class DependencyInstaller : public QObject
{
    Q_OBJECT
public:
    explicit DependencyInstaller(QObject* parent = nullptr);
    ~DependencyInstaller();

    // 传入直链 URL 和目标游戏文件夹
    void install(const QString& url, const QString& destDir);

signals:
    void progressUpdated(const QString& text); // 更新 UI 进度文字
    void finished(bool success, const QString& errorMsg); // 安装完成信号

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onExtractFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_reply;
    QProcess* m_process;
    QString m_tempZipPath;
    QString m_destDir;
};

#endif // DEPENDENCYINSTALLER_H