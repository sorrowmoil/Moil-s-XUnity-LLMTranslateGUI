/**
 * ModernWindow.cpp - Implementation of the modern glass-style window.
 * ModernWindow.cpp - 现代玻璃风格窗口的实现。
 *
 * This file contains the ModernWindow class, which provides a frameless,
 * translucent UI with animated transitions, a glossary drawer, and integration
 * with the translation server.
 * 本文件包含ModernWindow类，提供无边框、半透明的UI，支持动画过渡、术语表抽屉以及与翻译服务器的集成。
 */

#include "ModernWindow.h"
#include <QPainterPath>
#include <QApplication>
#include <QFileDialog>
#include <QScrollBar>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QTextStream>
#include <QPropertyAnimation>
#include <QMenu>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QTextCursor>
#include <QTextDocument>
#include <QMessageBox>
#include "ConfigManager.h"
#include "json.hpp"
#include "LogManager.h"
#include <functional>              // Required for std::function / 必须用于std::function
#include <QLabel>                  // Used for screenshot overlay / 用于截图覆盖层
#include <QPixmap>                 // Used for screen capture / 用于捕获屏幕
#include <QParallelAnimationGroup> // Used for simultaneous scaling and fading / 用于同时执行缩放和淡出
#include <QStyle>
#include <QLinearGradient>
#include <QFontDatabase>

#include <QSyntaxHighlighter>
#include "GlossaryManager.h"
#include "ModernUI.h"

