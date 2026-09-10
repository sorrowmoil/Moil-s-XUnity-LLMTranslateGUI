#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMouseEvent>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QTimer>
#include <QScrollArea>

class GlassCard;
class ModernCompactLink;

// ==========================================
// 结构体：现代流光扫描结果行 (避免与经典模式重名)
// ==========================================
struct ModernScanRow
{
    QWidget *container;
    QLabel *iconLabel;
    QLabel *titleLabel;
    QLabel *subtitleLabel;
    QLabel *statusLabel;
    QPushButton *actionBtn;
};

class ModernEnvScanWindow : public QDialog
{
    Q_OBJECT
public:
    explicit ModernEnvScanWindow(bool isDark, int lang, QWidget *parent = nullptr);
    ~ModernEnvScanWindow();

    void updateTheme(bool isDark);
    void updateLanguage(int lang);
    void updateRounded(bool isRounded);
    void updateAlpha(int alpha);
    void setGlassParams(int renderMode, int hueShift, int tintIntensity);
    void animateClose();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // UI 构建与样式
    void setupUI();
    void applyThemeStyle();
    void retranslateUI();
    ModernScanRow createModernRow(const QString &objName);
    void updateRowState(ModernScanRow &row, const QString &state, const QString &statusText, const QString &btnText, const QString &btnRole = "secondary");
    
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

    int m_alpha = 210;
    bool m_isRounded = true;
    int m_glassMode = 0;
    int m_hueShift = 0;
    int m_tintIntensity = 100;

    bool m_isDragging = false;
    QPoint m_dragPos;
    bool m_isClosing = false;
    QRect m_finalRect, m_startRect;

    QWidget *m_bgFrame;
    QWidget *m_titleBar;
    QLabel *m_titleLabel;
    QPushButton *m_btnClose;

    QScrollArea *m_scrollArea;
    QWidget *m_scrollWidget;

    QPushButton *btnSelectFolder;

    QList<GlassCard*> m_glassCards;

    // 总体状态摘要
    GlassCard *frameSummary;
    QLabel *lblSummaryIcon;
    QLabel *lblSummaryTitle;
    QLabel *lblSummaryPath;
    QLabel *lblSummaryCount;

    // 基础环境组
    QLabel *lblBasicGroupTitle;
    QLabel *lblBasicGroupCount;
    GlassCard *frameBasicGroup;
    ModernScanRow rowGamePath;
    ModernScanRow rowGameType;
    ModernScanRow rowUnityVersion;

    // 翻译组件组
    QLabel *lblPluginGroupTitle;
    QLabel *lblPluginGroupCount;
    GlassCard *framePluginGroup;
    ModernScanRow rowXUnity;
    ModernScanRow rowGlossary;
    ModernScanRow rowFont;

    // 帮助组
    QLabel *lblHelpGroupTitle;
    QLabel *lblHelpGroupSubtitle;
    GlassCard *frameHelpGroup;
    ModernCompactLink *linkXUnity;
    ModernCompactLink *linkGlossary;
    ModernCompactLink *linkFont;
    ModernCompactLink *linkFaq;

    // 提示文本缓存（用于多语言切换）
    QString m_tipGamePath, m_tipGameType, m_tipUnityVersion, m_tipXUnity, m_tipGlossary, m_tipFont;
};