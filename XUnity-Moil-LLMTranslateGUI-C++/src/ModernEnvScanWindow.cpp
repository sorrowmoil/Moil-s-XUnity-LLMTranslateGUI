#include "ModernEnvScanWindow.h"
#include "ModernWindow.h"
#include "ModernUI.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QDir>
#include <QToolTip>
#include <QDesktopServices>
#include <QUrl>
#include <windows.h>
#include <winver.h>
#include <QRegularExpression>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QEventLoop>
#include <QGuiApplication>
#include <QDirIterator>
#include <QRandomGenerator>
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
// 🚀 流光模式专属紧凑链接 (增强版适配经典模式说明)
// ==========================================
class ModernCompactLink : public QWidget
{
public:
    ModernCompactLink(const QString &icon, const QString &url, bool isDark, QWidget *parent = nullptr)
        : QWidget(parent), m_url(url)
    {
        setCursor(Qt::PointingHandCursor);

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(6, 4, 6, 4);
        layout->setSpacing(8);

        m_icon = new QLabel(icon, this);
        m_icon->setStyleSheet("border: none; background: transparent; font-size: 13px;");
        m_icon->setFixedWidth(20);
        m_icon->setAlignment(Qt::AlignCenter);

        m_title = new QLabel(this);
        m_title->setFixedWidth(105);
        m_title->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

        m_desc = new QLabel(this);
        m_desc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        m_arrow = new QLabel("›", this);
        m_arrow->setStyleSheet("border: none; background: transparent; font-size: 14px; font-family: Arial;");

        layout->addWidget(m_icon);
        layout->addWidget(m_title);
        layout->addWidget(m_desc);
        layout->addWidget(m_arrow);

        updateTheme(isDark);
    }

    void updateTheme(bool isDark)
    {
        QString titleColor = isDark ? "#EAEAEA" : "#222222";
        QString descColor = isDark ? "rgba(255,255,255,150)" : "rgba(0,0,0,150)";
        QString arrowColor = isDark ? "rgba(255,255,255,80)" : "rgba(0,0,0,80)";

        m_icon->setStyleSheet(QString("border: none; background: transparent; font-size: 13px; color: %1;").arg(arrowColor));
        m_title->setStyleSheet(QString("border: none; background: transparent; font-weight: bold; font-size: 11px; color: %1;").arg(titleColor));
        m_desc->setStyleSheet(QString("border: none; background: transparent; font-size: 10px; color: %1;").arg(descColor));
        m_arrow->setStyleSheet(QString("border: none; background: transparent; font-size: 14px; font-family: Arial; color: %1;").arg(arrowColor));
    }

    void updateText(const QString &title, const QString &desc, const QString &tooltipText)
    {
        m_title->setText(title);
        m_desc->setText(desc);
        setToolTip(tooltipText);
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (!m_url.isEmpty()) QDesktopServices::openUrl(QUrl(m_url));
        }
        QWidget::mouseReleaseEvent(event);
    }

    void enterEvent(QEnterEvent *) override
    {
        m_title->setStyleSheet(m_title->styleSheet().replace("text-decoration: none", "text-decoration: underline"));
    }

    void leaveEvent(QEvent *) override
    {
        m_title->setStyleSheet(m_title->styleSheet().replace("text-decoration: underline", "text-decoration: none"));
    }

private:
    QString m_url;
    QLabel *m_icon;
    QLabel *m_title;
    QLabel *m_desc;
    QLabel *m_arrow;
};

// ==========================================
// 🚀 ModernEnvScanWindow 主体实现
// ==========================================