// ==========================================
// Event filter to block right‑click from expanding the combo‑box dropdown.
// 事件过滤器，阻止右键点击时弹出下拉历史列表。
// ==========================================
class RightClickBlocker : public QObject
{
public:
    explicit RightClickBlocker(QObject *parent = nullptr) : QObject(parent) {}
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        // Intercept mouse press events.
        // 拦截鼠标按下事件。
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::RightButton)
            {
                return true; // Consume right‑click, preventing dropdown from opening.
                             // 吞噬右键按下事件，阻止下拉框展开。
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

// ==========================================
// External string references (defined in MainWindow.cpp)
// 外部字符串引用（定义在MainWindow.cpp中）
// ==========================================
extern const char *STR_API_ADDR[];
extern const char *STR_API_KEY[];
extern const char *STR_MODEL[];
extern const char *STR_PORT[];
extern const char *STR_THREAD[];
extern const char *STR_TEMP[];
extern const char *STR_CTX[];
extern const char *STR_SYS_PROMPT[];
extern const char *STR_PRE_PROMPT[];
extern const char *STR_GLOSSARY[];
extern const char *STR_CHK_GLOSSARY[];
extern const char *STR_LOCK_SYS[];
extern const char *STR_CLEAR_CTX[];
extern const char *STR_BTN_AUTO[];
extern const char *STR_START[];
extern const char *STR_RELOAD[];
extern const char *STR_STOP[];
extern const char *STR_TOKENS[];
extern const char *STR_FETCH[];
extern const char *STR_TEST[];
extern const char *STR_CLEAR_LOG[];
extern const char *STR_REMOVE_PATH[];
extern const char *STR_CLEAR_HISTORY[];
extern const char *TIP_TOKENS[];
extern const char *TIP_PORT[];
extern const char *TIP_THREAD[];
extern const char *TIP_TEMP[];
extern const char *TIP_CTX[];
extern const char *TIP_GLOSSARY[];
extern const char *TIP_LOCK_SYS[];
extern const char *TIP_BTN_AUTO[];
extern const char *TIP_CLEAR_CTX[];
extern const char *TIP_COMBO_MAIN[];
extern const char *LOG_TEST_START[];
extern const char *LOG_NO_KEY[];
extern const char *LOG_PARSE_ERR[];
extern const char *LOG_AUTO_TESTING[];
extern const char *LOG_MODEL_SWITCH[];
extern const char *LOG_MODEL_SAME[];
extern const char *LOG_RELOADED[];
extern const char *LOG_CFG_SAVED[];
extern const char *LOG_EXPORTED[];
extern const char *LOG_CFG_LOADED[];
extern const char *STR_BATCH_MODE[];
extern const char *TIP_BATCH_MODE[];

// Local API preset list (mirror of the one in MainWindow, but internal to this file)
// 本地API预设列表（与MainWindow中的相同，但属于本文件内部）
struct LocalApiPreset
{
    const char *url;
    const char *tips[2]; // [0]=English, [1]=Chinese
};

static const LocalApiPreset MODERN_PRESETS[] = {
    {"https://api.openai.com/v1", {"OpenAI Official API", "OpenAI 官方接口"}},
    {"https://api.deepseek.com", {"DeepSeek Official API", "DeepSeek 官方接口"}},
    {"https://api.openai.com/v1", {"OpenAI Official API", "OpenAI 官方接口"}},
    {"https://api.x.ai/v1", {"Grok (xAI) Official API", "Grok (xAI) 官方接口"}},
    {"https://api.deepseek.com", {"DeepSeek Official API", "DeepSeek 官方接口"}},
    {"https://api.siliconflow.cn/v1", {"SiliconFlow", "硅基流动 (SiliconFlow)"}},
    {"https://openrouter.ai/api/v1", {"OpenRouter Aggregator", "OpenRouter 聚合平台"}},
    {"https://generativelanguage.googleapis.com/v1beta/openai", {"Google Gemini", "Google Gemini (OpenAI 兼容端点)"}},
    {"http://localhost:11434/v1", {"Ollama Local Service", "Ollama 本地服务"}},
    {"http://localhost:1234/v1", {"LM Studio Local Service", "LM Studio 本地服务"}}};

// ==========================================
// Implementation of ModernWindow
// ModernWindow 的实现
// ==========================================

/**
 * Constructor. Initializes the window as frameless and translucent, sets up the UI,
 * and connects signals from the server and log manager.
 * 构造函数。将窗口初始化为无边框和半透明，设置UI，并连接服务器和日志管理器的信号。
 *
 * @param server Pointer to the translation server instance (shared with classic window).
 *              指向翻译服务器实例的指针（与经典窗口共享）。
 * @param parent Parent widget (usually nullptr).
 *              父窗口（通常为nullptr）。
 */
ModernWindow::ModernWindow(TranslationServer *server, QWidget *parent)
    : QMainWindow(parent), m_server(server), m_isServerRunning(false), m_alpha(210), m_lang(1), m_isDark(true)
{
    // Set frameless window flags and enable translucent background.
    // 设置无边框窗口标志并启用半透明背景。
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // Fixed size as requested: 500x832.
    // 固定尺寸为500x832。
    resize(500, 832);

    setupUi();

    if (m_server)
    {
        // Connect signals from the global log manager to the log display.
        // 连接全局日志管理器的信号到日志显示。
        connect(&LogManager::instance(), &LogManager::newLogAvailable, this, &ModernWindow::updateLog);
        connect(&LogManager::instance(), &LogManager::logsCleared, logArea, &QTextEdit::clear);

        m_tokenManager = new TokenManager(this);

        // 1. Server emits token usage -> TokenManager records it.
        // 1. 服务器发出token消耗信号 -> TokenManager记账。
        connect(m_server, &TranslationServer::tokenUsageReceived,
                m_tokenManager, &TokenManager::addUsage);

        // 2. TokenManager updates -> UI updates the token display.
        // 2. TokenManager更新 -> UI刷新token显示。
        connect(m_tokenManager, &TokenManager::tokensUpdated,
                this, &ModernWindow::updateToken);

        // Connect server start/stop signals to update the power button state.
        // 连接服务器启动/停止信号以更新电源按钮状态。
        connect(m_server, &TranslationServer::serverStarted, this, [this]()
                { updatePowerButtonState(true); });
        connect(m_server, &TranslationServer::serverStopped, this, [this]()
                { updatePowerButtonState(false); });
    }

    // Apply the initial style based on theme and rounded flag.
    // 根据主题和圆角标志应用初始样式。
    setStyleSheet(getModernStyle(m_isDark, m_isRounded));
    // The fade‑in animation has been moved to showEvent.
    // 淡入动画已移至showEvent。
}

ModernWindow::~ModernWindow() {}

/**
 * Overridden close event to perform a smooth exit animation (fade‑out).
 * 重写的关闭事件，执行平滑退出动画（淡出）。
 *
 * @param event The close event.
 */
void ModernWindow::closeEvent(QCloseEvent *event)
{
    // Check if this is the second invocation (after animation finished).
    // 检查是否为第二次调用（动画完成后）。
    if (this->property("finished_closing").toBool())
    {
        event->accept();
        return;
    }

    // 1. Intercept the close event.
    // 1. 拦截关闭事件。
    event->ignore();

    // 2. Save configuration early to avoid delay during exit.
    // 2. 提前保存配置，避免退出时延迟。
    ConfigManager::saveConfig(getUiConfig(), "config.ini");

    if (m_server)
        m_server->stopServer();

    // 3. Create a parallel animation group for exit effects.
    // 3. 创建并行动画组以执行退出效果。
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    // --- Animation A: Opacity fade‑out.
    // --- 动画A：透明度淡出。
    QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
    fadeAnim->setDuration(400);
    fadeAnim->setStartValue(this->windowOpacity());
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::InCubic);

    group->addAnimation(fadeAnim);

    // 4. After animation finishes, mark as finished and call close again.
    // 4. 动画结束后，标记为已完成并再次调用close。
    connect(group, &QParallelAnimationGroup::finished, this, [this]()
            {
                this->setProperty("finished_closing", true);
                this->close(); // This time it will accept.
                // 这次会接受关闭。
            });
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

/**
 * Return a user‑friendly error message based on HTTP status code.
 * 根据HTTP状态码返回用户友好的错误信息。
 *
 * @param code HTTP status code.
 * @param lang Language (0=English, 1=Chinese).
 * @return Friendly error string.
 */
QString ModernWindow::getFriendlyErrorMessage(int code, int lang)
{
    if (lang == 1) // Chinese
    {
        switch (code)
        {
        case 0:
            return "网络连接超时：无法连接到服务器，请检查 API 地址或网络代理。";
        case 400:
            return "请求格式错误 (400): 请检查参数设置是否符合该模型要求。";
        case 401:
            return "身份验证失败 (401): API Key 或是 API地址 填写错误、已过期或已被封禁。";
        case 402:
            return "额度不足 (402): 账户余额已耗尽，请前往官网充值。";
        case 403:
            return "拒绝访问 (403): 权限不足，或者该 Key 不支持访问所选模型。";
        case 404:
            return "地址错误 (404): API 地址或路径错误，请检查末尾是否多/少了 /v1。";
        case 429:
            return "频率限制/额度用尽 (429): 请求太快了，或者本月免费额度已领完。";
        case 500:
            return "服务器错误 (500): 供应商服务器崩溃了。";
        case 503:
            return "服务不可用 (503): 供应商正在维护中。";
        case 999:
            return "连接超时 (10s): 服务器响应太慢，请检查网络是否稳定。";
        default:
            return QString("网络错误 (%1): 请检查 API 地址是否有效。").arg(code);
        }
    }
    else // English
    {
        switch (code)
        {
        case 0:
            return "Connection Timeout: Please check your network or proxy.";
        case 400:
            return "Bad Request (400): Please check if parameters meet model requirements.";
        case 401:
            return "Auth Failed (401): Invalid or expired API Key/Address.";
        case 402:
            return "Insufficient Balance (402): Please top up your account.";
        case 403:
            return "Forbidden (403): Access denied or Key lacks model permissions.";
        case 429:
            return "Rate Limit (429): Too many requests or quota exhausted.";
        case 404:
            return "Not Found (404): Check your API URL.";
        case 500:
            return "Server Error (500): Provider server crashed.";
        case 503:
            return "Service Unavailable (503): Provider is under maintenance.";
        case 999:
            return "Connection Timeout (10s): Server response too slow.";
        default:
            return QString("Error (%1): Check provider status.").arg(code);
        }
    }
}

/**
 * Set up the user interface: create all widgets, layouts, and connect signals.
 * 设置用户界面：创建所有控件、布局并连接信号。
 */
void ModernWindow::setupUi()
{
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setObjectName("MainContainer");
    setCentralWidget(mainWidget);

    QVBoxLayout *rootLayout = new QVBoxLayout(mainWidget);
    // Compress margins to leave more space for content.
    // 压缩外边距，给内容留出更多空间。
    rootLayout->setContentsMargins(10, 5, 10, 10);
    rootLayout->setSpacing(5); // Reduce spacing between components.
                               // 减少组件间距。

    // --- Top window bar (minimize, close) ---
    // --- 顶部窗口栏（最小化、关闭） ---
    QHBoxLayout *winBar = new QHBoxLayout();
    btnMin = new QPushButton("—");
    btnMin->setObjectName("WinBtnMin");
    btnMin->setFixedSize(30, 30);
    connect(btnMin, &QPushButton::clicked, this, &ModernWindow::showMinimized);

    btnClose = new QPushButton("✕");
    btnClose->setObjectName("WinBtnClose");
    btnClose->setFixedSize(30, 30);
    connect(btnClose, &QPushButton::clicked, this, &ModernWindow::close);

    winBar->addStretch();
    winBar->addWidget(btnMin);
    winBar->addWidget(btnClose);
    rootLayout->addLayout(winBar);

    // --- Top toolbar (debug, load, save, language, theme, export, shape, back) ---
    // --- 顶部工具栏（调试、加载、保存、语言、主题、导出、形状、返回） ---
    QHBoxLayout *topBar = new QHBoxLayout();
    auto createIconBtn = [&](QString icon)
    {
        QPushButton *btn = new QPushButton(icon);
        btn->setFixedSize(38, 30);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("QPushButton { background: rgba(128, 128, 128, 20); border: 1px solid rgba(128, 128, 128, 30); border-radius: 6px; } QPushButton:hover { background: rgba(0, 229, 255, 30); border-color: #00e5ff; }");
        return btn;
    };

    btnLoad = createIconBtn("📂");
    connect(btnLoad, &QPushButton::clicked, this, &ModernWindow::onLoadConfig);
    btnSave = createIconBtn("💾");
    connect(btnSave, &QPushButton::clicked, this, &ModernWindow::onSaveConfig);
    btnLang = createIconBtn("🌐");
    connect(btnLang, &QPushButton::clicked, this, &ModernWindow::toggleLanguage);
    btnTheme = createIconBtn("🌓");
    connect(btnTheme, &QPushButton::clicked, this, &ModernWindow::toggleTheme);
    btnExport = createIconBtn("📜");
    connect(btnExport, &QPushButton::clicked, this, &ModernWindow::onExportLog);
    btnShape = createIconBtn("📐");
    connect(btnShape, &QPushButton::clicked, this, &ModernWindow::toggleShape);
    btnDebug = createIconBtn("⏱️");
    connect(btnDebug, &QPushButton::clicked, this, &ModernWindow::toggleDebugMode);

    topBar->addWidget(btnDebug);
    topBar->addWidget(btnLoad);
    topBar->addWidget(btnSave);
    topBar->addWidget(btnLang);
    topBar->addWidget(btnTheme);
    topBar->addWidget(btnExport);
    topBar->addWidget(btnShape);
    topBar->addStretch();

    btnBack = createIconBtn("🔙");
    connect(btnBack, &QPushButton::clicked, this, &ModernWindow::onSwitchClicked);
    topBar->addWidget(btnBack);
    rootLayout->addLayout(topBar);

    // --- Scrollable content area ---
    // --- 可滚动的内容区域 ---
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    // Disable horizontal scrollbar completely.
    // 彻底禁用横向滚动条。
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("background:transparent;");

    QWidget *content = new QWidget();
    content->setStyleSheet("background:transparent;");
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(2, 0, 2, 0); // Leave 2px gap left and right for border.
                                            // 左右留2px缝隙给边框。
    layout->setSpacing(6);                  // Spacing between cards.
                                            // 卡片间距。

    // Helper to create a glass card with a vertical layout.
    // 创建带有垂直布局的玻璃卡片的辅助函数。
    auto createCard = [&](QVBoxLayout *&vbox)
    {
        GlassCard *card = new GlassCard(m_isDark);
        vbox = new QVBoxLayout(card);
        vbox->setContentsMargins(10, 6, 10, 8); // Shrink margins.
        vbox->setSpacing(4);
        return card;
    };

    // 1. API Configuration card
    // 1. API配置卡片
    QVBoxLayout *v1;
    layout->addWidget(createCard(v1));
    QGridLayout *g1 = new QGridLayout();
    g1->setVerticalSpacing(4);
    g1->setHorizontalSpacing(6); // Lock column spacing to 6px.
                                 // 锁定列间距为6px。

    // Fixed widths for alignment (calculated to match the design).
    // 用于对齐的固定宽度（根据设计计算得出）。
    const int INPUT_LOCK_W = 365;
    const int BTN_FETCH_W = 60;
    const int MODEL_LOCK_W = INPUT_LOCK_W - BTN_FETCH_W - 6; // = 274px

    const int LABEL_W = 85;
    lblApi = new QLabel();
    lblApi->setFixedWidth(LABEL_W);
    lblKey = new QLabel();
    lblKey->setFixedWidth(LABEL_W);
    lblMod = new QLabel();
    lblMod->setFixedWidth(LABEL_W);

    // Row 0: API address
    // 第0行：API地址
    g1->addWidget(lblApi, 0, 0);
    apiAddressCombo = new SideGlassCombo();
    apiAddressCombo->setEnv(m_isDark, m_alpha, m_isRounded);
    apiAddressCombo->setEditable(true);
    apiAddressCombo->lineEdit()->setStyleSheet("padding: 0px; margin: 0px; background: transparent; border: none;");
    apiAddressCombo->lineEdit()->setTextMargins(3, 0, 0, 0);
    apiAddressCombo->setMinimumHeight(24);
    apiAddressCombo->setFixedWidth(INPUT_LOCK_W); // Lock total width.
                                                  // 锁死总宽度。
    int presetCount = sizeof(MODERN_PRESETS) / sizeof(LocalApiPreset);
    for (int k = 0; k < presetCount; ++k)
        apiAddressCombo->addItem(MODERN_PRESETS[k].url);
    g1->addWidget(apiAddressCombo, 0, 1, 1, 2); // Span columns 1 and 2.

    // Row 1: API key
    // 第1行：API密钥
    g1->addWidget(lblKey, 1, 0);
    apiKeyEdit = new QLineEdit();
    apiKeyEdit->setMinimumHeight(24);
    apiKeyEdit->setTextMargins(3, 0, 0, 0);
    apiKeyEdit->setFixedWidth(INPUT_LOCK_W);
    g1->addWidget(apiKeyEdit, 1, 1, 1, 2); // Span columns 1 and 2.

    // Row 2: Model name and fetch button
    // 第2行：模型名称和获取按钮
    g1->addWidget(lblMod, 2, 0);
    modelCombo = new SideGlassCombo();
    modelCombo->setEnv(m_isDark, m_alpha, m_isRounded);
    modelCombo->setEditable(true);
    modelCombo->lineEdit()->setStyleSheet("padding: 0px; margin: 0px; background: transparent; border: none;");
    modelCombo->lineEdit()->setTextMargins(3, 0, 0, 0);
    modelCombo->setMinimumHeight(24);
    modelCombo->setFixedWidth(MODEL_LOCK_W); // Use exact 274px.
                                             // 使用精确计算的274px。
    g1->addWidget(modelCombo, 2, 1);         // Only column 1.

    btnFetch = new QPushButton("获取");
    btnFetch->setObjectName("SmallFuncBtn");
    btnFetch->setFixedSize(BTN_FETCH_W, 24); // Fixed 60px width.
                                             // 固定60px宽度。
    connect(btnFetch, &QPushButton::clicked, this, &ModernWindow::onFetchModels);
    g1->addWidget(btnFetch, 2, 2); // Column 2.
    v1->addLayout(g1);

    // 2. Parameters card
    // 2. 参数卡片
    QVBoxLayout *v2;
    layout->addWidget(createCard(v2));
    QGridLayout *g2 = new QGridLayout();
    g2->setVerticalSpacing(4);
    portEdit = new QLineEdit();
    threadSpin = new QSpinBox();
    tempSpin = new QDoubleSpinBox();
    contextSpin = new QSpinBox();
    portEdit->setAlignment(Qt::AlignCenter);
    threadSpin->setRange(0, 1000);
    tempSpin->setRange(0, 2);
    contextSpin->setRange(0, 20);
    // All heights compressed to 24.
    // 所有高度压缩到24。
    portEdit->setFixedHeight(24);
    threadSpin->setFixedHeight(24);
    tempSpin->setFixedHeight(24);
    contextSpin->setFixedHeight(24);

    g2->addWidget(lblPrt = new QLabel(), 0, 0);
    g2->addWidget(portEdit, 0, 1);
    g2->addWidget(lblThd = new QLabel(), 0, 2);
    g2->addWidget(threadSpin, 0, 3);
    g2->addWidget(lblTmp = new QLabel(), 1, 0);
    g2->addWidget(tempSpin, 1, 1);
    g2->addWidget(lblCtx = new QLabel(), 1, 2);

    QHBoxLayout *ctxH = new QHBoxLayout();
    ctxH->addWidget(contextSpin, 1);
    btnClearCtx = new QPushButton();
    btnClearCtx->setObjectName("SmallFuncBtn");
    btnClearCtx->setFixedSize(45, 24);
    connect(btnClearCtx, &QPushButton::clicked, this, &ModernWindow::onClearContext);
    ctxH->addWidget(btnClearCtx);
    g2->addLayout(ctxH, 1, 3);
    v2->addLayout(g2);

    // 3. Prompts card
    // 3. 提示词卡片
    QVBoxLayout *v3;
    layout->addWidget(createCard(v3));
    QHBoxLayout *h3 = new QHBoxLayout();
    lblSys = new QLabel();
    h3->addWidget(lblSys);
    h3->addStretch();

    // Batch mode checkbox
    // 多行模式复选框
    chkBatch = new QCheckBox();
    connect(chkBatch, &QCheckBox::toggled, this, [this](bool checked)
            {
        QString msg = (m_lang == 1) ?
            QString("📦 多行模式: %1").arg(checked ? "已开启" : "已关闭") :
            QString("📦 Batch Mode: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);

        if (m_isServerRunning && m_server) {
            m_server->updateConfig(getUiConfig());
        } });
    h3->addWidget(chkBatch);
    h3->addSpacing(10); // Add some spacing.
                        // 添加一点间距。

    chkLockSysPrompt = new QCheckBox();
    h3->addWidget(chkLockSysPrompt);
    v3->addLayout(h3);

    systemPromptEdit = new QTextEdit();
    systemPromptEdit->setMinimumHeight(80); // Compress height.
                                            // 压缩高度。
    v3->addWidget(systemPromptEdit);
    v3->addWidget(lblPre = new QLabel());
    prePromptEdit = new QLineEdit();
    prePromptEdit->setFixedHeight(24);
    v3->addWidget(prePromptEdit);

    // 4. Glossary card
    // 4. 术语表卡片
    QVBoxLayout *v4;
    layout->addWidget(createCard(v4));
    QHBoxLayout *h4 = new QHBoxLayout();
    lblGlo = new QLabel();
    h4->addWidget(lblGlo);
    h4->addSpacing(10);
    chkLockGlossary = new QCheckBox();
    h4->addWidget(chkLockGlossary);
    h4->addStretch();
    // New: Edit glossary button.
    // 新增：编辑术语表按钮。
    btnEditGlossary = new QPushButton();
    btnEditGlossary->setObjectName("SmallFuncBtn");
    btnEditGlossary->setFixedSize(110, 24);
    connect(btnEditGlossary, &QPushButton::clicked, this, &ModernWindow::onEditGlossaryClicked);
    h4->addWidget(btnEditGlossary);

    // Existing: auto translations button.
    // 原有的：自动翻译按钮。
    btnEditAuto = new QPushButton();
    btnEditAuto->setObjectName("SmallFuncBtn");
    btnEditAuto->setFixedSize(64, 24);
    connect(btnEditAuto, &QPushButton::clicked, this, &ModernWindow::onOpenAutoTranslations);
    h4->addWidget(btnEditAuto);
    v4->addLayout(h4);

    QHBoxLayout *gloRow = new QHBoxLayout();
    chkGlossary = new QCheckBox();
    gloRow->addWidget(chkGlossary);
    glossaryCombo = new SideGlassCombo();
    glossaryCombo->setEnv(m_isDark, m_alpha, m_isRounded);
    glossaryCombo->setEditable(true);
    glossaryCombo->setFixedHeight(24);
    glossaryCombo->setFixedWidth(330); // Adjust width.
                                       // 微调宽度。

    // ==========================================
    // Prevent right‑click from opening the dropdown.
    // 防止右键点击时弹出下拉列表。
    // ==========================================
    glossaryCombo->installEventFilter(new RightClickBlocker(glossaryCombo));
    glossaryCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    glossaryCombo->lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(glossaryCombo, &QComboBox::customContextMenuRequested, this, &ModernWindow::onGlossaryContextMenu);
    connect(glossaryCombo->lineEdit(), &QLineEdit::customContextMenuRequested, this, &ModernWindow::onGlossaryContextMenu);

    connect(glossaryCombo, &QComboBox::activated, this, &ModernWindow::onGlossaryChanged);
    gloRow->addWidget(glossaryCombo);

    btnSelectGlossary = new QPushButton("...");
    btnSelectGlossary->setObjectName("SmallFuncBtn");
    btnSelectGlossary->setFixedSize(36, 24);
    connect(btnSelectGlossary, &QPushButton::clicked, this, &ModernWindow::onSelectGlossary);
    btnSelectGlossary->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(btnSelectGlossary, &QPushButton::customContextMenuRequested, this, &ModernWindow::onGlossaryContextMenu);
    gloRow->addWidget(btnSelectGlossary);
    gloRow->addStretch();
    v4->addLayout(gloRow);

    scroll->setWidget(content);
    rootLayout->addWidget(scroll, 1);

    // --- Bottom control bar (Test, Stop, Power) ---
    // --- 底部控制栏（测试、停止、电源） ---
    QHBoxLayout *mainCtrl = new QHBoxLayout();
    mainCtrl->setSpacing(8);
    const int CTRL_BTN_H = 34; // Slightly compressed.
                               // 稍微压扁。

    btnTest = new QPushButton();
    btnTest->setFixedHeight(CTRL_BTN_H);
    connect(btnTest, &QPushButton::clicked, this, &ModernWindow::onTestConfig);

    btnStop = new QPushButton();
    btnStop->setFixedHeight(CTRL_BTN_H);
    connect(btnStop, &QPushButton::clicked, this, &ModernWindow::onStopClicked);

    btnPower = new QPushButton();
    btnPower->setFixedHeight(CTRL_BTN_H);
    connect(btnPower, &QPushButton::clicked, this, &ModernWindow::onPowerClicked);

    mainCtrl->addWidget(btnTest, 1);
    mainCtrl->addWidget(btnStop, 1);
    mainCtrl->addWidget(btnPower, 1);
    rootLayout->addLayout(mainCtrl);

    lblTokens = new QLabel("TOKENS: 0");
    lblTokens->setAlignment(Qt::AlignCenter);
    lblTokens->setStyleSheet("color: #E6B422; font-weight: bold; font-size: 12px;");
    rootLayout->addWidget(lblTokens);

    logArea = new QTextEdit();
    logArea->setObjectName("LogArea");
    logArea->setReadOnly(true);
    logArea->setMinimumHeight(150); // Compress log area height.
                                    // 压缩日志区域高度。
    logArea->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(logArea, &QTextEdit::customContextMenuRequested, this, &ModernWindow::onLogContextMenu);
    logArea->document()->setDocumentMargin(0);
    rootLayout->addWidget(logArea);

    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(50, 255);
    m_opacitySlider->setValue(m_alpha); // Will be overridden by loadConfigToUi later.
                                        // 稍后会被loadConfigToUi覆盖。
    m_opacitySlider->setFixedHeight(14);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &ModernWindow::onOpacityChange);
    rootLayout->addWidget(m_opacitySlider);

    // Align cursor to the start after selection.
    // 选择后将光标移到开头。
    connect(glossaryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int)
            { QTimer::singleShot(0, [this]()
                                 { 
            if(glossaryCombo->lineEdit()) glossaryCombo->lineEdit()->setCursorPosition(0); }); });
    connect(apiAddressCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int)
            { QTimer::singleShot(0, [this]()
                                 { 
            if(apiAddressCombo->lineEdit()) apiAddressCombo->lineEdit()->setCursorPosition(0); }); });
}

