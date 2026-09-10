#include "EnvScanWindow.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QToolTip>
#include <QDesktopServices>
#include <QUrl>
#include <QMouseEvent>
#include <windows.h>
#include <winver.h>
#include <QRegularExpression>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDirIterator>
#include <QPixmap>
#include <QRandomGenerator>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QStyle>

// ==========================================
// Github 加速节点负载均衡池
// ==========================================
static QString getLoadBalancedUrl(const QString &originalUrl)
{
    static const QStringList MIRRORS = {
        "https://gh.xmly.dev/",
        "https://github.dpik.top/",
        "https://mirror.ghproxy.com/"};
    int index = QRandomGenerator::global()->bounded(MIRRORS.size());
    return MIRRORS[index] + originalUrl;
}

// ==========================================
// 帮助面板链接组件 (独立封装)
// ==========================================
class HelpLinkWidget : public QWidget
{
public:
    HelpLinkWidget(const QString &symbol, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setObjectName("helpLinkWidget");

        QHBoxLayout *layout = new QHBoxLayout(this);
        // 🔥 左侧减去 4px 用于悬浮粗边框像素补偿 (10 - 4 = 6)
        layout->setContentsMargins(6, 5, 10, 5);
        layout->setSpacing(8);

        m_symbol = new QLabel(symbol, this);
        m_symbol->setObjectName("helpSymbol");
        m_symbol->setFixedWidth(20);
        m_symbol->setAlignment(Qt::AlignCenter);

        m_title = new QLabel(this);
        m_title->setObjectName("helpTitle");
        m_title->setFixedWidth(105);

        m_desc = new QLabel(this);
        m_desc->setObjectName("helpDesc");
        m_desc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        m_arrow = new QLabel("›", this);
        m_arrow->setObjectName("helpArrow");

        layout->addWidget(m_symbol);
        layout->addWidget(m_title);
        layout->addWidget(m_desc);
        layout->addWidget(m_arrow);
    }

    void updateContent(const QString &title, const QString &desc, const QString &url, const QString &tooltip)
    {
        m_title->setText(title);
        m_desc->setText(desc);
        m_url = url;
        m_tooltip = tooltip;
        setToolTip(tooltip); 
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (!m_url.isEmpty())
                QDesktopServices::openUrl(QUrl(m_url));
        }
        QWidget::mouseReleaseEvent(event);
    }
private:
    QLabel *m_symbol;
    QLabel *m_title;
    QLabel *m_desc;
    QLabel *m_arrow;
    QString m_url;
    QString m_tooltip;
};

// ==========================================
// EnvScanWindow 主体实现
// ==========================================

EnvScanWindow::EnvScanWindow(bool isDark, int lang, QWidget *parent)
    : QDialog(parent), m_isDark(isDark), m_lang(lang)
{
    m_netMgr = new QNetworkAccessManager(this);

    QPixmap transparentPix(1, 1);
    transparentPix.fill(Qt::transparent);
    setWindowIcon(QIcon(transparentPix));
    
    resize(420, 680);
    setMinimumSize(400, 600);
    setMaximumSize(520, 880);

    setupUI();
    applyThemeStyle();
    retranslateUI();
}

EnvScanWindow::~EnvScanWindow() {}

