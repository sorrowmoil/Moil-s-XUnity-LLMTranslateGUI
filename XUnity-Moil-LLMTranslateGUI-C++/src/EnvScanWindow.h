#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QCloseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QTimer>
#include <QScrollArea>
#include <QVBoxLayout>
#include <functional>

// ==========================================
// 结构体：现代经典扫描结果行
// ==========================================
struct ScanRow
{
    QWidget *container;
    QLabel *iconLabel;
    QLabel *titleLabel;
    QLabel *subtitleLabel;
    QLabel *statusLabel;
    QPushButton *actionBtn;
};

class HelpLinkWidget; // 前置声明

class EnvScanWindow : public QDialog
{
    Q_OBJECT
public:
    explicit EnvScanWindow(bool isDark, int lang, QWidget *parent = nullptr);
    ~EnvScanWindow();

    void updateTheme(bool isDark);
    void updateLanguage(int lang);
    bool isClosing() const { return m_isClosing; }

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // UI 构建与样式
    void setupUI();
    ScanRow createModernRow(const QString &objName);
    void applyThemeStyle();
    void retranslateUI();
    void updateRowState(ScanRow &row, const QString &state, const QString &statusText, const QString &btnText, const QString &btnRole = "secondary");

    // 逻辑功能
    void performScan();
    void smoothSwitch(std::function<void()> changeLogic);
    QString getUnityPlayerVersion(const QString &dllPath);
    void showFloatingTooltip(QPushButton *btn, const QString &text);

    // 🌟 一键下载与部署核心引擎
    void startDownloadAndExtract(const QString &url, const QString &zipName, QLabel *lblStatus, QPushButton *btnAction, const QString &successMsg, const QString &autoRunExe = "");
    bool extractZipNative(const QString &zipPath, const QString &destDir);

private slots:
    void onSelectGameFolderClicked();

private:
    QNetworkAccessManager *m_netMgr;

    bool m_isDark;
    int m_lang;
    QString m_gameFolderPath;
    bool m_hasScanned = false;
    bool m_isClosing = false;

    // --- UI 核心组件 ---
    QScrollArea *m_scrollArea;
    QWidget *m_scrollWidget;

    // 顶部描述与按钮
    QLabel *lblHeaderTitle;
    QLabel *lblHeaderDesc;
    QPushButton *btnSelectFolder;

    // 总体状态摘要
    QFrame *frameSummary;
    QLabel *lblSummaryIcon;
    QLabel *lblSummaryTitle;
    QLabel *lblSummaryPath;
    QLabel *lblSummaryCount;

    // 基础环境组
    QLabel *lblBasicGroupTitle;
    QLabel *lblBasicGroupCount;
    QFrame *frameBasicGroup;
    ScanRow rowGamePath;
    ScanRow rowGameType;
    ScanRow rowUnityVersion;

    // 翻译组件组
    QLabel *lblPluginGroupTitle;
    QLabel *lblPluginGroupCount;
    QFrame *framePluginGroup;
    ScanRow rowXUnity;
    ScanRow rowGlossary;
    ScanRow rowFont;

    // 帮助组
    QLabel *lblHelpGroupTitle;
    QLabel *lblHelpGroupSubtitle;
    QFrame *frameHelpGroup;
    HelpLinkWidget *linkXUnity;
    HelpLinkWidget *linkGlossary;
    HelpLinkWidget *linkFont;
    HelpLinkWidget *linkFaq;

    // 提示文本缓存（用于多语言切换）
    QString m_tipGamePath, m_tipGameType, m_tipUnityVersion, m_tipXUnity, m_tipGlossary, m_tipFont;
};