/**
 * Update all UI texts according to the current language.
 * 根据当前语言更新所有UI文本。
 */
void ModernWindow::updateUIText()
{
    int i = m_lang;
    QString currentApi = apiAddressCombo->currentText();
    QSignalBlocker blocker(apiAddressCombo);

    lblApi->setText(STR_API_ADDR[i]);
    lblKey->setText(STR_API_KEY[i]);
    lblMod->setText(STR_MODEL[i]);
    lblPrt->setText(STR_PORT[i]);
    lblThd->setText(STR_THREAD[i]);
    lblTmp->setText(STR_TEMP[i]);
    lblCtx->setText(STR_CTX[i]);
    lblSys->setText(STR_SYS_PROMPT[i]);
    lblPre->setText(STR_PRE_PROMPT[i]);
    lblGlo->setText(STR_GLOSSARY[i]);
    chkGlossary->setText(STR_CHK_GLOSSARY[i]);
    chkLockSysPrompt->setText(STR_LOCK_SYS[i]);
    chkLockGlossary->setText(STR_LOCK_SYS[i]);
    btnClearCtx->setText(STR_CLEAR_CTX[i]);
    btnEditAuto->setText(STR_BTN_AUTO[i]);

    btnDebug->setToolTip(i == 1 ? "开启后，日志将显示底层端口类型和毫秒级耗时" : "Enable to show endpoint types and ms latency in logs");
    btnEditGlossary->setText(i == 1 ? "📝 术语表编辑" : "📝 Edit Terms");
    btnShape->setToolTip(i == 1 ? "切换窗口形状 (圆角/直角)" : "Toggle Window Shape (Rounded/Sharp)");
    btnEditGlossary->setToolTip(i == 1 ? "侧边滑出流光面板，极速编辑当前选择的术语表" : "Slide out glass panel to edit current glossary");

    btnFetch->setText(STR_FETCH[i]);
    btnTest->setText(STR_TEST[i]);
    btnStop->setText(STR_STOP[i]);

    // Set tooltips for all buttons.
    // 设置所有按钮的工具提示。
    btnLoad->setToolTip(i == 1 ? "读取配置文件 (.ini)" : "Load Configuration (.ini)");
    btnSave->setToolTip(i == 1 ? "保存当前配置到文件" : "Save Current Configuration");
    btnLang->setToolTip(i == 1 ? "切换语言 (中文/English)" : "Toggle Language (CN/EN)");
    btnTheme->setToolTip(i == 1 ? "切换流光背景主题" : "Toggle Glass Theme Style");
    btnExport->setToolTip(i == 1 ? "导出当前运行日志" : "Export Runtime Logs");
    btnBack->setToolTip(i == 1 ? "返回经典窗口模式" : "Switch to Classic View");
    portEdit->setToolTip(TIP_PORT[i]);
    lblPrt->setToolTip(TIP_PORT[i]);
    threadSpin->setToolTip(TIP_THREAD[i]);
    lblThd->setToolTip(TIP_THREAD[i]);
    tempSpin->setToolTip(TIP_TEMP[i]);
    lblTmp->setToolTip(TIP_TEMP[i]);
    contextSpin->setToolTip(TIP_CTX[i]);
    lblCtx->setToolTip(TIP_CTX[i]);
    chkLockSysPrompt->setToolTip(TIP_LOCK_SYS[i]);
    glossaryCombo->setToolTip(TIP_GLOSSARY[i]);
    chkGlossary->setToolTip(TIP_GLOSSARY[i]);
    btnEditAuto->setToolTip(TIP_BTN_AUTO[i]);
    btnClearCtx->setToolTip(TIP_CLEAR_CTX[i]);
    chkLockGlossary->setToolTip(TIP_LOCK_SYS[i]);
    chkBatch->setText(STR_BATCH_MODE[i]);
    chkBatch->setToolTip(TIP_BATCH_MODE[i]);

    QString baseTip = (i == 1) ? "选择术语表文件 (.txt)" : "Select glossary files (.txt)";
    QString extraTip = (i == 1)
                           ? "\n💡 提示：\n• 左键点击：浏览文件\n• 右键点击：打开清理菜单 (移除路径/清空历史)"
                           : "\n💡 Tip:\n• Left Click: Browse files\n• Right Click: Open cleanup menu";
    btnSelectGlossary->setToolTip(baseTip + extraTip);
    apiAddressCombo->setToolTip(TIP_COMBO_MAIN[i]);

    int presetSize = sizeof(MODERN_PRESETS) / sizeof(MODERN_PRESETS[0]); // Get array length dynamically.
                                                                         // 动态获取数组长度。
    for (int k = 0; k < apiAddressCombo->count(); ++k)
    {
        QString urlStr = apiAddressCombo->itemText(k);
        for (int p = 0; p < presetSize; ++p)
        {
            if (urlStr == MODERN_PRESETS[p].url)
            {
                apiAddressCombo->setItemData(k, MODERN_PRESETS[p].tips[i], Qt::ToolTipRole);
                break;
            }
        }
    }
    apiAddressCombo->setCurrentText(currentApi);
    updatePowerButtonState(m_isServerRunning);

    long long t = lblTokens->property("current_total").toLongLong();
    long long p = lblTokens->property("current_p").toLongLong();
    long long c = lblTokens->property("current_c").toLongLong();
    updateToken(t, p, c);
}