void EnvScanWindow::closeEvent(QCloseEvent *event)
{
    if (m_isClosing) {
        event->accept();
        return;
    }
    event->ignore();
    m_isClosing = true;

    QPropertyAnimation *fadeOut = new QPropertyAnimation(this, "windowOpacity");
    fadeOut->setDuration(250);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    connect(fadeOut, &QPropertyAnimation::finished, this, [this]() {
        hide();
        deleteLater(); 
    });
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

void EnvScanWindow::smoothSwitch(std::function<void()> changeLogic)
{
    QPixmap pixmap = this->grab();
    QLabel *overlay = new QLabel(this);
    overlay->setPixmap(pixmap);
    overlay->setGeometry(0, 0, this->width(), this->height());
    overlay->show();

    changeLogic();

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(overlay);
    overlay->setGraphicsEffect(effect);
    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(250);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished, overlay, &QLabel::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void EnvScanWindow::updateTheme(bool isDark)
{
    if (m_isDark == isDark) return;
    smoothSwitch([this, isDark]() {
        m_isDark = isDark;
        applyThemeStyle(); 
    });
}

void EnvScanWindow::updateLanguage(int lang)
{
    if (m_lang == lang) return;
    smoothSwitch([this, lang]() {
        m_lang = lang;
        retranslateUI();
        if (m_hasScanned) performScan(); 
    });
}

void EnvScanWindow::showFloatingTooltip(QPushButton *btn, const QString &text)
{
    QPoint globalPos = btn->mapToGlobal(QPoint(btn->width() - 240, btn->height() + 5));
    QToolTip::showText(globalPos, text, btn);
}

ScanRow EnvScanWindow::createModernRow(const QString &objName)
{
    ScanRow row;
    row.container = new QWidget(this);
    row.container->setObjectName(objName);
    row.container->setMinimumHeight(44);
    row.container->setCursor(Qt::WhatsThisCursor);

    QGridLayout *layout = new QGridLayout(row.container);
    // 🔥 左侧减去 4px 用于悬浮粗边框像素补偿 (10 - 4 = 6)
    layout->setContentsMargins(6, 4, 10, 4);
    layout->setHorizontalSpacing(10);
    layout->setVerticalSpacing(0);

    row.iconLabel = new QLabel("—", row.container);
    row.iconLabel->setObjectName("stateIcon");
    row.iconLabel->setFixedSize(24, 24);
    row.iconLabel->setAlignment(Qt::AlignCenter);
    row.iconLabel->setProperty("state", "pending");

    row.titleLabel = new QLabel(row.container);
    row.titleLabel->setObjectName("rowTitle");

    row.subtitleLabel = new QLabel(row.container);
    row.subtitleLabel->setObjectName("rowSubtitle");

    row.statusLabel = new QLabel(row.container);
    row.statusLabel->setObjectName("rowStatus");
    row.statusLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    row.statusLabel->setProperty("state", "pending");
    row.statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    row.actionBtn = new QPushButton(row.container);
    row.actionBtn->setObjectName("actionBtn");
    row.actionBtn->setCursor(Qt::PointingHandCursor);
    row.actionBtn->setMinimumSize(54, 24);
    row.actionBtn->setProperty("role", "secondary");

    layout->addWidget(row.iconLabel, 0, 0, 2, 1, Qt::AlignVCenter);
    layout->addWidget(row.titleLabel, 0, 1, 1, 1, Qt::AlignBottom);
    layout->addWidget(row.subtitleLabel, 1, 1, 1, 1, Qt::AlignTop);
    layout->addWidget(row.statusLabel, 0, 2, 2, 1, Qt::AlignVCenter);
    layout->addWidget(row.actionBtn, 0, 3, 2, 1, Qt::AlignVCenter);

    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(2, 2);

    return row;
}

void EnvScanWindow::updateRowState(ScanRow &row, const QString &state, const QString &statusText, const QString &btnText, const QString &btnRole)
{
    row.statusLabel->setText(statusText);
    row.actionBtn->setText(btnText);
    
    if (state == "success") row.iconLabel->setText("✓");
    else if (state == "warning" || state == "danger") row.iconLabel->setText("!");
    else row.iconLabel->setText("—");

    row.iconLabel->setProperty("state", state);
    row.statusLabel->setProperty("state", state);
    row.actionBtn->setProperty("role", btnRole);

    row.iconLabel->style()->unpolish(row.iconLabel);
    row.iconLabel->style()->polish(row.iconLabel);
    
    row.statusLabel->style()->unpolish(row.statusLabel);
    row.statusLabel->style()->polish(row.statusLabel);

    row.actionBtn->style()->unpolish(row.actionBtn);
    row.actionBtn->style()->polish(row.actionBtn);
}

void EnvScanWindow::setupUI()
{
    QVBoxLayout *baseLayout = new QVBoxLayout(this);
    baseLayout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setObjectName("mainScrollArea");

    m_scrollWidget = new QWidget(m_scrollArea);
    m_scrollWidget->setObjectName("mainScrollWidget");
    QVBoxLayout *mainLayout = new QVBoxLayout(m_scrollWidget);
    
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    QWidget *headerWidget = new QWidget(m_scrollWidget);
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(2);

    lblHeaderTitle = new QLabel(headerWidget);
    lblHeaderTitle->setObjectName("headerTitle");
    lblHeaderDesc = new QLabel(headerWidget);
    lblHeaderDesc->setObjectName("headerDesc");

    headerLayout->addWidget(lblHeaderTitle);
    headerLayout->addWidget(lblHeaderDesc);
    mainLayout->addWidget(headerWidget);

    btnSelectFolder = new QPushButton(m_scrollWidget);
    btnSelectFolder->setObjectName("btnSelectFolder");
    btnSelectFolder->setCursor(Qt::PointingHandCursor);
    btnSelectFolder->setMinimumHeight(38);
    mainLayout->addWidget(btnSelectFolder);

    frameSummary = new QFrame(m_scrollWidget);
    frameSummary->setObjectName("frameSummary");
    frameSummary->setProperty("state", "pending"); 
    
    QHBoxLayout *summaryLayout = new QHBoxLayout(frameSummary);
    summaryLayout->setContentsMargins(12, 8, 12, 8);
    summaryLayout->setSpacing(8);

    lblSummaryIcon = new QLabel("—", frameSummary);
    lblSummaryIcon->setObjectName("summaryIcon");
    lblSummaryIcon->setProperty("state", "pending");
    lblSummaryIcon->setFixedSize(24, 24);
    lblSummaryIcon->setAlignment(Qt::AlignCenter);

    QWidget *sumCenterWidget = new QWidget(frameSummary);
    QVBoxLayout *sumCenterLayout = new QVBoxLayout(sumCenterWidget);
    sumCenterLayout->setContentsMargins(0, 0, 0, 0);
    sumCenterLayout->setSpacing(2);
    
    lblSummaryTitle = new QLabel(sumCenterWidget);
    lblSummaryTitle->setObjectName("summaryTitle");
    lblSummaryPath = new QLabel(sumCenterWidget);
    lblSummaryPath->setObjectName("summaryPath");

    sumCenterLayout->addWidget(lblSummaryTitle);
    sumCenterLayout->addWidget(lblSummaryPath);

    lblSummaryCount = new QLabel(frameSummary);
    lblSummaryCount->setObjectName("summaryCount");
    lblSummaryCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    summaryLayout->addWidget(lblSummaryIcon);
    summaryLayout->addWidget(sumCenterWidget, 1);
    summaryLayout->addWidget(lblSummaryCount);
    mainLayout->addWidget(frameSummary);
    
    QWidget *basicWrapper = new QWidget(m_scrollWidget);
    QVBoxLayout *basicWrapLayout = new QVBoxLayout(basicWrapper);
    basicWrapLayout->setContentsMargins(0, 0, 0, 0);
    basicWrapLayout->setSpacing(4);

    QWidget *basicTitleWidget = new QWidget(basicWrapper);
    QHBoxLayout *basicTitleLayout = new QHBoxLayout(basicTitleWidget);
    basicTitleLayout->setContentsMargins(4, 0, 4, 0); 
    
    lblBasicGroupTitle = new QLabel(basicTitleWidget);
    lblBasicGroupTitle->setObjectName("groupTitle");
    lblBasicGroupCount = new QLabel(basicTitleWidget);
    lblBasicGroupCount->setObjectName("groupCount");
    lblBasicGroupCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    basicTitleLayout->addWidget(lblBasicGroupTitle);
    basicTitleLayout->addStretch();
    basicTitleLayout->addWidget(lblBasicGroupCount);
    
    frameBasicGroup = new QFrame(basicWrapper);
    frameBasicGroup->setObjectName("groupFrame");
    QVBoxLayout *basicLayout = new QVBoxLayout(frameBasicGroup);
    basicLayout->setContentsMargins(0, 0, 0, 0);
    basicLayout->setSpacing(0);

    rowGamePath = createModernRow("resultRowTop");
    rowGameType = createModernRow("resultRowMiddle");
    rowUnityVersion = createModernRow("resultRowBottom");
    basicLayout->addWidget(rowGamePath.container);
    basicLayout->addWidget(rowGameType.container);
    basicLayout->addWidget(rowUnityVersion.container);

    basicWrapLayout->addWidget(basicTitleWidget);
    basicWrapLayout->addWidget(frameBasicGroup);
    mainLayout->addWidget(basicWrapper);

    QWidget *pluginWrapper = new QWidget(m_scrollWidget);
    QVBoxLayout *pluginWrapLayout = new QVBoxLayout(pluginWrapper);
    pluginWrapLayout->setContentsMargins(0, 0, 0, 0);
    pluginWrapLayout->setSpacing(4);

    QWidget *pluginTitleWidget = new QWidget(pluginWrapper);
    QHBoxLayout *pluginTitleLayout = new QHBoxLayout(pluginTitleWidget);
    pluginTitleLayout->setContentsMargins(4, 0, 4, 0);
    
    lblPluginGroupTitle = new QLabel(pluginTitleWidget);
    lblPluginGroupTitle->setObjectName("groupTitle");
    lblPluginGroupCount = new QLabel(pluginTitleWidget);
    lblPluginGroupCount->setObjectName("groupCount");
    lblPluginGroupCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    pluginTitleLayout->addWidget(lblPluginGroupTitle);
    pluginTitleLayout->addStretch();
    pluginTitleLayout->addWidget(lblPluginGroupCount);
    
    framePluginGroup = new QFrame(pluginWrapper);
    framePluginGroup->setObjectName("groupFrame");
    QVBoxLayout *pluginLayout = new QVBoxLayout(framePluginGroup);
    pluginLayout->setContentsMargins(0, 0, 0, 0);
    pluginLayout->setSpacing(0);

    rowXUnity = createModernRow("resultRowTop");
    rowGlossary = createModernRow("resultRowMiddle");
    rowFont = createModernRow("resultRowBottom");
    pluginLayout->addWidget(rowXUnity.container);
    pluginLayout->addWidget(rowGlossary.container);
    pluginLayout->addWidget(rowFont.container);

    pluginWrapLayout->addWidget(pluginTitleWidget);
    pluginWrapLayout->addWidget(framePluginGroup);
    mainLayout->addWidget(pluginWrapper);

    QWidget *helpWrapper = new QWidget(m_scrollWidget);
    QVBoxLayout *helpWrapLayout = new QVBoxLayout(helpWrapper);
    helpWrapLayout->setContentsMargins(0, 0, 0, 0);
    helpWrapLayout->setSpacing(4);

    QWidget *helpTitleWidget = new QWidget(helpWrapper);
    QHBoxLayout *helpTitleLayout = new QHBoxLayout(helpTitleWidget);
    helpTitleLayout->setContentsMargins(4, 0, 4, 0);
    
    lblHelpGroupTitle = new QLabel(helpTitleWidget);
    lblHelpGroupTitle->setObjectName("groupTitle");
    lblHelpGroupSubtitle = new QLabel(helpTitleWidget);
    lblHelpGroupSubtitle->setObjectName("groupCount");
    
    helpTitleLayout->addWidget(lblHelpGroupTitle);
    helpTitleLayout->addStretch();
    helpTitleLayout->addWidget(lblHelpGroupSubtitle);
    
    frameHelpGroup = new QFrame(helpWrapper);
    frameHelpGroup->setObjectName("groupFrame");
    QVBoxLayout *helpLayout = new QVBoxLayout(frameHelpGroup);
    helpLayout->setContentsMargins(0, 0, 0, 0);
    helpLayout->setSpacing(0);

    linkXUnity = new HelpLinkWidget("?", frameHelpGroup);
    linkXUnity->setObjectName("helpRowTop");
    linkGlossary = new HelpLinkWidget("#", frameHelpGroup);
    linkGlossary->setObjectName("helpRowMiddle");
    linkFont = new HelpLinkWidget("A", frameHelpGroup);
    linkFont->setObjectName("helpRowMiddle");
    linkFaq = new HelpLinkWidget("!", frameHelpGroup);
    linkFaq->setObjectName("helpRowBottom");

    helpLayout->addWidget(linkXUnity);
    helpLayout->addWidget(linkGlossary);
    helpLayout->addWidget(linkFont);
    helpLayout->addWidget(linkFaq);

    helpWrapLayout->addWidget(helpTitleWidget);
    helpWrapLayout->addWidget(frameHelpGroup);
    mainLayout->addWidget(helpWrapper);

    mainLayout->addStretch(1);

    m_scrollArea->setWidget(m_scrollWidget);
    baseLayout->addWidget(m_scrollArea);

    connect(btnSelectFolder, &QPushButton::clicked, this, &EnvScanWindow::onSelectGameFolderClicked);
}

void EnvScanWindow::applyThemeStyle()
{
    QString windowBg = m_isDark ? "#1E1F22" : "#F5F6F7";
    QString panelBg = m_isDark ? "#282A2E" : "#FFFFFF";
    
    // 🔥 大幅增强悬浮背景色对比度
    QString rowHover = m_isDark ? "#3F434C" : "#E8F0F8"; 
    
    QString border = m_isDark ? "#41444A" : "#D6D9DE";
    QString separator = m_isDark ? "#383B40" : "#E7E9EC";
    
    QString textMain = m_isDark ? "#E5E7EB" : "#202124";
    QString textSec = m_isDark ? "#B1B5BC" : "#666B73";
    QString textMuted = m_isDark ? "#898E97" : "#8A9099";

    // 🔥 专属定制：暗色配赤金，亮色配 VS Code蓝
    QString accent = m_isDark ? "#E6B422" : "#007ACC";
    QString accentHover = m_isDark ? "#FFD04B" : "#005999";
    
    // 🔥 当主要按钮背景是亮色的赤金时，文字自动变为深色，保证可读性
    QString primaryText = m_isDark ? "#191919" : "#FFFFFF";
    
    QString success = m_isDark ? "#72C58E" : "#287A46";
    QString successSoft = m_isDark ? "#263C2E" : "#E9F5ED";
    
    QString warning = m_isDark ? "#E8B45C" : "#A15C00";
    QString warningSoft = m_isDark ? "#443722" : "#FFF3DC";
    
    QString danger = m_isDark ? "#EF8580" : "#B3261E";
    QString dangerSoft = m_isDark ? "#472B2A" : "#FCEBEA";

    QString qss = QString(
        "QDialog, QScrollArea#mainScrollArea, QWidget#mainScrollWidget { background-color: %windowBg%; }"
        "QLabel { font-family: 'Segoe UI', 'Microsoft YaHei UI', 'Microsoft YaHei'; color: %textMain%; }"
        
        "QLabel#headerTitle { font-size: 18px; font-weight: bold; color: %textMain%; }"
        "QLabel#headerDesc { font-size: 11px; color: %textSec%; }"
        
        "QPushButton#btnSelectFolder { background-color: %panelBg%; border: 1px solid %border%; border-radius: 6px; padding: 0 14px; text-align: left; font-size: 12px; font-weight: bold; color: %textMain%; }"
        "QPushButton#btnSelectFolder:hover { background-color: %rowHover%; border-color: %accent%; }"
        
        "QFrame#frameSummary { border-radius: 6px; border: 1px solid %border%; background-color: %panelBg%; border-left: 4px solid %border%; }"
        "QFrame#frameSummary[state=\"pending\"] { border-left-color: %border%; background-color: %panelBg%; }"
        "QFrame#frameSummary[state=\"success\"] { border-left-color: %success%; background-color: %successSoft%; }"
        "QFrame#frameSummary[state=\"warning\"] { border-left-color: %warning%; background-color: %warningSoft%; }"
        "QFrame#frameSummary[state=\"danger\"] { border-left-color: %danger%; background-color: %dangerSoft%; }"
        
        "QLabel#summaryIcon { font-size: 14px; font-weight: bold; border-radius: 12px; }"
        "QLabel#summaryIcon[state=\"pending\"] { color: %textMuted%; background: transparent; }"
        "QLabel#summaryIcon[state=\"success\"] { color: %success%; background: transparent; }"
        "QLabel#summaryIcon[state=\"warning\"] { color: %warning%; background: transparent; }"
        "QLabel#summaryIcon[state=\"danger\"] { color: %danger%; background: transparent; }"
        
        "QLabel#summaryTitle { font-size: 12px; font-weight: bold; color: %textMain%; }"
        "QLabel#summaryPath { font-size: 10px; color: %textSec%; }"
        "QLabel#summaryCount { font-size: 10px; color: %textSec%; }"
        
        "QLabel#groupTitle { font-size: 10px; font-weight: bold; color: %textSec%; letter-spacing: 1px; text-transform: uppercase; }"
        "QLabel#groupCount { font-size: 10px; color: %textMuted%; }"
        
        "QFrame#groupFrame { background-color: %panelBg%; border: 1px solid %border%; border-radius: 6px; }"
        
        // 🔥 边框加粗到 4px！更加醒目！
        "QWidget#resultRowTop { border-bottom: 1px solid %separator%; border-top-left-radius: 5px; border-top-right-radius: 5px; border-left: 4px solid transparent; }"
        "QWidget#resultRowMiddle { border-bottom: 1px solid %separator%; border-left: 4px solid transparent; }"
        "QWidget#resultRowBottom { border-bottom: none; border-bottom-left-radius: 5px; border-bottom-right-radius: 5px; border-left: 4px solid transparent; }"
        "QWidget#resultRowTop:hover, QWidget#resultRowMiddle:hover, QWidget#resultRowBottom:hover { background-color: %rowHover%; border-left-color: %accent%; }"
        
        "QWidget#helpRowTop { border-bottom: 1px solid %separator%; border-top-left-radius: 5px; border-top-right-radius: 5px; border-left: 4px solid transparent; }"
        "QWidget#helpRowMiddle { border-bottom: 1px solid %separator%; border-left: 4px solid transparent; }"
        "QWidget#helpRowBottom { border-bottom: none; border-bottom-left-radius: 5px; border-bottom-right-radius: 5px; border-left: 4px solid transparent; }"
        "QWidget#helpRowTop:hover, QWidget#helpRowMiddle:hover, QWidget#helpRowBottom:hover { background-color: %rowHover%; border-left-color: %accent%; }"
        
        "QLabel#rowTitle { font-size: 11px; font-weight: bold; color: %textMain%; }"
        "QLabel#rowSubtitle { font-size: 10px; color: %textMuted%; }"
        "QLabel#rowStatus { font-size: 11px; font-weight: bold; }"
        
        "QLabel#rowStatus[state=\"success\"] { color: %success%; }"
        "QLabel#rowStatus[state=\"warning\"] { color: %warning%; }"
        "QLabel#rowStatus[state=\"danger\"] { color: %danger%; }"
        "QLabel#rowStatus[state=\"pending\"] { color: %textSec%; }"
        
        "QLabel#stateIcon { font-size: 12px; font-weight: bold; border-radius: 12px; }"
        "QLabel#stateIcon[state=\"success\"] { color: %success%; background-color: %successSoft%; }"
        "QLabel#stateIcon[state=\"warning\"] { color: %warning%; background-color: %warningSoft%; }"
        "QLabel#stateIcon[state=\"danger\"] { color: %danger%; background-color: %dangerSoft%; }"
        "QLabel#stateIcon[state=\"pending\"] { color: %textMuted%; background-color: transparent; }"
        
        "QPushButton#actionBtn { background-color: transparent; border: 1px solid transparent; border-radius: 4px; font-size: 10px; font-weight: bold; }"
        
        "QPushButton#actionBtn[role=\"secondary\"] { color: %accent%; border: 1px solid transparent; background: transparent; padding: 4px 6px; }"
        "QPushButton#actionBtn[role=\"secondary\"]:hover { background-color: transparent; color: %accentHover%; text-decoration: underline; }"
        
        // 🔥 Primary Button 文字颜色智能适配 (暗色赤金时为深色，亮色 VS蓝时为白色)
        "QPushButton#actionBtn[role=\"primary\"] { color: %primaryText%; background-color: %accent%; border: 1px solid %accent%; padding: 4px 10px; }"
        "QPushButton#actionBtn[role=\"primary\"]:hover { background-color: %accentHover%; border-color: %accentHover%; }"
        
        "QPushButton#actionBtn:disabled { background-color: %border%; color: %textMuted%; border: none; }"
        
        "QLabel#helpSymbol { font-size: 12px; color: %textSec%; font-weight: bold; }"
        "QLabel#helpTitle { font-size: 11px; font-weight: bold; color: %textMain%; }"
        "QLabel#helpDesc { font-size: 10px; color: %textMuted%; }"
        "QLabel#helpArrow { font-size: 14px; color: %textMuted%; }"
        
        "QToolTip { border: 1px solid %accent%; background-color: %panelBg%; color: %textMain%; padding: 8px; border-radius: 6px; font-size: 12px; }"
    )
    .replace("%windowBg%", windowBg)
    .replace("%textMain%", textMain)
    .replace("%textSec%", textSec)
    .replace("%panelBg%", panelBg)
    .replace("%border%", border)
    .replace("%rowHover%", rowHover)
    .replace("%accent%", accent)
    .replace("%textMuted%", textMuted)
    .replace("%separator%", separator)
    .replace("%success%", success)
    .replace("%warning%", warning)
    .replace("%danger%", danger)
    .replace("%successSoft%", successSoft)
    .replace("%warningSoft%", warningSoft)
    .replace("%dangerSoft%", dangerSoft)
    .replace("%accentHover%", accentHover)
    .replace("%primaryText%", primaryText);

    setStyleSheet(qss);
}

void EnvScanWindow::retranslateUI()
{
    setWindowTitle(m_lang == 1 ? "环境扫描" : "Environment Scan");
    
    lblHeaderTitle->setText(m_lang == 1 ? "环境检测中心" : "Environment Check");
    lblHeaderDesc->setText(m_lang == 1 ? "检查游戏运行环境和翻译组件是否配置完整" : "Verify game environment and translation components");
    
    btnSelectFolder->setText(m_lang == 1 ? "📂  选择游戏目录并开始扫描" : "📂  Select Game Folder && Scan");

    lblBasicGroupTitle->setText(m_lang == 1 ? "基础环境" : "Basic Environment");
    lblPluginGroupTitle->setText(m_lang == 1 ? "翻译组件" : "Translation Components");
    lblHelpGroupTitle->setText(m_lang == 1 ? "使用帮助" : "Documentation");
    lblHelpGroupSubtitle->setText(m_lang == 1 ? "查阅教程或疑难解答" : "Guides and Troubleshooting");

    rowGamePath.titleLabel->setText(m_lang == 1 ? "游戏目录" : "Game Path");
    rowGamePath.subtitleLabel->setText(m_lang == 1 ? "安装位置" : "Installation");
    
    rowGameType.titleLabel->setText(m_lang == 1 ? "游戏架构" : "Game Arch");
    rowGameType.subtitleLabel->setText(m_lang == 1 ? "Unity 后端" : "Unity Backend");
    
    rowUnityVersion.titleLabel->setText(m_lang == 1 ? "Unity 版本" : "Unity Version");
    rowUnityVersion.subtitleLabel->setText(m_lang == 1 ? "引擎兼容性" : "Engine Core");
    
    rowXUnity.titleLabel->setText(m_lang == 1 ? "XUnity 插件" : "XUnity Plugin");
    rowXUnity.subtitleLabel->setText(m_lang == 1 ? "文本拦截核心" : "Text Interceptor");
    
    rowGlossary.titleLabel->setText(m_lang == 1 ? "术语表" : "Glossary");
    rowGlossary.subtitleLabel->setText(m_lang == 1 ? "本地替换词典" : "Local Dictionary");
    
    rowFont.titleLabel->setText(m_lang == 1 ? "字体资源" : "Font Assets");
    rowFont.subtitleLabel->setText(m_lang == 1 ? "字符渲染" : "Character Rendering");

    m_tipGamePath = m_lang == 1 
        ? "<b>📌 游戏安装路径检测</b><br><br>当前扫描的游戏目录。自动翻译器必须安装在游戏根目录下（即包含游戏主运行程序 .exe 的文件夹）。" 
        : "<b>📌 Game Installation Path</b><br><br>The currently scanned game folder. The AutoTranslator must be installed in the game's root directory.";
        
    m_tipGameType = m_lang == 1 
        ? "<b>📌 Unity 游戏架构类型</b><br><br>• <b>Mono</b>: 传统 Unity 引擎架构，插件安装更为简单直接。<br>• <b>IL2CPP</b>: 预编译的高性能 C++ 架构，插件需要特定的 IL2CPP 桥接补丁（通常配合 BepInEx 6.x）。" 
        : "<b>📌 Unity Game Engine Architecture</b><br><br>• <b>Mono</b>: Traditional Unity backend. Straightforward installation.<br>• <b>IL2CPP</b>: Pre-compiled C++ backend. Requires IL2CPP-specific bridges.";
        
    m_tipUnityVersion = m_lang == 1 
        ? "<b>📌 Unity 引擎编辑器版本</b><br><br>通过解析 <code>UnityPlayer.dll</code> 获得的引擎底层版本。选择翻译插件或字重资源包时，某些版本需要特定的补丁兼容。" 
        : "<b>📌 Unity Engine Editor Version</b><br><br>The core engine version retrieved from <code>UnityPlayer.dll</code>.";
        
    m_tipXUnity = m_lang == 1 
        ? "<b>📌 XUnity.AutoTranslator 安装检测</b><br><br>检查 <code>XUnity.Common.dll</code> 是否存在。若未检测到，说明自动翻译器插件尚未安装，翻译服务将无法拦截游戏文本。" 
        : "<b>📌 XUnity AutoTranslator Check</b><br><br>Detects <code>XUnity.Common.dll</code>. If missing, text interception will not work.";
        
    m_tipGlossary = m_lang == 1 
        ? "<b>📌 术语表配置文件检测</b><br><br>检查 <code>_Substitutions.txt</code> 是否存在。它是 XUnity 的本地替换词典，本翻译器可以通过<b>自进化功能</b>自动维护和向其追加翻译条目。" 
        : "<b>📌 Glossary Configuration Check</b><br><br>Detects <code>_Substitutions.txt</code>. Used for the Self-Evolution dictionary feature.";
        
    m_tipFont = m_lang == 1 
        ? "<b>📌 字体资源检测</b><br><br>检测游戏目录中是否存在自定义字体（如 <code>.ttf</code>, <code>.otf</code> 或 Unity 字体资产包）。当游戏汉化出现缺字/乱码（□□□）时，必须配置中文字体。" 
        : "<b>📌 Font Asset Check</b><br><br>Checks for custom font files (<code>.ttf</code>, <code>.otf</code>). Essential for resolving empty squares (□□□).";

    rowGamePath.container->setToolTip(m_tipGamePath);
    rowGameType.container->setToolTip(m_tipGameType);
    rowUnityVersion.container->setToolTip(m_tipUnityVersion);
    rowXUnity.container->setToolTip(m_tipXUnity);
    rowGlossary.container->setToolTip(m_tipGlossary);
    rowFont.container->setToolTip(m_tipFont);

    if (!m_hasScanned) {
        lblSummaryIcon->setText("—");
        lblSummaryTitle->setText(m_lang == 1 ? "等待环境检测" : "Waiting for Scan");
        lblSummaryPath->setText(m_lang == 1 ? "请点击上方按钮选择游戏目录" : "Please select a game folder above");
        lblSummaryCount->setText("");
        
        lblBasicGroupCount->setText(m_lang == 1 ? "等待检测" : "Pending");
        lblPluginGroupCount->setText(m_lang == 1 ? "等待检测" : "Pending");

        QString pendingTxt = m_lang == 1 ? "等待检测..." : "Pending...";
        QString guideTxt = m_lang == 1 ? "说明" : "Guide";
        
        updateRowState(rowGamePath, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowGameType, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowUnityVersion, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowXUnity, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowGlossary, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowFont, "pending", pendingTxt, guideTxt, "secondary");
    }

    linkXUnity->updateContent(m_lang == 1 ? "XUnity 安装教程" : "XUnity Setup", m_lang == 1 ? "下载、部署和首次启动说明" : "Download, install and first run", "https://github.com/bbepis/XUnity.AutoTranslator", m_lang == 1 ? "<b>🛠️ XUnity 安装教程</b><br><br>跳转至项目页面阅读完整文档。" : "<b>🛠️ XUnity Setup</b><br><br>Go to project repo.");
    linkGlossary->updateContent(m_lang == 1 ? "术语表打包模式" : "Glossary & Batch", m_lang == 1 ? "启用替换词典和重定向目录" : "Enable substitution & redirects", "", m_lang == 1 ? "<b>📖 术语表说明</b><br><br>1. 修改 AutoTranslatorConfig.ini 中 EnableSubstitution=True<br>2. 将词库放到 Redirects 目录下<br>3. 在主界面勾选 [SE] 模式" : "<b>📖 Glossary Guide</b><br><br>EnableSubstitution=True in config.");
    linkFont->updateContent(m_lang == 1 ? "中文字体方案" : "Font Overrides", m_lang == 1 ? "解决方块字、缺字和乱码" : "Fix missing characters & blocks", "https://github.com/sorrowmoil/sorrowmoil-MoeFont-for-XUnity.AutoTranslator", m_lang == 1 ? "<b>🔤 字体替换方案</b><br><br>在游戏中遇到 □□□ 缺字时，由此获取 fallback_fonts 字重包。" : "<b>🔤 Font Overrides</b><br><br>Get fallback_fonts bundle here.");
    linkFaq->updateContent(m_lang == 1 ? "常见问题排查" : "FAQ & Troubleshoot", m_lang == 1 ? "端口冲突、无反应等疑难解答" : "Port conflicts and silent failures", "", m_lang == 1 ? "<b>❓ 常见问题排查</b><br><br>问题：毫无反应？<br>解决：请检查游戏 config 中 Endpoint 端口是否与本软件一致。" : "<b>❓ FAQ</b><br><br>Check Endpoint port in config matches.");
}

void EnvScanWindow::onSelectGameFolderClicked()
{
    QString title = (m_lang == 1) ? "选择游戏根目录" : "Select Game Root Directory";
    QString dir = QFileDialog::getExistingDirectory(this, title, "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_gameFolderPath = dir;
        m_hasScanned = true;
        performScan();
    }
}

void EnvScanWindow::performScan()
{
    if (m_gameFolderPath.isEmpty()) return;
    QDir gameDir(m_gameFolderPath);

    int basicSuccess = 0;
    int pluginSuccess = 0;
    int totalIssues = 0;

    // 1. 游戏路径
    updateRowState(rowGamePath, "success", gameDir.dirName(), m_lang == 1 ? "说明" : "Guide", "secondary");
    rowGamePath.actionBtn->disconnect();
    connect(rowGamePath.actionBtn, &QPushButton::clicked, this, [this](){ showFloatingTooltip(rowGamePath.actionBtn, m_tipGamePath); });
    basicSuccess++;

    // 2. 游戏架构与 BepInEx 框架检测
    bool isIl2cpp = false;
    for (const QString &dataDir : gameDir.entryList(QStringList() << "*_Data", QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (QDir(gameDir.absoluteFilePath(dataDir)).exists("il2cpp_data")) {
            isIl2cpp = true;
            break;
        }
    }

    bool hasBepInExEngine = QFile::exists(gameDir.absoluteFilePath("BepInEx/core/BepInEx.Core.dll"));
    bool hasMelonCore = QFile::exists(gameDir.absoluteFilePath("MelonLoader/MelonLoader.dll")) || QDir(gameDir.absoluteFilePath("MelonLoader/net6")).exists();
    bool hasBepInEx = hasBepInExEngine || hasMelonCore;

    if (isIl2cpp) {
        if (hasBepInEx) {
            updateRowState(rowGameType, "success", "IL2CPP · " + QString(m_lang == 1 ? "已就绪" : "Ready"), m_lang == 1 ? "说明" : "Guide", "secondary");
            rowGameType.actionBtn->disconnect();
            connect(rowGameType.actionBtn, &QPushButton::clicked, this, [this](){ showFloatingTooltip(rowGameType.actionBtn, m_tipGameType); });
            basicSuccess++;
        } else {
            updateRowState(rowGameType, "danger", "IL2CPP · " + QString(m_lang == 1 ? "无框架" : "Missing"), m_lang == 1 ? "部署" : "Install", "primary");
            rowGameType.actionBtn->disconnect();
            connect(rowGameType.actionBtn, &QPushButton::clicked, this, [this](){
                QString targetZip = "https://github.com/sorrowmoil/Moil-s-XUnity-LLMTranslateGUI/releases/download/Automated-installation/BepInEx-Unity.IL2CPP-win-x64-6.0.0-be.785+6abdba4.zip";
                QString url = getLoadBalancedUrl(targetZip);
                startDownloadAndExtract(url, "BepInEx_Framework.zip", rowGameType.statusLabel, rowGameType.actionBtn, m_lang == 1 ? "部署成功" : "Installed");
            });
            totalIssues++;
        }
    } else {
        updateRowState(rowGameType, "success", "Mono", m_lang == 1 ? "说明" : "Guide", "secondary");
        rowGameType.actionBtn->disconnect();
        connect(rowGameType.actionBtn, &QPushButton::clicked, this, [this](){ showFloatingTooltip(rowGameType.actionBtn, m_tipGameType); });
        basicSuccess++;
    }

    // 3. Unity 版本
    QString targetFileForVersion;
    QString unityPlayerPath = gameDir.absoluteFilePath("UnityPlayer.dll");
    if (QFile::exists(unityPlayerPath)) {
        targetFileForVersion = unityPlayerPath;
    } else {
        QStringList dataDirsForExe = gameDir.entryList(QStringList() << "*_Data", QDir::Dirs | QDir::NoDotAndDotDot);
        if (!dataDirsForExe.isEmpty()) {
            QString dataDirName = dataDirsForExe.first();
            QString baseExeName = dataDirName.left(dataDirName.length() - 5) + ".exe";
            QString possibleExePath = gameDir.absoluteFilePath(baseExeName);
            if (QFile::exists(possibleExePath)) targetFileForVersion = possibleExePath;
        }
        if (targetFileForVersion.isEmpty()) {
            QStringList exes = gameDir.entryList(QStringList() << "*.exe", QDir::Files);
            for (const QString &exe : exes) {
                if (!exe.contains("CrashHandler", Qt::CaseInsensitive) && !exe.contains("unins", Qt::CaseInsensitive)) {
                    targetFileForVersion = gameDir.absoluteFilePath(exe);
                    break;
                }
            }
        }
    }

    if (!targetFileForVersion.isEmpty()) {
        QString version = getUnityPlayerVersion(targetFileForVersion);
        if (version != (m_lang == 1 ? "未知版本" : "Unknown Version")) {
            updateRowState(rowUnityVersion, "success", version, m_lang == 1 ? "说明" : "Guide", "secondary");
            basicSuccess++;
        } else {
            updateRowState(rowUnityVersion, "warning", version, m_lang == 1 ? "说明" : "Guide", "secondary");
            totalIssues++;
        }
    } else {
        updateRowState(rowUnityVersion, "warning", m_lang == 1 ? "未找到核心" : "Not Found", m_lang == 1 ? "说明" : "Guide", "secondary");
        totalIssues++;
    }
    
    rowUnityVersion.actionBtn->disconnect();
    connect(rowUnityVersion.actionBtn, &QPushButton::clicked, this, [this](){ showFloatingTooltip(rowUnityVersion.actionBtn, m_tipUnityVersion); });

    // 4. 插件与字体遍历
    bool hasXUnity = false, hasGlossary = false;
    QStringList foundFonts;
    static const QRegularExpression extlessFontRegex("^(.+)\\s+(5\\.[0-9]|201[7-9]|202[0-3]|6000)$");

    QDirIterator it(m_gameFolderPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString fileName = it.fileName();
        if (!hasXUnity && fileName.compare("XUnity.Common.dll", Qt::CaseInsensitive) == 0) hasXUnity = true;
        if (!hasGlossary && fileName.compare("_Substitutions.txt", Qt::CaseInsensitive) == 0) hasGlossary = true;
        if (fileName.endsWith(".ttf", Qt::CaseInsensitive) || fileName.endsWith(".otf", Qt::CaseInsensitive)) {
            QString name = fileName.left(fileName.lastIndexOf('.'));
            if (!foundFonts.contains(name)) foundFonts.append(name);
        } else if (!fileName.contains('.')) {
            if (extlessFontRegex.match(fileName).hasMatch() && !foundFonts.contains(fileName)) foundFonts.append(fileName);
        }
    }

    if (hasXUnity) {
        updateRowState(rowXUnity, "success", m_lang == 1 ? "已安装" : "Installed", m_lang == 1 ? "说明" : "Guide", "secondary");
        rowXUnity.actionBtn->disconnect();
        connect(rowXUnity.actionBtn, &QPushButton::clicked, this, [this](){ showFloatingTooltip(rowXUnity.actionBtn, m_tipXUnity); });
        pluginSuccess++;
    } else {
        updateRowState(rowXUnity, "danger", m_lang == 1 ? "未安装" : "Not Inst", m_lang == 1 ? "部署" : "Install", "primary");
        rowXUnity.actionBtn->disconnect();
        connect(rowXUnity.actionBtn, &QPushButton::clicked, this, [this, isIl2cpp](){
            QString targetZip = isIl2cpp
                ? "https://github.com/sorrowmoil/Moil-s-XUnity-LLMTranslateGUI/releases/download/Automated-installation/XUnity.AutoTranslator-BepInEx-IL2CPP-5.6.1.zip"
                : "https://github.com/sorrowmoil/Moil-s-XUnity-LLMTranslateGUI/releases/download/Automated-installation/XUnity.AutoTranslator-ReiPatcher-5.6.1.zip";
            QString url = getLoadBalancedUrl(targetZip);
            if (isIl2cpp) {
                startDownloadAndExtract(url, "XUnity_Plugin.zip", rowXUnity.statusLabel, rowXUnity.actionBtn, m_lang == 1 ? "部署成功" : "Installed", "");
            } else {
                startDownloadAndExtract(url, "XUnity_ReiPatcher.zip", rowXUnity.statusLabel, rowXUnity.actionBtn, m_lang == 1 ? "补丁已应用" : "Patch Applied", "SetupReiPatcherAndAutoTranslator.exe");
            }
        });
        totalIssues++;
    }

    if (hasGlossary) {
        updateRowState(rowGlossary, "success", m_lang == 1 ? "已找到" : "Found", m_lang == 1 ? "说明" : "Guide", "secondary");
        pluginSuccess++;
    } else {
        updateRowState(rowGlossary, "warning", m_lang == 1 ? "尚未建立" : "Not Found", m_lang == 1 ? "说明" : "Guide", "secondary");
    }
    rowGlossary.actionBtn->disconnect();
    connect(rowGlossary.actionBtn, &QPushButton::clicked, this, [this](){ showFloatingTooltip(rowGlossary.actionBtn, m_tipGlossary); });

    if (!foundFonts.isEmpty()) {
        updateRowState(rowFont, "success", foundFonts.size() > 1 ? foundFonts[0] + " | ..." : foundFonts[0], m_lang == 1 ? "说明" : "Guide", "secondary");
        
        QString listHtml; for (const QString &f : foundFonts) listHtml += "<br>• " + f;
        QString combinedTip = m_tipFont + (m_lang == 1 ? "<br><br><b>已检测到字体:</b>" : "<br><br><b>Found:</b>") + listHtml;
        rowFont.container->setToolTip(combinedTip);
        
        rowFont.actionBtn->disconnect();
        connect(rowFont.actionBtn, &QPushButton::clicked, this, [this, combinedTip](){ showFloatingTooltip(rowFont.actionBtn, combinedTip); });
        pluginSuccess++;
    } else {
        updateRowState(rowFont, "warning", m_lang == 1 ? "无自定义字体" : "No Custom Font", m_lang == 1 ? "配置" : "Config", "secondary");
        rowFont.actionBtn->disconnect();
        connect(rowFont.actionBtn, &QPushButton::clicked, this, [this](){ showFloatingTooltip(rowFont.actionBtn, m_tipFont); });
        totalIssues++;
    }

    QString strLangOk = m_lang == 1 ? QString("正常") : QString("OK");
    lblBasicGroupCount->setText(QString("%1 / 3 %2").arg(basicSuccess).arg(strLangOk));
    lblPluginGroupCount->setText(QString("%1 / 3 %2").arg(pluginSuccess).arg(strLangOk));

    // 6. 更新总体状态摘要
    lblSummaryPath->setText(QDir::toNativeSeparators(m_gameFolderPath));
    
    if (totalIssues == 0) {
        lblSummaryIcon->setText("✓");
        lblSummaryTitle->setText(m_lang == 1 ? "扫描完成，环境就绪" : "Scan Complete, Ready");
        lblSummaryCount->setText(m_lang == 1 ? "均通过" : "All passed");
        frameSummary->setProperty("state", "success");
        lblSummaryIcon->setProperty("state", "success");
    } else if (!hasXUnity || (!isIl2cpp && !hasBepInEx)) {
        lblSummaryIcon->setText("!");
        lblSummaryTitle->setText(m_lang == 1 ? "环境未就绪，需处理严重问题" : "Not Ready, Critical Issues");
        lblSummaryCount->setText(QString(m_lang == 1 ? "%1 项待处理" : "%1 issues").arg(totalIssues));
        frameSummary->setProperty("state", "danger");
        lblSummaryIcon->setProperty("state", "danger");
    } else {
        lblSummaryIcon->setText("!");
        lblSummaryTitle->setText(m_lang == 1 ? "环境基本可用，有优化空间" : "Basically Ready");
        lblSummaryCount->setText(QString(m_lang == 1 ? "%1 项待处理" : "%1 issues").arg(totalIssues));
        frameSummary->setProperty("state", "warning");
        lblSummaryIcon->setProperty("state", "warning");
    }
    
    frameSummary->style()->unpolish(frameSummary);
    frameSummary->style()->polish(frameSummary);
    lblSummaryIcon->style()->unpolish(lblSummaryIcon);
    lblSummaryIcon->style()->polish(lblSummaryIcon);
}

QString EnvScanWindow::getUnityPlayerVersion(const QString &dllPath)
{
    std::wstring wstr = dllPath.toStdWString();
    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeW(wstr.c_str(), &dummy);
    if (size == 0) return m_lang == 1 ? "未知版本" : "Unknown Version";

    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(wstr.c_str(), 0, size, data.data()))
        return m_lang == 1 ? "未知版本" : "Unknown Version";

    VS_FIXEDFILEINFO *fileInfo = nullptr;
    UINT fileInfoSize;
    if (VerQueryValueW(data.data(), L"\\", (LPVOID *)&fileInfo, &fileInfoSize)) {
        return QString("%1.%2.%3.%4").arg(HIWORD(fileInfo->dwFileVersionMS)).arg(LOWORD(fileInfo->dwFileVersionMS)).arg(HIWORD(fileInfo->dwFileVersionLS)).arg(LOWORD(fileInfo->dwFileVersionLS));
    }
    return m_lang == 1 ? "未知版本" : "Unknown Version";
}

void EnvScanWindow::startDownloadAndExtract(const QString &url, const QString &zipName, QLabel *lblStatus, QPushButton *btnAction, const QString &successMsg, const QString &autoRunExe)
{
    btnAction->setEnabled(false);
    
    QString tempPath = QDir::tempPath() + "/" + zipName;
    QNetworkRequest request((QUrl(url)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_netMgr->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, [=](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            int percent = (bytesReceived * 100) / bytesTotal;
            btnAction->setText(QString("%1%").arg(percent));
        } 
    });

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            btnAction->setText(m_lang == 1 ? "部署中" : "Ext...");
            QCoreApplication::processEvents(); 

            QFile file(tempPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();

                if (extractZipNative(tempPath, m_gameFolderPath)) {
                    if (!autoRunExe.isEmpty()) {
                        QString exePath = QDir(m_gameFolderPath).absoluteFilePath(autoRunExe);
                        if (QFile::exists(exePath)) {
                            btnAction->setText(m_lang == 1 ? "执行中" : "Exec...");
                            QCoreApplication::processEvents();
                            QProcess::startDetached(exePath, QStringList(), m_gameFolderPath);
                        }
                    }

                    lblStatus->setText(successMsg);
                    btnAction->setText(m_lang == 1 ? "完成" : "Done");
                    btnAction->setProperty("role", "secondary");
                    btnAction->style()->unpolish(btnAction);
                    btnAction->style()->polish(btnAction);
                    btnAction->setEnabled(true);
                    
                    QTimer::singleShot(2000, this, [this]() { performScan(); });
                } else {
                    btnAction->setText(m_lang == 1 ? "解压失败" : "Failed");
                    btnAction->setEnabled(true); 
                }
                file.remove(); 
            }
        } else {
            btnAction->setText(m_lang == 1 ? "下载失败" : "Failed");
            btnAction->setEnabled(true); 
        }
        reply->deleteLater(); 
    });
}

bool EnvScanWindow::extractZipNative(const QString &zipPath, const QString &destDir)
{
    QProcess process;
    QStringList args;
    args << "-xf" << QDir::toNativeSeparators(zipPath) << "-C" << QDir::toNativeSeparators(destDir);
    process.start("tar", args);
    if (process.waitForFinished(30000)) {
        return process.exitCode() == 0;
    }
    return false;
}