ModernEnvScanWindow::ModernEnvScanWindow(bool isDark, int lang, QWidget *parent)
    : QDialog(parent), m_isDark(isDark), m_lang(lang)
{
    m_netMgr = new QNetworkAccessManager(this);

    // 🔥 流光模式专属环境：无边框 + 背景透明
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 从 ModernWindow 获取渲染参数
    if (parent) {
        if (ModernWindow* modWin = qobject_cast<ModernWindow*>(parent)) {
            m_alpha = modWin->getAlpha();
            m_isRounded = modWin->getIsRounded();
            m_glassMode = static_cast<int>(modWin->getGlassRenderMode());
            m_hueShift = modWin->getHueShift();
            m_tintIntensity = modWin->getTintIntensity();
        }
    }

    setFixedWidth(440);
    setupUI();
    applyThemeStyle();
    retranslateUI();

    // 入场动画：从父窗口边缘滑入 + 淡入
    if (parent) {
        QRect parentRect = parent->geometry();
        QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();

        int w = 440;
        int h = std::min(parentRect.height() * 0.90, 720.0);
        int y = parentRect.top() + (parentRect.height() - h) / 2;
        int x = parentRect.right() + 8;

        if (x + w > screenRect.right()) {
            x = parentRect.left() - w - 8;
            if (x < screenRect.left()) {
                x = screenRect.right() - w - 8;
            }
        }
        if (y + h > screenRect.bottom()) y = screenRect.bottom() - h - 8;
        if (y < screenRect.top()) y = screenRect.top() + 8;

        m_finalRect = QRect(x, y, w, h);

        if (x > parentRect.x()) {
            m_startRect = QRect(parentRect.right(), y, 0, h);
        } else {
            m_startRect = QRect(parentRect.left(), y, 0, h);
        }
        setGeometry(m_startRect);
        setWindowOpacity(0.0);

        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        QPropertyAnimation *geoAnim = new QPropertyAnimation(this, "geometry");
        geoAnim->setDuration(400);
        geoAnim->setStartValue(m_startRect);
        geoAnim->setEndValue(m_finalRect);
        geoAnim->setEasingCurve(QEasingCurve::OutExpo);
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
        fadeAnim->setDuration(300);
        fadeAnim->setStartValue(0.0);
        fadeAnim->setEndValue(1.0);
        group->addAnimation(geoAnim);
        group->addAnimation(fadeAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

ModernEnvScanWindow::~ModernEnvScanWindow() {}

void ModernEnvScanWindow::animateClose()
{
    if (m_isClosing) return;
    m_isClosing = true;
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    QPropertyAnimation *geoAnim = new QPropertyAnimation(this, "geometry");
    geoAnim->setDuration(300);
    geoAnim->setStartValue(geometry());
    QRect endRect = geometry();
    endRect.setWidth(0);
    geoAnim->setEndValue(endRect);
    geoAnim->setEasingCurve(QEasingCurve::InExpo);
    QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
    fadeAnim->setDuration(250);
    fadeAnim->setStartValue(windowOpacity());
    fadeAnim->setEndValue(0.0);
    group->addAnimation(geoAnim);
    group->addAnimation(fadeAnim);
    connect(group, &QParallelAnimationGroup::finished, this, &QWidget::close);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void ModernEnvScanWindow::smoothSwitch(std::function<void()> changeLogic)
{
    QPoint globalPos = QCursor::pos();
    QPoint epicenter = this->mapFromGlobal(globalPos);

    RippleOverlay *overlay = new RippleOverlay(this->grab(), epicenter, this);
    overlay->setGeometry(this->rect());
    overlay->setStyleSheet("background: transparent; border: none;");
    overlay->show();
    overlay->raise();
    overlay->repaint();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    QTimer::singleShot(15, this, [this, overlay, globalPos, epicenter, changeLogic]()
    {
        changeLogic();
        overlay->m_isDarkTarget = this->m_isDark;
        this->repaint();

        int w = this->width();
        int h = this->height();
        double d1 = std::hypot((double)epicenter.x(), (double)epicenter.y());
        double d2 = std::hypot((double)(w - epicenter.x()), (double)epicenter.y());
        double d3 = std::hypot((double)(w - epicenter.x()), (double)(h - epicenter.y()));
        double d4 = std::hypot((double)epicenter.x(), (double)(h - epicenter.y()));
        float finalRadius = (float)std::max({d1, d2, d3, d4}) + 50.0f;

        QVariantAnimation *anim = new QVariantAnimation(this);
        anim->setDuration(550);
        anim->setStartValue(0.0f);
        anim->setEndValue(finalRadius);
        anim->setEasingCurve(QEasingCurve::InOutCubic);

        connect(anim, &QVariantAnimation::valueChanged, overlay, [overlay](const QVariant &val){
            overlay->setRadius(val.toFloat());
        });

        connect(anim, &QVariantAnimation::finished, [overlay, anim](){
            overlay->deleteLater();
            anim->deleteLater();
        });

        anim->start();
    });
}

void ModernEnvScanWindow::updateTheme(bool isDark)
{
    if (m_isDark == isDark) return;
    m_isDark = isDark;
    applyThemeStyle();
}

void ModernEnvScanWindow::updateLanguage(int lang)
{
    if (m_lang == lang) return;
    m_lang = lang;
    retranslateUI();
    if (m_hasScanned) performScan();
    else {
        QString pendingTxt = m_lang == 1 ? "等待检测" : "Pending Scan";
        QString guideTxt = m_lang == 1 ? "说明" : "Guide";
        updateRowState(rowGamePath, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowGameType, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowUnityVersion, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowXUnity, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowGlossary, "pending", pendingTxt, guideTxt, "secondary");
        updateRowState(rowFont, "pending", pendingTxt, guideTxt, "secondary");
    }
}

void ModernEnvScanWindow::updateRounded(bool isRounded)
{
    if (m_isRounded == isRounded) return;
    m_isRounded = isRounded;
    applyThemeStyle();
}

void ModernEnvScanWindow::updateAlpha(int alpha)
{
    if (m_alpha == alpha) return;
    m_alpha = alpha;
    applyThemeStyle();
}

void ModernEnvScanWindow::setGlassParams(int renderMode, int hueShift, int tintIntensity)
{
    m_glassMode = renderMode;
    m_hueShift = hueShift;
    m_tintIntensity = tintIntensity;
    applyThemeStyle();
}

void ModernEnvScanWindow::showFloatingTooltip(QPushButton *btn, const QString &text)
{
    QPoint globalPos = btn->mapToGlobal(QPoint(btn->width() - 240, btn->height() + 5));
    QToolTip::showText(globalPos, text, btn);
}

ModernScanRow ModernEnvScanWindow::createModernRow(const QString &objName)
{
    ModernScanRow row;
    row.container = new QWidget(m_scrollWidget);
    row.container->setObjectName(objName);
    row.container->setMinimumHeight(44);
    row.container->setCursor(Qt::WhatsThisCursor);

    QGridLayout *layout = new QGridLayout(row.container);
    layout->setContentsMargins(8, 4, 8, 4);
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

void ModernEnvScanWindow::updateRowState(ModernScanRow &row, const QString &state, const QString &statusText, const QString &btnText, const QString &btnRole)
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

void ModernEnvScanWindow::setupUI()
{
    QVBoxLayout *windowLayout = new QVBoxLayout(this);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    
    m_bgFrame = new QWidget(this);
    m_bgFrame->setObjectName("bgFrame");
    m_bgFrame->setAttribute(Qt::WA_NoSystemBackground, true);
    m_bgFrame->setAutoFillBackground(false);
    windowLayout->addWidget(m_bgFrame);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_bgFrame);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 自定义无边框标题栏
    m_titleBar = new QWidget(m_bgFrame);
    m_titleBar->setFixedHeight(36); 
    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(14, 0, 8, 0);
    
    m_titleLabel = new QLabel(m_titleBar);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    m_titleLabel->setFont(titleFont);
    
    m_btnClose = new QPushButton("✕", m_titleBar);
    m_btnClose->setFixedSize(30, 30);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    connect(m_btnClose, &QPushButton::clicked, this, &ModernEnvScanWindow::animateClose);
    
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_btnClose);
    mainLayout->addWidget(m_titleBar);

    // 平滑滚动区
    m_scrollArea = new QScrollArea(m_bgFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setObjectName("mainScrollArea");

    m_scrollWidget = new QWidget(m_scrollArea);
    m_scrollWidget->setObjectName("mainScrollWidget");
    QVBoxLayout *scrollLayout = new QVBoxLayout(m_scrollWidget);
    scrollLayout->setContentsMargins(14, 8, 14, 16);
    scrollLayout->setSpacing(12);

    btnSelectFolder = new QPushButton(m_scrollWidget);
    btnSelectFolder->setObjectName("btnSelectFolder");
    btnSelectFolder->setCursor(Qt::PointingHandCursor);
    btnSelectFolder->setMinimumHeight(40);
    scrollLayout->addWidget(btnSelectFolder);

    // ===================================
    // 摘要面板 (Summary)
    // ===================================
    frameSummary = new GlassCard(m_isDark, m_scrollWidget);
    m_glassCards.append(frameSummary);
    frameSummary->setObjectName("frameSummary");
    frameSummary->setProperty("state", "pending");
    
    QHBoxLayout *summaryLayout = new QHBoxLayout(frameSummary);
    summaryLayout->setContentsMargins(12, 10, 12, 10);
    summaryLayout->setSpacing(10);

    lblSummaryIcon = new QLabel("—", frameSummary);
    lblSummaryIcon->setObjectName("summaryIcon");
    lblSummaryIcon->setProperty("state", "pending");
    lblSummaryIcon->setFixedSize(26, 26);
    lblSummaryIcon->setAlignment(Qt::AlignCenter);

    QWidget *sumCenterWidget = new QWidget(frameSummary);
    sumCenterWidget->setStyleSheet("background: transparent; border: none;");
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
    scrollLayout->addWidget(frameSummary);

    // ===================================
    // 基础环境 (Basic Group)
    // ===================================
    QWidget *basicTitleWidget = new QWidget(m_scrollWidget);
    QHBoxLayout *basicTitleLayout = new QHBoxLayout(basicTitleWidget);
    basicTitleLayout->setContentsMargins(4, 4, 4, 0); 
    lblBasicGroupTitle = new QLabel(basicTitleWidget);
    lblBasicGroupTitle->setObjectName("groupTitle");
    lblBasicGroupCount = new QLabel(basicTitleWidget);
    lblBasicGroupCount->setObjectName("groupCount");
    lblBasicGroupCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    basicTitleLayout->addWidget(lblBasicGroupTitle);
    basicTitleLayout->addStretch();
    basicTitleLayout->addWidget(lblBasicGroupCount);
    scrollLayout->addWidget(basicTitleWidget);

    frameBasicGroup = new GlassCard(m_isDark, m_scrollWidget);
    m_glassCards.append(frameBasicGroup);
    frameBasicGroup->setObjectName("groupFrame");
    QVBoxLayout *basicLayout = new QVBoxLayout(frameBasicGroup);
    basicLayout->setContentsMargins(4, 4, 4, 4);
    basicLayout->setSpacing(2);

    rowGamePath = createModernRow("resultRow");
    rowGameType = createModernRow("resultRow");
    rowUnityVersion = createModernRow("resultRow");
    basicLayout->addWidget(rowGamePath.container);
    basicLayout->addWidget(rowGameType.container);
    basicLayout->addWidget(rowUnityVersion.container);
    scrollLayout->addWidget(frameBasicGroup);

    // ===================================
    // 翻译组件 (Plugin Group)
    // ===================================
    QWidget *pluginTitleWidget = new QWidget(m_scrollWidget);
    QHBoxLayout *pluginTitleLayout = new QHBoxLayout(pluginTitleWidget);
    pluginTitleLayout->setContentsMargins(4, 4, 4, 0); 
    lblPluginGroupTitle = new QLabel(pluginTitleWidget);
    lblPluginGroupTitle->setObjectName("groupTitle");
    lblPluginGroupCount = new QLabel(pluginTitleWidget);
    lblPluginGroupCount->setObjectName("groupCount");
    lblPluginGroupCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pluginTitleLayout->addWidget(lblPluginGroupTitle);
    pluginTitleLayout->addStretch();
    pluginTitleLayout->addWidget(lblPluginGroupCount);
    scrollLayout->addWidget(pluginTitleWidget);

    framePluginGroup = new GlassCard(m_isDark, m_scrollWidget);
    m_glassCards.append(framePluginGroup);
    framePluginGroup->setObjectName("groupFrame");
    QVBoxLayout *pluginLayout = new QVBoxLayout(framePluginGroup);
    pluginLayout->setContentsMargins(4, 4, 4, 4);
    pluginLayout->setSpacing(2);

    rowXUnity = createModernRow("resultRow");
    rowGlossary = createModernRow("resultRow");
    rowFont = createModernRow("resultRow");
    pluginLayout->addWidget(rowXUnity.container);
    pluginLayout->addWidget(rowGlossary.container);
    pluginLayout->addWidget(rowFont.container);
    scrollLayout->addWidget(framePluginGroup);

    // ===================================
    // 使用帮助 (Help Group)
    // ===================================
    QWidget *helpTitleWidget = new QWidget(m_scrollWidget);
    QHBoxLayout *helpTitleLayout = new QHBoxLayout(helpTitleWidget);
    helpTitleLayout->setContentsMargins(4, 4, 4, 0); 
    lblHelpGroupTitle = new QLabel(helpTitleWidget);
    lblHelpGroupTitle->setObjectName("groupTitle");
    lblHelpGroupSubtitle = new QLabel(helpTitleWidget);
    lblHelpGroupSubtitle->setObjectName("groupCount");
    helpTitleLayout->addWidget(lblHelpGroupTitle);
    helpTitleLayout->addStretch();
    helpTitleLayout->addWidget(lblHelpGroupSubtitle);
    scrollLayout->addWidget(helpTitleWidget);

    frameHelpGroup = new GlassCard(m_isDark, m_scrollWidget);
    m_glassCards.append(frameHelpGroup);
    frameHelpGroup->setObjectName("groupFrame");
    QVBoxLayout *helpLayout = new QVBoxLayout(frameHelpGroup);
    helpLayout->setContentsMargins(4, 6, 4, 6);
    helpLayout->setSpacing(2);

    linkXUnity = new ModernCompactLink("?", "https://github.com/bbepis/XUnity.AutoTranslator", m_isDark, frameHelpGroup);
    linkGlossary = new ModernCompactLink("#", "", m_isDark, frameHelpGroup);
    linkFont = new ModernCompactLink("A", "https://github.com/sorrowmoil/sorrowmoil-MoeFont-for-XUnity.AutoTranslator", m_isDark, frameHelpGroup);
    linkFaq = new ModernCompactLink("!", "", m_isDark, frameHelpGroup);

    helpLayout->addWidget(linkXUnity);
    helpLayout->addWidget(linkGlossary);
    helpLayout->addWidget(linkFont);
    helpLayout->addWidget(linkFaq);
    scrollLayout->addWidget(frameHelpGroup);

    m_scrollArea->setWidget(m_scrollWidget);
    mainLayout->addWidget(m_scrollArea);

    connect(btnSelectFolder, &QPushButton::clicked, this, &ModernEnvScanWindow::onSelectGameFolderClicked);
}

void ModernEnvScanWindow::applyThemeStyle()
{
    QString windowBg = m_isDark ? "rgba(30, 30, 35, 0)" : "rgba(255, 255, 255, 0)";
    QString panelBg = m_isDark ? "rgba(40, 42, 46, 120)" : "rgba(255, 255, 255, 140)";
    QString rowHover = m_isDark ? "rgba(255, 255, 255, 15)" : "rgba(0, 0, 0, 8)";
    
    QString border = m_isDark ? "rgba(255, 255, 255, 20)" : "rgba(0, 0, 0, 20)";
    
    QString textMain = m_isDark ? "#EAEAEA" : "#222222";
    QString textSec = m_isDark ? "rgba(255,255,255,180)" : "rgba(0,0,0,180)";
    QString textMuted = m_isDark ? "rgba(255,255,255,120)" : "rgba(0,0,0,120)";

    // 流光主题 Accent 色调
    QString accent = m_isDark ? "#FF8C00" : "#9400D3";
    QString accentHover = m_isDark ? "#FFA500" : "#BA55D3";
    QString primaryText = "#FFFFFF";
    
    QString success = m_isDark ? "#81C784" : "#2E7D32";
    QString warning = m_isDark ? "#FFB74D" : "#E65100";
    QString danger = m_isDark ? "#E57373" : "#C62828";
    
    // 摘要卡片特判背景 (更强调玻璃模糊 + 颜色叠加)
    QString successSoft = m_isDark ? "rgba(129, 199, 132, 20)" : "rgba(46, 125, 50, 15)";
    QString warningSoft = m_isDark ? "rgba(255, 183, 77, 20)"  : "rgba(230, 81, 0, 15)";
    QString dangerSoft = m_isDark ? "rgba(229, 115, 115, 20)"  : "rgba(198, 40, 40, 15)";

    int r = m_isRounded ? 8 : 0;

    QString qss = QString(
        "QDialog { background: transparent; }"
        "QWidget#bgFrame { background: transparent; border: none; }"
        "QScrollArea#mainScrollArea, QWidget#mainScrollWidget { background: transparent; border: none; }"
        
        // 隐藏滚动条或非常细的毛玻璃滚动条
        "QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0; }"
        "QScrollBar::handle:vertical { background: %textMuted%; border-radius: 3px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        
        "QLabel { font-family: 'Segoe UI', 'Microsoft YaHei'; color: %textMain%; background: transparent; }"
        
        "QPushButton#btnSelectFolder { background: %panelBg%; border: 1px solid %border%; border-radius: %r%px; padding: 0 14px; text-align: left; font-size: 12px; font-weight: bold; color: %textMain%; }"
        "QPushButton#btnSelectFolder:hover { background: %rowHover%; border-color: %accent%; }"
        
        // --- 摘要区域特殊状态配色 ---
        "GlassCard#frameSummary[state=\"pending\"] { border-left: 3px solid %border%; }"
        "GlassCard#frameSummary[state=\"success\"] { border-left: 3px solid %success%; background-color: %successSoft%; }"
        "GlassCard#frameSummary[state=\"warning\"] { border-left: 3px solid %warning%; background-color: %warningSoft%; }"
        "GlassCard#frameSummary[state=\"danger\"] { border-left: 3px solid %danger%; background-color: %dangerSoft%; }"
        
        "QLabel#summaryIcon { font-size: 15px; font-weight: bold; border-radius: 13px; }"
        "QLabel#summaryIcon[state=\"pending\"] { color: %textMuted%; }"
        "QLabel#summaryIcon[state=\"success\"] { color: %success%; }"
        "QLabel#summaryIcon[state=\"warning\"] { color: %warning%; }"
        "QLabel#summaryIcon[state=\"danger\"] { color: %danger%; }"
        
        "QLabel#summaryTitle { font-size: 12px; font-weight: bold; color: %textMain%; }"
        "QLabel#summaryPath { font-size: 10px; color: %textSec%; }"
        "QLabel#summaryCount { font-size: 10px; color: %textSec%; }"
        
        "QLabel#groupTitle { font-size: 11px; font-weight: bold; color: %textMain%; letter-spacing: 1px; }"
        "QLabel#groupCount { font-size: 10px; color: %textMuted%; }"
        
        "QWidget#resultRow { background: transparent; border-radius: 6px; }"
        "QWidget#resultRow:hover { background: %rowHover%; }"
        
        "QLabel#rowTitle { font-size: 12px; font-weight: bold; color: %textMain%; }"
        "QLabel#rowSubtitle { font-size: 10px; color: %textMuted%; }"
        "QLabel#rowStatus { font-size: 11px; font-weight: bold; }"
        
        "QLabel#rowStatus[state=\"success\"] { color: %success%; }"
        "QLabel#rowStatus[state=\"warning\"] { color: %warning%; }"
        "QLabel#rowStatus[state=\"danger\"] { color: %danger%; }"
        "QLabel#rowStatus[state=\"pending\"] { color: %textSec%; }"
        
        "QLabel#stateIcon { font-size: 13px; font-weight: bold; border-radius: 12px; }"
        "QLabel#stateIcon[state=\"success\"] { color: %success%; background-color: %successSoft%; }"
        "QLabel#stateIcon[state=\"warning\"] { color: %warning%; background-color: %warningSoft%; }"
        "QLabel#stateIcon[state=\"danger\"] { color: %danger%; background-color: %dangerSoft%; }"
        "QLabel#stateIcon[state=\"pending\"] { color: %textMuted%; }"
        
        "QPushButton#actionBtn { background-color: transparent; border: 1px solid transparent; border-radius: %r%px; font-size: 11px; font-weight: bold; }"
        
        "QPushButton#actionBtn[role=\"secondary\"] { color: %accent%; padding: 4px 6px; }"
        "QPushButton#actionBtn[role=\"secondary\"]:hover { color: %accentHover%; text-decoration: underline; }"
        
        "QPushButton#actionBtn[role=\"primary\"] { color: %primaryText%; background-color: %accent%; border: 1px solid %accent%; padding: 4px 10px; }"
        "QPushButton#actionBtn[role=\"primary\"]:hover { background-color: %accentHover%; border-color: %accentHover%; }"
        "QPushButton#actionBtn:disabled { background-color: %border%; color: %textMuted%; border: none; }"
        
        "QToolTip { border: 1px solid %border%; background-color: %panelBg%; color: %textMain%; padding: 8px; border-radius: %r%px; font-size: 12px; }"
    )
    .replace("%textMain%", textMain)
    .replace("%textSec%", textSec)
    .replace("%panelBg%", panelBg)
    .replace("%border%", border)
    .replace("%rowHover%", rowHover)
    .replace("%accent%", accent)
    .replace("%textMuted%", textMuted)
    .replace("%success%", success)
    .replace("%warning%", warning)
    .replace("%danger%", danger)
    .replace("%successSoft%", successSoft)
    .replace("%warningSoft%", warningSoft)
    .replace("%dangerSoft%", dangerSoft)
    .replace("%accentHover%", accentHover)
    .replace("%primaryText%", primaryText)
    .replace("%r%", QString::number(r));

    setStyleSheet(qss);

    m_titleLabel->setStyleSheet(QString("color: %1; background: transparent; border: none;").arg(textMain));
    m_btnClose->setStyleSheet(QString("QPushButton { background: transparent; border: none; color: %1; font-size: 14px; }"
                                      "QPushButton:hover { background: #E81123; color: white; border-radius: %2px; }").arg(textMain).arg(r));

    linkXUnity->updateTheme(m_isDark);
    linkGlossary->updateTheme(m_isDark);
    linkFont->updateTheme(m_isDark);
    linkFaq->updateTheme(m_isDark);

    for (GlassCard *card : m_glassCards) {
        if (card) {
            card->setTheme(m_isDark);
            card->setAlpha(m_alpha);
            card->setRounded(m_isRounded);
            card->setRenderMode(static_cast<GlassRenderMode>(m_glassMode));
            card->setHueShift(m_hueShift);
        }
    }
}

void ModernEnvScanWindow::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int rds = m_isRounded ? 12 : 0; 
    QRect r = rect();

    if (m_glassMode == static_cast<int>(GlassRenderMode::Frosted)) {
        drawMenuGlassEffect(p, r, m_isDark, m_alpha, rds, m_hueShift, m_tintIntensity);
        p.setPen(QPen(QColor(255, 255, 255, m_isDark ? 30 : 100), 1));
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
    } else {
        drawLegacyGlowEffect(p, r, m_isDark, m_alpha, rds, 1.0f);
    }
}

void ModernEnvScanWindow::retranslateUI()
{
    m_titleLabel->setText(m_lang == 1 ? "环境扫描" : "Environment Scan");
    
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
    }

    linkXUnity->updateText(m_lang == 1 ? "XUnity 安装教程" : "XUnity Setup", m_lang == 1 ? "下载、部署和首次启动说明" : "Download, install and first run", m_lang == 1 ? "<b>🛠️ XUnity 安装教程</b><br><br>跳转至项目页面阅读完整文档。" : "<b>🛠️ XUnity Setup</b><br><br>Go to project repo.");
    linkGlossary->updateText(m_lang == 1 ? "术语表打包模式" : "Glossary & Batch", m_lang == 1 ? "启用替换词典和重定向目录" : "Enable substitution & redirects", m_lang == 1 ? "<b>📖 术语表说明</b><br><br>1. 修改 AutoTranslatorConfig.ini 中 EnableSubstitution=True<br>2. 将词库放到 Redirects 目录下<br>3. 在主界面勾选 [SE] 模式" : "<b>📖 Glossary Guide</b><br><br>EnableSubstitution=True in config.");
    linkFont->updateText(m_lang == 1 ? "中文字体方案" : "Font Overrides", m_lang == 1 ? "解决方块字、缺字和乱码" : "Fix missing characters & blocks", m_lang == 1 ? "<b>🔤 字体替换方案</b><br><br>在游戏中遇到 □□□ 缺字时，由此获取 fallback_fonts 字重包。" : "<b>🔤 Font Overrides</b><br><br>Get fallback_fonts bundle here.");
    linkFaq->updateText(m_lang == 1 ? "常见问题排查" : "FAQ & Troubleshoot", m_lang == 1 ? "端口冲突、无反应等疑难解答" : "Port conflicts and silent failures", m_lang == 1 ? "<b>❓ 常见问题排查</b><br><br>问题：毫无反应？<br>解决：请检查游戏 config 中 Endpoint 端口是否与本软件一致。" : "<b>❓ FAQ</b><br><br>Check Endpoint port in config matches.");
}

void ModernEnvScanWindow::onSelectGameFolderClicked()
{
    QString title = (m_lang == 1) ? "选择游戏根目录" : "Select Game Root Directory";
    QString dir = QFileDialog::getExistingDirectory(this, title, "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_gameFolderPath = dir;
        m_hasScanned = true;
        performScan();
    }
}

void ModernEnvScanWindow::performScan()
{
    if (m_gameFolderPath.isEmpty()) return;
    QDir gameDir(m_gameFolderPath);

    int basicSuccess = 0;
    int pluginSuccess = 0;
    int totalIssues = 0;

    // 🎯 修复无截断问题：实例化字体测量器，为行项目设定最大安全宽度（约 140px）
    QFontMetrics fmRow(rowGamePath.statusLabel->font());
    QString elidedFolderName = fmRow.elidedText(gameDir.dirName(), Qt::ElideRight, 140);

    // 1. 游戏路径 (应用截断文本)
    updateRowState(rowGamePath, "success", elidedFolderName, m_lang == 1 ? "说明" : "Guide", "secondary");
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
        // 🎯 修复无截断问题：对于未知的长版本号做智能省略
        QString elidedVersion = fmRow.elidedText(version, Qt::ElideRight, 140);
        
        if (version != (m_lang == 1 ? "未知版本" : "Unknown Version")) {
            updateRowState(rowUnityVersion, "success", elidedVersion, m_lang == 1 ? "说明" : "Guide", "secondary");
            basicSuccess++;
        } else {
            updateRowState(rowUnityVersion, "warning", elidedVersion, m_lang == 1 ? "说明" : "Guide", "secondary");
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
        // 🎯 修复无截断问题：对于极长名字的字体名称同样实施截断
        QString rawFontText = foundFonts.size() > 1 ? foundFonts[0] + " | ..." : foundFonts[0];
        QString elidedFontText = fmRow.elidedText(rawFontText, Qt::ElideRight, 140);
        
        updateRowState(rowFont, "success", elidedFontText, m_lang == 1 ? "说明" : "Guide", "secondary");
        
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
    // 🎯 修复无截断问题：中置省略顶部的绝对路径，避免它把右侧统计信息挤出画面外
    QString fullPath = QDir::toNativeSeparators(m_gameFolderPath);
    QFontMetrics fmSum(lblSummaryPath->font());
    lblSummaryPath->setText(fmSum.elidedText(fullPath, Qt::ElideMiddle, 260)); 
    
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

QString ModernEnvScanWindow::getUnityPlayerVersion(const QString &dllPath)
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

void ModernEnvScanWindow::startDownloadAndExtract(const QString &url, const QString &zipName, QLabel *lblStatus, QPushButton *btnAction, const QString &successMsg, const QString &autoRunExe)
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

bool ModernEnvScanWindow::extractZipNative(const QString &zipPath, const QString &destDir)
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

void ModernEnvScanWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() < 50) {
        m_isDragging = true;
        m_dragPos = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
        event->accept();
    }
}

void ModernEnvScanWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

void ModernEnvScanWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDragging = false;
    event->accept();
}