/**
 * Update the token display label with the latest totals.
 * 使用最新总数更新令牌显示标签。
 *
 * @param total Total tokens used.
 * @param p Prompt tokens.
 * @param c Completion tokens.
 */
void ModernWindow::updateToken(long long total, long long p, long long c)
{
    // Store the values in dynamic properties so they survive language switches.
    // 将数值存入动态属性中，以便在语言切换后仍然可用。
    lblTokens->setProperty("current_total", total);
    lblTokens->setProperty("current_p", p);
    lblTokens->setProperty("current_c", c);

    lblTokens->setText(QString("%1 %2").arg(STR_TOKENS[m_lang]).arg(total));

    QString pL = (m_lang == 1) ? "输入总计 (Total Prompt):" : "Total Input:";
    QString cL = (m_lang == 1) ? "输出总计 (Total Completion):" : "Total Output:";

    QString fullTip = QString("<b>%1</b><br><br>%2 %3<br>%4 %5")
                          .arg(TIP_TOKENS[m_lang])
                          .arg(pL)
                          .arg(p)
                          .arg(cL)
                          .arg(c);
    lblTokens->setToolTip(fullTip);
}

/**
 * Fetch the list of models from the API (triggered by the Fetch button).
 * 从API获取模型列表（由获取按钮触发）。
 */
void ModernWindow::onFetchModels()
{
    QString urlBase = apiAddressCombo->currentText();
    if (urlBase.isEmpty())
        return;
    if (urlBase.endsWith("/"))
        urlBase.chop(1);

    m_server->injectLog(m_lang == 1 ? "🔍 正在获取模型列表..." : "🔍 Fetching models...");

    // ==========================================
    // Show a rotating arrow icon on the fetch button during the request.
    // 在请求期间，在获取按钮上显示旋转箭头图标。
    // ==========================================
    btnFetch->setEnabled(false);
    btnFetch->setText(""); // Clear text, only show icon.
                           // 清空文字，只显示图标。

    QTimer *spinTimer = new QTimer(btnFetch);
    spinTimer->setProperty("angle", 0);

    QColor arrowColor = m_isDark ? Qt::white : QColor(50, 50, 50);

    connect(spinTimer, &QTimer::timeout, [this, spinTimer, arrowColor]()
            {
        int angle = spinTimer->property("angle").toInt();
        angle = (angle + 15) % 360; // Rotate 15° each step.
        spinTimer->setProperty("angle", angle);

        QPixmap pix(16, 16);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(8, 8);
        p.rotate(angle);
        p.translate(-8, -8);
        
        QPen pen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        // Draw a 270° arc from 12 o'clock to 9 o'clock (clockwise).
        // 绘制从12点钟到9点钟的270°圆弧（顺时针）。
        p.drawArc(3, 3, 10, 10, 90 * 16, -270 * 16);
        // Draw two small lines for the arrowhead.
        // 绘制箭头的小倒刺。
        p.drawLine(3, 8, 1, 11);
        p.drawLine(3, 8, 5, 11);
        
        btnFetch->setIcon(QIcon(pix)); });
    spinTimer->start(30); // ~33 FPS.
                          // 约33帧/秒。

    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    QNetworkRequest req;
    req.setUrl(QUrl(urlBase + "/models"));
    QString key = apiKeyEdit->text().split(',')[0].trimmed();
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    req.setTransferTimeout(10000);

    QNetworkReply *rep = mgr->get(req);

    connect(rep, &QNetworkReply::finished, [this, rep, mgr, spinTimer]()
            {
        // Stop the spinning animation and restore button text.
        // 停止旋转动画并恢复按钮文字。
        spinTimer->stop();
        spinTimer->deleteLater(); 
        btnFetch->setIcon(QIcon()); // Clear icon.
        btnFetch->setText(STR_FETCH[m_lang]); 
        btnFetch->setEnabled(true);

        if(rep->error() == QNetworkReply::NoError) {
            try {
                auto j = nlohmann::json::parse(rep->readAll().toStdString()); 
                modelCombo->clear();
                int count = 0;
                for(const auto& it : j["data"]) {
                    modelCombo->addItem(QString::fromStdString(it["id"]));
                    count++;
                }
                m_server->injectLog(m_lang == 1 ? QString("✅ 获取成功！共发现 %1 个可用模型。").arg(count) 
                                                 : QString("✅ Success! Found %1 available models.").arg(count));
                modelCombo->setFocus();
            } catch(...) { 
                m_server->injectLog("❌ " + QString(LOG_PARSE_ERR[m_lang])); 
            }
        } else { 
            m_server->injectLog("❌ " + getFriendlyErrorMessage(rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), m_lang)); 
        }
        rep->deleteLater(); 
        mgr->deleteLater(); });
}

/**
 * Test the configuration by sending a simple request to each API key.
 * 通过向每个API密钥发送简单请求来测试配置。
 */
void ModernWindow::onTestConfig()
{
    m_server->injectLog(LOG_TEST_START[m_lang]);
    QStringList keys = apiKeyEdit->text().split(',', Qt::SkipEmptyParts);
    if (keys.isEmpty())
    {
        m_server->injectLog(LOG_NO_KEY[m_lang]);
        return;
    }

    // ==========================================
    // Show a rotating arrow on the test button during the test.
    // 在测试期间，在测试按钮上显示旋转箭头。
    // ==========================================
    btnTest->setEnabled(false);
    btnTest->setText(m_lang == 1 ? " 测试中..." : " Testing...");

    QTimer *spinTimer = new QTimer(btnTest);
    spinTimer->setProperty("angle", 0);

    QColor arrowColor = m_isDark ? Qt::white : QColor(50, 50, 50);

    connect(spinTimer, &QTimer::timeout, [this, spinTimer, arrowColor]()
            {
        int angle = spinTimer->property("angle").toInt();
        angle = (angle + 12) % 360; // Slightly slower rotation.
        spinTimer->setProperty("angle", angle);

        QPixmap pix(16, 16);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(8, 8);
        p.rotate(angle);
        p.translate(-8, -8);
        
        QPen pen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.drawArc(3, 3, 10, 10, 90 * 16, -270 * 16);
        p.drawLine(3, 8, 1, 11);
        p.drawLine(3, 8, 5, 11);
        
        btnTest->setIcon(QIcon(pix)); });
    spinTimer->start(30);

    QString urlBase = apiAddressCombo->currentText();
    if (urlBase.endsWith("/"))
        urlBase.chop(1);
    urlBase += "/chat/completions";

    QString modelStr = modelCombo->currentText();
    auto fnd = std::make_shared<int>(0);
    auto scs = std::make_shared<int>(0);
    int ttl = keys.size();

    for (int i = 0; i < ttl; ++i)
    {
        QString key = keys[i].trimmed();
        QString msk = (key.length() > 8) ? ("..." + key.right(8)) : key;
        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        QNetworkRequest req;
        req.setUrl(QUrl(urlBase));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
        req.setTransferTimeout(10000);

        nlohmann::json j;
        j["model"] = modelStr.toStdString();
        j["messages"] = nlohmann::json::array({{{"role", "user"}, {"content", "hi"}}});
        j["max_tokens"] = 5;

        QNetworkReply *rep = mgr->post(req, QByteArray::fromStdString(j.dump()));

        connect(rep, &QNetworkReply::finished, [this, rep, mgr, msk, i, fnd, scs, ttl, spinTimer]()
                {
            (*fnd)++; if(rep->error() == QNetworkReply::NoError) (*scs)++;
            int cd = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(); 
            if(rep->error() == QNetworkReply::TimeoutError) cd = 999;
            
            QString icn = (rep->error() == QNetworkReply::NoError) ? "✅" : "❌";
            QString st = (rep->error() == QNetworkReply::NoError) ? (m_lang == 1 ? "通过" : "PASS") : getFriendlyErrorMessage(cd, m_lang);
            m_server->injectLog(QString("%1 Key-%2 (%3): %4").arg(icn).arg(i+1).arg(msk).arg(st));
            
            if(*fnd == ttl) {
                // Stop spinning and restore button.
                // 停止旋转并恢复按钮。
                spinTimer->stop();         
                spinTimer->deleteLater();  
                btnTest->setIcon(QIcon()); 
                
                btnTest->setEnabled(true); 
                btnTest->setText(STR_TEST[m_lang]);

                m_server->injectLog("----------------------------------");
                QString summary = QString(m_lang == 1 ? "📊 测试结束！%1: %2, %3: %4" : "📊 Finished! %1: %2, %3: %4")
                    .arg(m_lang == 1 ? "成功" : "Success").arg(*scs)
                    .arg(m_lang == 1 ? "失败" : "Failed").arg(ttl - *scs);

                m_server->injectLog("<b>" + summary + "</b>");
            }
            rep->deleteLater(); 
            mgr->deleteLater(); });
    }
}

