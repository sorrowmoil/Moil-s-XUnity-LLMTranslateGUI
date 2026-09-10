#include "DependencyInstaller.h"
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QUrl>
#include <QNetworkRequest>

DependencyInstaller::DependencyInstaller(QObject* parent) 
    : QObject(parent), m_manager(new QNetworkAccessManager(this)), 
      m_reply(nullptr), m_process(nullptr) 
{
}

DependencyInstaller::~DependencyInstaller() {
    if (m_reply) m_reply->deleteLater();
    if (m_process) m_process->deleteLater();
}

void DependencyInstaller::install(const QString& url, const QString& destDir) 
{
    m_destDir = destDir;
    // 在系统 Temp 目录下生成一个临时 ZIP 文件
    m_tempZipPath = QDir::tempPath() + "/xunity_env_" + QString::number(QCoreApplication::applicationPid()) + ".zip";

    // 自动套上稳定镜像源，解决 Github 随机墙问题
    QString proxyUrl = url;
    if (url.contains("github.com")) {
        proxyUrl = "https://ghp.ci/" + url; 
    }

    QNetworkRequest req((QUrl(proxyUrl)));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_manager->get(req);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &DependencyInstaller::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &DependencyInstaller::onDownloadFinished);

    emit progressUpdated("🔗 连接中...");
}

void DependencyInstaller::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) 
{
    if (bytesTotal > 0) {
        int percent = (bytesReceived * 100) / bytesTotal;
        emit progressUpdated(QString("⬇️ 下载中 %1%").arg(percent));
    }
}

void DependencyInstaller::onDownloadFinished() 
{
    if (m_reply->error() != QNetworkReply::NoError) {
        emit finished(false, "下载失败");
        m_reply->deleteLater();
        m_reply = nullptr;
        return;
    }

    // 写入临时文件
    QFile file(m_tempZipPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit finished(false, "写入失败");
        m_reply->deleteLater();
        m_reply = nullptr;
        return;
    }
    file.write(m_reply->readAll());
    file.close();
    m_reply->deleteLater();
    m_reply = nullptr;

    emit progressUpdated("📦 解压部署中...");

    // 🌟 核心：白嫖 Windows 原生 PowerShell 进行静默解压覆盖
    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
            this, &DependencyInstaller::onExtractFinished);
    
    // -Force 参数确保强制覆盖现有文件
    QString cmd = "powershell";
    QStringList args;
    args << "-NoProfile" << "-NonInteractive" << "-Command" 
         << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force").arg(m_tempZipPath, m_destDir);
    
    // 隐藏命令行黑框
    m_process->start(cmd, args);
}

void DependencyInstaller::onExtractFinished(int exitCode, QProcess::ExitStatus exitStatus) 
{
    QFile::remove(m_tempZipPath); // 无论成功与否，擦除临时文件痕迹

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit finished(true, "安装成功");
    } else {
        emit finished(false, "解压失败");
    }
    
    m_process->deleteLater();
    m_process = nullptr;
}