/**
 * Load configuration from file and update UI elements.
 * 从文件加载配置并更新UI元素。
 */
void ModernWindow::loadConfigToUi()
{
    if (!m_server)
        return;

    AppConfig cfg = ConfigManager::loadConfig();

    // First, set language and theme based on the loaded config.
    // 首先，根据加载的配置设置语言和主题。
    m_lang = cfg.language;

    if (m_isDark != cfg.is_dark || m_isRounded != cfg.is_rounded)
    {
        m_isDark = cfg.is_dark;
        m_isRounded = cfg.is_rounded;
        m_storedIsRounded = cfg.is_rounded;

        // Reapply global stylesheet.
        // 重新应用全局样式表。
        setStyleSheet(getModernStyle(m_isDark, m_isRounded));
        style()->unpolish(this);
        style()->polish(this);
        // Force repolish of all child widgets.
        // 强制所有子控件重新抛光。
        QList<QWidget *> widgets = this->findChildren<QWidget *>();
        for (QWidget *w : widgets)
        {
            style()->unpolish(w);
            style()->polish(w);
        }
        // Update all glass cards.
        // 更新所有玻璃卡片。
        QList<GlassCard *> cards = this->findChildren<GlassCard *>();
        for (GlassCard *card : cards)
        {
            card->setTheme(m_isDark);
            card->setRounded(m_isRounded);
        }
    }

    // Sync log history from global LogManager.
    // 从全局LogManager同步日志历史。
    logArea->clear();
    QStringList logs = LogManager::instance().getHistory();
    for (const QString &msg : logs)
    {
        updateLog(msg);
    }

    // Restore lock states.
    // 恢复锁定状态。
    chkLockSysPrompt->setChecked(cfg.lock_system_prompt);
    chkLockGlossary->setChecked(cfg.lock_glossary);

    // Fill basic UI fields.
    // 填充基本UI字段。
    apiAddressCombo->setCurrentText(cfg.api_address);
    apiKeyEdit->setText(cfg.api_key);
    modelCombo->setCurrentText(cfg.model_name);
    portEdit->setText(QString::number(cfg.port));
    threadSpin->setValue(cfg.max_threads);
    tempSpin->setValue(cfg.temperature);
    contextSpin->setValue(cfg.context_num);

    prePromptEdit->setText(cfg.pre_prompt);

    // Block signals temporarily to avoid unwanted toggles.
    // 临时阻塞信号以避免不必要的切换。
    chkBatch->blockSignals(true);
    chkBatch->setChecked(cfg.enable_batch);
    chkBatch->blockSignals(false);
    chkGlossary->setChecked(cfg.enable_glossary);

    m_isDebugMode = cfg.enable_debug_mode;
    if (m_isDebugMode)
    {
        btnDebug->setStyleSheet("QPushButton { background: rgba(255, 140, 0, 40); border: 1px solid #FF8C00; border-radius: 6px; }");
    }
    else
    {
        btnDebug->setStyleSheet("QPushButton { background: rgba(128, 128, 128, 20); border: 1px solid rgba(128, 128, 128, 30); border-radius: 6px; } QPushButton:hover { background: rgba(0, 229, 255, 30); border-color: #00e5ff; }");
    }

    // System prompt handling with lock.
    // 带锁的系统提示词处理。
    if (!cfg.lock_system_prompt || systemPromptEdit->toPlainText().isEmpty())
    {
        systemPromptEdit->setText(cfg.system_prompt);
    }

    // Glossary handling with lock.
    // 带锁的术语表处理。
    if (!chkLockGlossary->isChecked())
    {
        glossaryCombo->clear();
        if (!cfg.glossary_history.isEmpty())
            glossaryCombo->addItems(cfg.glossary_history);
        if (!cfg.glossary_path.isEmpty())
            glossaryCombo->setCurrentText(cfg.glossary_path);
    }

    if (glossaryCombo->lineEdit())
        glossaryCombo->lineEdit()->setCursorPosition(0);
    QTimer::singleShot(50, [this]()
                       {
        if(apiAddressCombo->lineEdit()) apiAddressCombo->lineEdit()->setCursorPosition(0);
        if(glossaryCombo->lineEdit()) glossaryCombo->lineEdit()->setCursorPosition(0);
        apiKeyEdit->setCursorPosition(0);
        prePromptEdit->setCursorPosition(0); });

    // Restore opacity and trigger repaint.
    // 恢复透明度并触发重绘。
    m_storedModernOpacity = cfg.modern_opacity;
    m_alpha = cfg.modern_opacity;
    if (m_opacitySlider)
    {
        QSignalBlocker blocker(m_opacitySlider);
        m_opacitySlider->setValue(m_alpha);
    }

    updateUIText();
    updatePowerButtonState(m_server->isRunning());
    updateComboEnv();
    update(); // Final repaint.

    // Sync the updated config to the server.
    // 将更新后的配置同步到服务器。
    if (m_server)
    {
        m_server->updateConfig(cfg);
    }
}

/**
 * Get the current UI state as an AppConfig structure.
 * 将当前UI状态作为AppConfig结构体获取。
 *
 * @return AppConfig containing current settings.
 */
AppConfig ModernWindow::getUiConfig()
{
    AppConfig cfg;
    cfg.api_address = apiAddressCombo->currentText();
    cfg.api_key = apiKeyEdit->text();
    cfg.model_name = modelCombo->currentText();
    cfg.port = portEdit->text().toInt();
    cfg.max_threads = threadSpin->value();
    cfg.temperature = tempSpin->value();
    cfg.context_num = contextSpin->value();
    cfg.system_prompt = systemPromptEdit->toPlainText();
    cfg.pre_prompt = prePromptEdit->text();
    cfg.enable_glossary = chkGlossary->isChecked();
    cfg.glossary_path = glossaryCombo->currentText();
    QStringList h;
    for (int i = 0; i < glossaryCombo->count(); ++i)
        h << glossaryCombo->itemText(i);
    cfg.glossary_history = h;
    cfg.language = m_lang;

    cfg.modern_opacity = m_storedModernOpacity;
    cfg.enable_debug_mode = m_isDebugMode;

    cfg.lock_system_prompt = chkLockSysPrompt->isChecked();
    cfg.lock_glossary = chkLockGlossary->isChecked();

    // Store current UI state.
    // 存储当前UI状态。
    cfg.is_dark = m_isDark;
    cfg.ui_mode = 1; // Modern mode.
    cfg.modern_opacity = m_alpha;
    cfg.is_rounded = m_isRounded;

    cfg.is_rounded = m_storedIsRounded;
    cfg.is_from_modern = true;
    cfg.enable_batch = chkBatch->isChecked();

    return cfg;
}

/**
 * Handle the power button click (start or reload server).
 * 处理电源按钮点击（启动或重载服务器）。
 */
void ModernWindow::onPowerClicked()
{
    if (!m_server)
        return;
    AppConfig nCfg = getUiConfig();
    if (m_isServerRunning)
    {
        AppConfig oCfg = m_server->getConfig();
        if (oCfg.model_name != nCfg.model_name)
            m_server->injectLog(QString(LOG_MODEL_SWITCH[m_lang]).arg(oCfg.model_name).arg(nCfg.model_name));
        else
            m_server->injectLog(QString(LOG_MODEL_SAME[m_lang]).arg(nCfg.model_name));
        m_server->updateConfig(nCfg);
        m_server->injectLog(LOG_RELOADED[m_lang]);
        m_server->injectLog(LOG_AUTO_TESTING[m_lang]);
        onTestConfig();
    }
    else
    {
        m_server->updateConfig(nCfg);
        m_server->startServer();
    }
}

/**
 * Stop the server.
 * 停止服务器。
 */
void ModernWindow::onStopClicked()
{
    if (m_server)
        m_server->stopServer();
}

/**
 * Update the appearance and text of the power button based on server state.
 * 根据服务器状态更新电源按钮的外观和文本。
 *
 * @param running True if server is running, false otherwise.
 */
void ModernWindow::updatePowerButtonState(bool running)
{
    m_isServerRunning = running;

    btnStop->setEnabled(running);
    portEdit->setEnabled(!running);
    threadSpin->setEnabled(!running);

    int r = m_isRounded ? 6 : 0; // Border radius.

    // ============================================================
    // Test button style
    // 测试按钮样式
    // ============================================================
    QString testStyle = m_isDark ? "QPushButton { background: rgba(0, 229, 255, 15); border: 1px solid rgba(0, 229, 255, 45); border-radius: %1px; color: #00E5FF; font-weight: bold; }"
                                   "QPushButton:hover { background: rgba(0, 229, 255, 40); border: 1px solid #00E5FF; color: #FFFFFF; }"
                                   "QPushButton:pressed { background: rgba(0, 229, 255, 60); }"
                                 : "QPushButton { background: rgba(255, 140, 0, 15); border: 1px solid rgba(255, 140, 0, 45); border-radius: %1px; color: #FF8C00; font-weight: bold; }"
                                   "QPushButton:hover { background: rgba(255, 140, 0, 40); border: 1px solid #FF8C00; color: #FFFFFF; }"
                                   "QPushButton:pressed { background: rgba(255, 140, 0, 60); }";
    btnTest->setStyleSheet(testStyle.arg(r));

    // ============================================================
    // Stop button style (neon red)
    // 停止按钮样式（霓虹红）
    // ============================================================
    QString stopStyle = QString(
                            "QPushButton {"
                            "  background: rgba(255, 51, 102, 15);"
                            "  border: 1px solid rgba(255, 51, 102, 50);"
                            "  color: #FF3366;"
                            "  font-weight: bold; border-radius: %1px;"
                            "}"
                            "QPushButton:hover {"
                            "  background: rgba(255, 51, 102, 40);"
                            "  border: 1px solid #FF3366;"
                            "  color: #FFFFFF;"
                            "}"
                            "QPushButton:pressed { background: rgba(255, 51, 102, 60); }"
                            "QPushButton:disabled {"
                            "  background: transparent;"
                            "  border: 1px solid rgba(128, 128, 128, 25);"
                            "  color: rgba(128, 128, 128, 80);"
                            "}")
                            .arg(r);
    btnStop->setStyleSheet(stopStyle);

    // ============================================================
    // Power button style (breathing when running)
    // 电源按钮样式（运行时呼吸）
    // ============================================================
    // Delete any old animation.
    // 删除旧的动画。
    QVariantAnimation *oldAnim = btnPower->findChild<QVariantAnimation *>("powerAnim");
    if (oldAnim)
    {
        oldAnim->stop();
        oldAnim->deleteLater();
    }

    if (running)
    {
        // Running state: reload with breathing effect.
        // 运行状态：重载，带呼吸效果。
        btnPower->setText(STR_RELOAD[m_lang]);

        QVariantAnimation *pulseAnim = new QVariantAnimation(btnPower);
        pulseAnim->setObjectName("powerAnim");
        pulseAnim->setDuration(2500); // 2.5 seconds for deeper breathing.
        pulseAnim->setStartValue(40);
        pulseAnim->setKeyValueAt(0.5, 220); // Peak brightness.
        pulseAnim->setEndValue(40);
        pulseAnim->setLoopCount(-1);
        pulseAnim->setEasingCurve(QEasingCurve::InOutSine);

        connect(pulseAnim, &QVariantAnimation::valueChanged, [this, r](const QVariant &value)
                {
            int alpha = value.toInt(); 
            QString qss;
            
            if (this->m_isDark) {
                qss = QString(
                    "QPushButton { background: rgba(0, 229, 255, 20); border: 1px solid rgba(0, 229, 255, %1); border-radius: %2px; color: #00E5FF; font-weight: bold; }"
                    "QPushButton:hover { background: rgba(0, 229, 255, 60); color: #FFF; border: 1px solid #00E5FF; }"
                ).arg(alpha).arg(r);
            } else {
                qss = QString(
                    "QPushButton { background: rgba(255, 140, 0, 20); border: 1px solid rgba(255, 140, 0, %1); border-radius: %2px; color: #FF8C00; font-weight: bold; }"
                    "QPushButton:hover { background: rgba(255, 140, 0, 60); color: #FFF; border: 1px solid #FF8C00; }"
                ).arg(alpha).arg(r);
            }
            btnPower->setStyleSheet(qss); });
        pulseAnim->start();
    }
    else
    {
        // Stopped state: start.
        // 停止状态：启动。
        btnPower->setText(STR_START[m_lang]);

        QString startStyle = m_isDark ? QString("QPushButton { background: rgba(255, 140, 0, 15); border: 1px solid rgba(255, 140, 0, 50); border-radius: %1px; color: #FF8C00; font-weight: bold; }"
                                                "QPushButton:hover { background: rgba(255, 140, 0, 40); border: 1px solid #FF8C00; color: #FFFFFF; }"
                                                "QPushButton:pressed { background: rgba(255, 140, 0, 60); }")
                                            .arg(r)
                                      : QString("QPushButton { background: rgba(148, 0, 211, 15); border: 1px solid rgba(148, 0, 211, 50); border-radius: %1px; color: #9400D3; font-weight: bold; }"
                                                "QPushButton:hover { background: rgba(148, 0, 211, 40); border: 1px solid #9400D3; color: #FFFFFF; }"
                                                "QPushButton:pressed { background: rgba(148, 0, 211, 60); }")
                                            .arg(r);

        btnPower->setStyleSheet(startStyle);
    }
}

/**
 * Append a log message to the log area, with language adaptation.
 * 将日志消息追加到日志区域，并进行语言适配。
 *
 * @param msg The log message.
 */
void ModernWindow::updateLog(QString msg)
{
    if (!logArea)
        return;

    // Adapt messages for the current language.
    // 根据当前语言适配消息。
    if (m_lang == 0) // English
    {
        if (msg.contains("测速模式"))
        {
            msg.replace("已开启", "ON");
            msg.replace("已关闭", "OFF");
            msg.replace("测速模式: ", "Speed Test Mode: ");
            msg.replace("测速模式：", "Speed Test Mode: ");
            msg.replace("测速模式", "Speed Test Mode");
        }
        if (msg.contains("多行模式"))
        {
            msg.replace("已开启", "ON");
            msg.replace("已关闭", "OFF");
            msg.replace("多行模式: ", "Batch Mode: ");
            msg.replace("多行模式：", "Batch Mode: ");
            msg.replace("多行模式", "Batch Mode");
        }
    }
    else if (m_lang == 1) // Chinese
    {
        if (msg.contains("Speed Test Mode"))
        {
            msg.replace("ON", "已开启");
            msg.replace("OFF", "已关闭");
            msg.replace("Speed Test Mode: ", "测速模式: ");
            msg.replace("Speed Test Mode", "测速模式");
        }
        if (msg.contains("Batch Mode"))
        {
            msg.replace("ON", "已开启");
            msg.replace("OFF", "已关闭");
            msg.replace("Batch Mode: ", "多行模式: ");
            msg.replace("Batch Mode", "多行模式");
        }
    }

    logArea->append(msg);

    // Limit log size to prevent memory issues.
    // 限制日志大小以防内存问题。
    if (logArea->document()->blockCount() > 2000)
    {
        QTextCursor c(logArea->document());
        c.movePosition(QTextCursor::Start);
        for (int i = 0; i < 100; ++i)
            c.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        c.removeSelectedText();
    }

    // Auto‑scroll to bottom.
    // 自动滚动到底部。
    logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
}

/**
 * Open a file dialog to select a glossary file.
 * 打开文件对话框选择术语表文件。
 */
void ModernWindow::onSelectGlossary()
{
    QString fn = QFileDialog::getOpenFileName(this, "Select", "", "_Substitutions (*.txt);;All (*)");
    if (!fn.isEmpty())
        addToGlossaryHistory(fn);

    // If server is running, apply immediately.
    // 如果服务器正在运行，立即生效。
    if (m_isServerRunning && m_server)
    {
        AppConfig cfg = getUiConfig();
        m_server->updateConfig(cfg);
        m_server->injectLog(m_lang == 0 ? "✅ New glossary applied." : "✅ 新术语表已应用。");
    }
}

/**
 * Handle glossary selection change.
 * 处理术语表选择变化。
 */
void ModernWindow::onGlossaryChanged()
{
    AppConfig currentCfg = getUiConfig();
    if (m_isServerRunning && m_server)
    {
        m_server->updateConfig(currentCfg);
        QString path = currentCfg.glossary_path;
        if (path.isEmpty())
            path = (m_lang == 1 ? "未选择" : "None");
        QString msg = (m_lang == 1)
                          ? QString("🔄 术语表已切换至：%1").arg(path)
                          : QString("🔄 Glossary switched to: %1").arg(path);
        m_server->injectLog(msg);
    }
}

/**
 * Load configuration from a selected file.
 * 从选定的文件加载配置。
 */
void ModernWindow::onLoadConfig()
{
    QString fn = QFileDialog::getOpenFileName(this, "Load", "", "*.ini");
    if (!fn.isEmpty())
    {
        AppConfig c = ConfigManager::loadConfig(fn);
        apiAddressCombo->setCurrentText(c.api_address);
        apiKeyEdit->setText(c.api_key);
        modelCombo->setCurrentText(c.model_name);
        portEdit->setText(QString::number(c.port));
        threadSpin->setValue(c.max_threads);
        tempSpin->setValue(c.temperature);
        contextSpin->setValue(c.context_num);
        prePromptEdit->setText(c.pre_prompt);
        chkGlossary->setChecked(c.enable_glossary);
        if (!chkLockSysPrompt->isChecked())
            systemPromptEdit->setText(c.system_prompt);
        if (!chkLockGlossary->isChecked())
        {
            glossaryCombo->clear();
            glossaryCombo->addItems(c.glossary_history);
            glossaryCombo->setCurrentText(c.glossary_path);
        }
        m_server->injectLog(QString(LOG_CFG_LOADED[m_lang]) + fn);
    }
}

/**
 * Save current configuration to a selected file.
 * 将当前配置保存到选定的文件。
 */
void ModernWindow::onSaveConfig()
{
    QString fn = QFileDialog::getSaveFileName(this, "Save", "config.ini", "*.ini");
    if (!fn.isEmpty())
    {
        ConfigManager::saveConfig(getUiConfig(), fn);
        m_server->injectLog(QString(LOG_CFG_SAVED[m_lang]) + fn);
    }
}

/**
 * Export the log area content to "run_log.txt".
 * 将日志区域内容导出到"run_log.txt"。
 */
void ModernWindow::onExportLog()
{
    QFile f("run_log.txt");
    if (f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream(&f) << logArea->toPlainText();
        m_server->injectLog(LOG_EXPORTED[m_lang]);
    }
}

/**
 * Open the automatically generated translations file (_AutoGeneratedTranslations.txt)
 * located in the same directory as the current glossary.
 * 打开与当前术语表同一目录下的自动翻译文件 (_AutoGeneratedTranslations.txt)。
 */
void ModernWindow::onOpenAutoTranslations()
{
    QString p = glossaryCombo->currentText();

    if (p.isEmpty() || p == "未选择" || p == "None")
    {
        GlassMessageBox::warning(this,
                                 (m_lang == 1 ? "⚠️ 提示" : "⚠️ Notice"),
                                 (m_lang == 1 ? "请先选择一个有效的术语表路径！\n程序需要基于该路径来定位自动翻译记录文件。"
                                              : "Please select a valid glossary path first!\nThe program needs it to locate the auto-translation file."),
                                 m_isDark, m_alpha, m_isRounded);
        return;
    }

    QString t = QFileInfo(p).absolutePath() + "/_AutoGeneratedTranslations.txt";
    if (!QFileInfo::exists(t))
    {
        GlassMessageBox::warning(this,
                                 (m_lang == 1 ? "⚠️ 文件未找到" : "⚠️ File Not Found"),
                                 (m_lang == 1 ? "未找到 _AutoGeneratedTranslations.txt。\n请确认游戏是否已经运行并生成了翻译。"
                                              : "Could not find _AutoGeneratedTranslations.txt."),
                                 m_isDark, m_alpha, m_isRounded);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(t));
}

/**
 * Show a context menu for glossary management (remove path, clear history).
 * 显示术语表管理的上下文菜单（移除路径、清空历史）。
 *
 * @param pos Position where the menu was requested.
 */
void ModernWindow::onGlossaryContextMenu(const QPoint &pos)
{
    QMenu m(this);
    m.setStyleSheet("QMenu { background-color: #333; color: white; border: 1px solid #555; } QMenu::item:selected { background-color: #00e5ff; color: black; }");
    QAction *rm = m.addAction(STR_REMOVE_PATH[m_lang]);
    QAction *cl = m.addAction(STR_CLEAR_HISTORY[m_lang]);

    if (glossaryCombo->currentText().isEmpty())
    {
        rm->setEnabled(false);
    }

    QWidget *senderWidget = qobject_cast<QWidget *>(sender());
    QPoint globalPos;
    if (senderWidget)
    {
        globalPos = senderWidget->mapToGlobal(pos);
        globalPos.ry() += 5;
    }
    else
    {
        globalPos = QCursor::pos();
    }

    QAction *s = m.exec(globalPos);
    if (s == rm)
    {
        int index = glossaryCombo->currentIndex();
        if (index != -1)
        {
            glossaryCombo->removeItem(index);
            if (m_server)
            {
                m_server->injectLog(m_lang == 1 ? "🧽 已从历史记录中移除该路径。" : "🧽 Removed current path from history.");
            }
        }

        if (glossaryCombo->count() == 0 || index == -1)
        {
            glossaryCombo->setEditText("");
        }
    }
    else if (s == cl)
    {
        glossaryCombo->clear();
        glossaryCombo->setEditText("");

        if (m_server)
        {
            m_server->injectLog(m_lang == 1 ? "🗑️ 术语表历史记录已清空。" : "🗑️ Glossary history cleared.");
        }
    }
}

/**
 * Show a context menu for the log area (clear log).
 * 显示日志区域的上下文菜单（清空日志）。
 *
 * @param pos Position where the menu was requested.
 */
void ModernWindow::onLogContextMenu(const QPoint &pos)
{
    QMenu *m = logArea->createStandardContextMenu();
    m->setStyleSheet("QMenu { background-color: #333; color: white; border: 1px solid #555; } QMenu::item:selected { background-color: #00e5ff; color: black; }");

    m->addSeparator();

    QAction *cl = m->addAction(STR_CLEAR_LOG[m_lang]);
    connect(cl, &QAction::triggered, []()
            {
                LogManager::instance().clear(); // Call global clear.
                                                // 调用全局清空。
            });

    m->exec(logArea->mapToGlobal(pos));
    delete m;
}

/**
 * Clear all context histories (calls the server).
 * 清除所有上下文历史（调用服务器）。
 */
void ModernWindow::onClearContext()
{
    if (m_server)
        m_server->clearAllContexts();
}

/**
 * Add a path to the glossary history combo box.
 * 将路径添加到术语表历史组合框。
 *
 * @param p The file path to add.
 */
void ModernWindow::addToGlossaryHistory(const QString &p)
{
    if (p.isEmpty())
        return;
    QStringList l;
    for (int i = 0; i < glossaryCombo->count(); ++i)
        l << glossaryCombo->itemText(i);
    l.removeAll(p);
    l.insert(0, p);
    while (l.size() > 10)
        l.removeLast();
    glossaryCombo->clear();
    glossaryCombo->addItems(l);
    glossaryCombo->setCurrentIndex(0);
    if (glossaryCombo->lineEdit())
        glossaryCombo->lineEdit()->setCursorPosition(0);
}

/**
 * Custom paint event: draws the glass background with gradient and glow.
 * 自定义绘制事件：绘制带有渐变和发光的玻璃背景。
 *
 * @param event Unused.
 */
void ModernWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRect rect = this->rect();
    int r = m_isRounded ? 20 : 0;

    // Background gradient.
    // 背景渐变。
    QLinearGradient bgGrad(rect.topLeft(), rect.bottomLeft());
    if (m_isDark)
    {
        bgGrad.setColorAt(0.0, QColor(35, 30, 45, qMin(255, m_alpha)));
        bgGrad.setColorAt(1.0, QColor(10, 10, 15, qMin(255, m_alpha + 10)));
    }
    else
    {
        bgGrad.setColorAt(0.0, QColor(245, 250, 255, qMin(255, m_alpha)));
        bgGrad.setColorAt(1.0, QColor(220, 230, 240, qMin(255, m_alpha + 15)));
    }
    p.setBrush(bgGrad);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect, r, r);

    // Top highlight.
    // 顶部高光。
    QRect shineRect = rect;
    shineRect.setHeight(rect.height() * 0.35);
    QLinearGradient shineGrad(shineRect.topLeft(), shineRect.bottomLeft());
    shineGrad.setColorAt(0.0, QColor(255, 255, 255, m_isDark ? 15 : 120));
    shineGrad.setColorAt(1.0, QColor(255, 255, 255, 0));
    p.setBrush(shineGrad);
    p.drawRoundedRect(shineRect, r, r);

    // Inner glow layers.
    // 内部发光层。
    int glowLayers = 5;
    QColor glowColor = m_isDark ? QColor(255, 140, 0) : QColor(100, 40, 20);
    int startAlpha = m_isDark ? 130 : 120;

    p.setBrush(Qt::NoBrush);
    for (int i = 0; i < glowLayers; ++i)
    {
        QColor col = glowColor;
        col.setAlpha(startAlpha * (glowLayers - i) / glowLayers);
        p.setPen(QPen(col, 1));
        p.drawRoundedRect(rect.adjusted(i, i, -i, -i), r - i, r - i);
    }

    // Outer border.
    // 外边框。
    QPen borderPen(m_isDark ? QColor(255, 180, 100, 80) : QColor(148, 0, 211, 100), 1);
    p.setPen(borderPen);
    p.drawRoundedRect(rect, r, r);
}

/**
 * Handle mouse press events to initiate window dragging (only from top area).
 * 处理鼠标按下事件以开始窗口拖拽（仅限顶部区域）。
 *
 * @param e Mouse event.
 */
void ModernWindow::mousePressEvent(QMouseEvent *e)
{
    // Allow dragging only when clicking in the top 60 pixels.
    // 仅当点击顶部60像素区域内时允许拖动。
    if (e->button() == Qt::LeftButton && e->pos().y() <= 60)
    {
        m_isDragging = true;
        m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    else
    {
        m_isDragging = false;
    }
}

/**
 * Handle mouse move events to perform window dragging.
 * 处理鼠标移动事件以执行窗口拖拽。
 *
 * @param e Mouse event.
 */
void ModernWindow::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) && m_isDragging)
    {
        move(e->globalPosition().toPoint() - m_dragPos);
    }
}

/**
 * Handle mouse release events to reset dragging state.
 * 处理鼠标释放事件以重置拖拽状态。
 *
 * @param e Mouse event.
 */
void ModernWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        m_isDragging = false;
    }
    QMainWindow::mouseReleaseEvent(e);
}

/**
 * Overridden show event: performs a fade‑in animation unless bypassed.
 * 重写的显示事件：执行淡入动画（除非被绕过）。
 *
 * @param event Show event.
 */
void ModernWindow::showEvent(QShowEvent *event)
{
    // Do nothing for spontaneous show events (e.g., from OS).
    // 对于自发显示事件（如操作系统触发）不做处理。
    if (event->spontaneous())
    {
        QMainWindow::showEvent(event);
        return;
    }

    // Load configuration and sync with server.
    // 加载配置并与服务器同步。
    loadConfigToUi();

    if (m_server)
    {
        AppConfig liveCfg = m_server->getConfig();
        if (!liveCfg.glossary_path.isEmpty())
        {
            if (glossaryCombo->findText(liveCfg.glossary_path) == -1)
                glossaryCombo->addItem(liveCfg.glossary_path);
            glossaryCombo->setCurrentText(liveCfg.glossary_path);
        }
        chkGlossary->setChecked(liveCfg.enable_glossary);
        chkLockGlossary->setChecked(liveCfg.lock_glossary);
        chkLockSysPrompt->setChecked(liveCfg.lock_system_prompt);
        if (m_lang != liveCfg.language)
        {
            m_lang = liveCfg.language;
            updateUIText();
        }
        if (glossaryCombo->lineEdit())
            glossaryCombo->lineEdit()->setCursorPosition(0);
    }

    // If the bypass property is set (e.g., from main.cpp during transition), just show with full opacity.
    // 如果设置了绕过属性（例如在过渡期间由main.cpp设置），则直接以完全不透明度显示。
    if (this->property("bypass_fade").toBool())
    {
        this->setWindowOpacity(1.0);
        this->setProperty("bypass_fade", false); // Clear the property.
    }
    else
    {
        this->setWindowOpacity(0.0); // Start transparent.

        // Fade‑in animation (400ms).
        // 淡入动画（400毫秒）。
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
        fadeAnim->setDuration(400);
        fadeAnim->setStartValue(0.0);
        fadeAnim->setEndValue(1.0);
        fadeAnim->setEasingCurve(QEasingCurve::OutQuad);
        fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QMainWindow::showEvent(event);
}

/**
 * Handle changes in the opacity slider.
 * 处理透明度滑块的变动。
 *
 * @param v New opacity value (50–255).
 */
void ModernWindow::onOpacityChange(int v)
{
    m_alpha = v;
    m_storedModernOpacity = v;

    updateComboEnv();

    // Update the glossary drawer if open.
    // 如果术语表抽屉已打开，则更新其透明度。
    if (m_glossaryDrawer)
    {
        static_cast<GlossaryDrawer *>(m_glossaryDrawer.data())->setAlpha(m_alpha);
    }

    update(); // Repaint main window.
}

/**
 * Toggle the interface language.
 * 切换界面语言。
 */
void ModernWindow::toggleLanguage()
{
    smoothSwitch([this]()
                 {
                     m_lang = (m_lang == 0) ? 1 : 0;
                     updateUIText();
                     if (m_server)
                     {
                         AppConfig c = m_server->getConfig();
                         c.language = m_lang;
                         m_server->updateConfig(c);
                     }

                     if (m_glossaryDrawer)
                     {
                         static_cast<GlossaryDrawer *>(m_glossaryDrawer.data())->updateLanguage(m_lang);
                     } });
}

/**
 * Switch back to classic mode (emits signal for main.cpp to handle).
 * 切换回经典模式（发射信号让main.cpp处理）。
 */
void ModernWindow::onSwitchClicked()
{
    // Close any open glossary drawer immediately.
    // 立即关闭任何打开的术语表抽屉。
    if (m_glossaryDrawer)
    {
        m_glossaryDrawer->close();
    }

    if (m_server)
        m_server->updateConfig(getUiConfig());

    emit requestClassicMode();
}

// ==========================================
// Local class for the holographic ripple overlay used during smooth transitions.
// 用于平滑过渡期间的全息波纹覆盖层的局部类。
// ==========================================
class RippleOverlay : public QWidget
{
public:
    QPixmap m_pixmap;
    QPoint m_center;
    float m_radius = 0;
    bool m_isDarkTarget = true; // Determines the color of the wave.
                                // 决定光波的色调。

    RippleOverlay(const QPixmap &pix, const QPoint &center, QWidget *parent = nullptr)
        : QWidget(parent), m_pixmap(pix), m_center(center)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        // 1. Clip the old screenshot with a circular hole.
        // 1. 用圆形孔洞剪裁旧截图。
        QPainterPath path;
        path.addRect(rect());
        path.addEllipse(QPointF(m_center), m_radius, m_radius);
        p.setClipPath(path);

        // Use Source composition mode to avoid blending with underlying new UI.
        // 使用Source合成模式避免与底层新UI混合。
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawPixmap(0, 0, m_pixmap);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        p.setClipping(false);

        // ==========================================
        // Draw the holographic energy wave.
        // 绘制全息能量波。
        // ==========================================
        if (m_radius > 0)
        {
            float waveFront = m_radius + 5.0f;
            float waveTail = std::max(0.0f, m_radius - 70.0f);
            if (waveFront <= 0)
                return;

            QRadialGradient grad(m_center, waveFront);
            float cutStop = m_radius / waveFront;
            float tailStop = waveTail / waveFront;

            QColor coreColor = m_isDarkTarget ? QColor(255, 120, 0, 255) : QColor(148, 0, 211, 220);
            QColor midColor = m_isDarkTarget ? QColor(255, 120, 0, 90) : QColor(148, 0, 211, 90);

            grad.setColorAt(1.0, Qt::transparent);
            grad.setColorAt(qMin(1.0f, cutStop + 0.015f), coreColor);
            grad.setColorAt(cutStop, coreColor);

            if (tailStop > 0 && tailStop < cutStop)
            {
                grad.setColorAt(std::max(tailStop, cutStop - 0.15f), midColor);
                grad.setColorAt(tailStop, Qt::transparent);
            }
            grad.setColorAt(0.0, Qt::transparent);

            p.setBrush(grad);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(m_center), waveFront, waveFront);
        }
    }
};

/**
 * Smooth transition effect for UI changes (theme, language, shape).
 * UI变化时的平滑过渡效果（主题、语言、形状）。
 *
 * @param changeLogic Function that performs the actual UI change.
 */
void ModernWindow::smoothSwitch(std::function<void()> changeLogic)
{
    // Prevent multiple concurrent switches.
    // 防止多个并发切换。
    if (this->property("is_switching").toBool())
        return;
    this->setProperty("is_switching", true);

    // 1. Get cursor position as epicenter.
    // 1. 获取光标位置作为震中。
    QPoint globalPos = QCursor::pos();
    QPoint epicenter = this->mapFromGlobal(globalPos);

    // 2. Capture current window and create overlay.
    // 2. 捕获当前窗口并创建覆盖层。
    RippleOverlay *overlay = new RippleOverlay(this->grab(), epicenter, this);
    overlay->setGeometry(this->rect());
    overlay->setStyleSheet("background: transparent; border: none;");
    overlay->show();
    overlay->raise();

    RippleOverlay *drawerOverlay = nullptr;
    if (m_glossaryDrawer)
    {
        QPoint drawerCenter = m_glossaryDrawer->mapFromGlobal(globalPos);
        drawerOverlay = new RippleOverlay(m_glossaryDrawer->grab(), drawerCenter, m_glossaryDrawer.data());
        drawerOverlay->setGeometry(m_glossaryDrawer->rect());
        drawerOverlay->setStyleSheet("background: transparent; border: none;");
        drawerOverlay->show();
        drawerOverlay->raise();
    }

    // Force immediate repaint of overlays to ensure they appear before any change.
    // 强制立即重绘覆盖层，确保它们在任何变化前出现。
    overlay->repaint();
    if (drawerOverlay)
        drawerOverlay->repaint();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // Schedule the actual change after a tiny delay.
    // 在短暂延迟后安排实际变化。
    QTimer::singleShot(15, this, [this, overlay, drawerOverlay, epicenter, changeLogic]()
                       {
        
        // 3. Execute the time‑consuming UI change logic (now hidden behind the overlay).
        // 3. 执行耗时的UI变化逻辑（现在被覆盖层隐藏）。
        changeLogic();

        // 4. Update overlay theme to match the new target.
        // 4. 更新覆盖层的主题以匹配新目标。
        overlay->m_isDarkTarget = this->m_isDark;
        if (drawerOverlay) drawerOverlay->m_isDarkTarget = this->m_isDark;

        // Force the new UI to be fully rendered.
        // 强制新UI完全渲染。
        this->repaint();

        // Calculate maximum radius needed to fully reveal the window.
        // 计算完全显示窗口所需的最大半径。
        int w = this->width();
        int h = this->height();
        double maxRadius = std::sqrt(std::pow(std::max(epicenter.x(), w - epicenter.x()), 2) +
                                     std::pow(std::max(epicenter.y(), h - epicenter.y()), 2));
        maxRadius += 30;

        // 5. Animate the expanding ripple.
        // 5. 动画化扩展的波纹。
        QVariantAnimation *anim = new QVariantAnimation(this);
        anim->setDuration(550); 
        anim->setStartValue(0.0f);
        anim->setEndValue((float)maxRadius);
        anim->setEasingCurve(QEasingCurve::InOutCubic);

        connect(anim, &QVariantAnimation::valueChanged, [overlay, drawerOverlay](const QVariant& val){
            float r = val.toFloat();
            overlay->m_radius = r;
            overlay->update();
            if(drawerOverlay) {
                drawerOverlay->m_radius = r;
                drawerOverlay->update();
            }
        });

        connect(anim, &QVariantAnimation::finished, [this, overlay, drawerOverlay, anim](){
            overlay->deleteLater();
            if(drawerOverlay) drawerOverlay->deleteLater();
            anim->deleteLater();
            this->setProperty("is_switching", false); 
        });

        anim->start(); });
}

/**
 * Open the glossary drawer (sliding panel) to edit the currently selected glossary.
 * 打开术语表抽屉（滑动面板）以编辑当前选择的术语表。
 */
void ModernWindow::onEditGlossaryClicked()
{
    // If drawer already open, close it.
    // 如果抽屉已打开，则关闭它。
    if (m_glossaryDrawer)
    {
        static_cast<GlossaryDrawer *>(m_glossaryDrawer.data())->animateClose();
        return;
    }

    // Get current glossary path.
    // 获取当前术语表路径。
    QString path = glossaryCombo->currentText();
    if (path.isEmpty() || path == "未选择" || path == "None")
    {
        GlassMessageBox::warning(this,
                                 m_lang == 1 ? "⚠️ 提示" : "⚠️ Notice",
                                 m_lang == 1 ? "请先选择一个术语表！" : "Please select a glossary first!",
                                 m_isDark, m_alpha, m_isRounded);
        return;
    }

    if (!QFileInfo::exists(path))
    {
        GlassMessageBox::warning(this,
                                 m_lang == 1 ? "❌ 错误" : "❌ Error",
                                 m_lang == 1 ? "该术语表文件不存在！" : "Glossary file does not exist!",
                                 m_isDark, m_alpha, m_isRounded);
        return;
    }

    // Create the drawer.
    // 创建抽屉。
    m_glossaryDrawer = new GlossaryDrawer(this, path, m_isDark, m_alpha, m_isRounded, m_lang, m_server);
    m_glossaryDrawer->show();
}

/**
 * Toggle the theme (dark/light) with smooth transition.
 * 切换主题（暗色/亮色）并带有平滑过渡。
 */
void ModernWindow::toggleTheme()
{
    smoothSwitch([this]()
                 {
        m_isDark = !m_isDark;
        setStyleSheet(getModernStyle(m_isDark, m_isRounded));
        
        style()->unpolish(this);
        style()->polish(this);
        
        QList<QWidget*> widgets = this->findChildren<QWidget*>();
        for (QWidget* w : widgets) {
            style()->unpolish(w);
            style()->polish(w);
        }
        
        QList<GlassCard*> cards = this->findChildren<GlassCard*>();
        for (GlassCard* card : cards) {
            card->setTheme(m_isDark);
        }
        
        updateComboEnv();
        
        if (m_glossaryDrawer) {
            static_cast<GlossaryDrawer*>(m_glossaryDrawer.data())->updateEnv(m_isDark, m_alpha, m_isRounded);
        }
        
        update(); 
        
        // Force cursor reset to avoid Qt viewport offset bug.
        // 强制重置光标以避免Qt视口偏移错误。
        resetAllCursors(); 
        updatePowerButtonState(m_isServerRunning); });
}

/**
 * Toggle the window shape (rounded / sharp corners) with smooth transition.
 * 切换窗口形状（圆角/直角）并带有平滑过渡。
 */
void ModernWindow::toggleShape()
{
    smoothSwitch([this]()
                 {
        m_isRounded = !m_isRounded;
        m_storedIsRounded = m_isRounded; 
        
        setStyleSheet(getModernStyle(m_isDark, m_isRounded));
        style()->unpolish(this); style()->polish(this);
        
        QList<QWidget*> widgets = this->findChildren<QWidget*>();
        for (QWidget* w : widgets) { style()->unpolish(w); style()->polish(w); }
        
        QList<GlassCard*> cards = this->findChildren<GlassCard*>();
        for (GlassCard* card : cards) card->setRounded(m_isRounded);
        
        updateComboEnv();
        
        if (m_glossaryDrawer) {
            static_cast<GlossaryDrawer*>(m_glossaryDrawer.data())->updateEnv(m_isDark, m_alpha, m_isRounded);
        }
        
        update();
        
        resetAllCursors(); 
        updatePowerButtonState(m_isServerRunning); });
}

/**
 * Toggle the debug mode (speed test) and update UI.
 * 切换测速模式并更新UI。
 */
void ModernWindow::toggleDebugMode()
{
    m_isDebugMode = !m_isDebugMode;

    if (m_isDebugMode)
    {
        btnDebug->setStyleSheet("QPushButton { background: rgba(255, 140, 0, 40); border: 1px solid #FF8C00; border-radius: 6px; }");
    }
    else
    {
        btnDebug->setStyleSheet("QPushButton { background: rgba(128, 128, 128, 20); border: 1px solid rgba(128, 128, 128, 30); border-radius: 6px; } QPushButton:hover { background: rgba(0, 229, 255, 30); border-color: #00e5ff; }");
    }

    updateUIText();

    QString msg = (m_lang == 1) ? QString("🛠️ 测速模式: %1").arg(m_isDebugMode ? "已开启" : "已关闭") : QString("🛠️ Speed Test Mode: %1").arg(m_isDebugMode ? "ON" : "OFF");
    LogManager::instance().addLog(msg);

    if (m_server && m_isServerRunning)
    {
        m_server->updateConfig(getUiConfig());
    }
}