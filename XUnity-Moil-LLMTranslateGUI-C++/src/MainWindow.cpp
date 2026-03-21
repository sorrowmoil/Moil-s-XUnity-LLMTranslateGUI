/**
 * MainWindow.cpp - Implementation of the main window for Moil's XUnity LLM Translator.
 * MainWindow.cpp - Moil的XUnity大模型翻译器主窗口实现。
 *
 * Based on preferred UI/UX and robust hot reload logic.
 * 基于优选UI/UX和鲁棒的热重载逻辑。
 */

#include "MainWindow.h"
#include "json.hpp"
#include "LogManager.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QCloseEvent>
#include <QStyleFactory>
#include <QPixmap>
#include <QMenu>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QListView>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QParallelAnimationGroup>
#include <QEasingCurve>

#include <QSyntaxHighlighter>
#include <QRegularExpression>

// ==========================================
// Glossary syntax highlighter (real‑time rendering of "original=translation")
// 术语表语法高亮器（实时渲染“原文=译文”）
// ==========================================
class GlossaryHighlighter : public QSyntaxHighlighter
{
public:
    explicit GlossaryHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {}

    /**
     * Set the color scheme according to the current theme.
     * 根据当前主题设置配色方案。
     * @param isDark True for dark theme, false for light theme.
     */
    void setTheme(bool isDark)
    {
        if (isDark)
        {
            // Dark theme colors / 暗色主题
            fmtOriginal.setForeground(QColor("#CD7F32"));    // Bronze for original term ; 原文用古铜色
            fmtOriginal.setFontWeight(QFont::Bold);          // Bold for better visibility ; 加粗以增强辨识度
            fmtEqual.setForeground(QColor("#3bfd00"));       // Grayish for the equals sign ; 等号用灰色
            fmtTranslation.setForeground(QColor("#fd0000")); // Golden for translation ; 译文用金色
            fmtComment.setForeground(QColor("#6A9955"));     // Green for comments ; 注释用护眼绿
        }
        else
        {
            // Light theme colors / 亮色主题
            fmtOriginal.setForeground(QColor("#fc0000")); // Darker bronze ; 深马鞍棕
            fmtOriginal.setFontWeight(QFont::Bold);
            fmtEqual.setForeground(QColor("#ff0000fd"));
            fmtTranslation.setForeground(QColor("#ff00d4")); // Microsoft blue ; 微软科技蓝
            fmtComment.setForeground(QColor("#008000"));
        }
        rehighlight(); // Refresh the whole document ; 立即刷新整个文档的高亮
    }

protected:
    void highlightBlock(const QString &text) override
    {
        // 1. If the line starts with "//", treat it as a comment (whole line green)
        // 1. 如果行以“//”开头，整行视为注释（绿色）
        if (text.trimmed().startsWith("//"))
        {
            setFormat(0, text.length(), fmtComment);
            return;
        }

        // 2. Find the first equals sign (XUnity format uses the first '=' as separator)
        // 2. 查找第一个等号的位置（XUnity格式以首个等号分割）
        int equalPos = text.indexOf('=');
        if (equalPos != -1)
        {
            // Original text (from start to equalPos)
            setFormat(0, equalPos, fmtOriginal);

            // The equals sign itself
            setFormat(equalPos, 1, fmtEqual);

            // Translation (from after equals to end of line)
            setFormat(equalPos + 1, text.length() - equalPos - 1, fmtTranslation);
        }
        // If no equals sign, no formatting is applied.
        // 如果没有等号，则不应用任何格式。
    }

private:
    QTextCharFormat fmtOriginal;
    QTextCharFormat fmtEqual;
    QTextCharFormat fmtTranslation;
    QTextCharFormat fmtComment;
};

// ==========================================
// Multilingual dictionary definitions (UI text)
// 多语言字典定义（UI文本）
// ==========================================

const char *STR_BATCH_MODE[] = {"Batching", "多行模式"};
const char *TIP_BATCH_MODE[] = {
    "[Advanced] If checked, Config.ini will be hijacked to support batch translation (auto-backup).\n[Rec]: OFF for Visual Novels, ON for RPG/UI-heavy games.\n[Disclaimer] Enabling may increase translation latency. \n[Tip] Startup flow: Start service first, then launch game (ensure ini is modified).",
    "【慎用】开启后，启动服务时将自动修改游戏 Config.ini 以支持多行并发（会自动备份）。\n【建议】纯视觉小说游戏请关闭此项；UI 密集型游戏请开启。\n【声明】开启多行翻译后翻译延时可能会增加。\n【提示】开启后，启动流程为：先启动服务，再运行游戏（确保 ini 已被修改）。"};

const char *STR_DEBUG_MODE[] = {"Speed Test", "测速模式"};
const char *TIP_DEBUG_MODE[] = {"Enable to show endpoint types and ms latency in logs", "开启后，日志将显示底层端口类型和毫秒级耗时"};

// Window titles / 窗口标题
const char *STR_TITLE[] = {"Moil's XUnity LLM Translator", "Moil的XUnity大模型翻译GUI"};

// Configuration section titles / 配置部分标题
const char *STR_API_CFG[] = {"API Configuration", "API 配置"};
const char *STR_LOG_AREA[] = {"Runtime Logs", "运行日志"};

// API configuration related text / API配置相关文本
const char *STR_API_ADDR[] = {"API Address:", "API 地址:"};
const char *STR_API_KEY[] = {"API Key:", "API 密钥:"};
const char *STR_MODEL[] = {"Model Name:", "模型名称:"};
const char *STR_FETCH[] = {"Fetch", "获取"};

// Server parameter text / 服务器参数文本
const char *STR_PORT[] = {"Port:", "端口:"};
const char *STR_THREAD[] = {"Thr:", "线程:"};
const char *STR_TEMP[] = {"Tmp:", "温度:"};
const char *STR_CTX[] = {"Context:", "上下文:"};

// Prompt related text / 提示词相关文本
const char *STR_SYS_PROMPT[] = {"System Prompt:", "系统提示:"};
const char *STR_PRE_PROMPT[] = {"Pre-Prompt:", "前置文本:"};

// Glossary editor and modern mode related text
const char *STR_BTN_MODERN[] = {"✨ Modern", "✨ 玻璃模式"};
const char *TIP_BTN_MODERN[] = {"Switch to Glass/Fluid UI", "切换至极简透明玻璃界面"};

const char *STR_BTN_EDIT_GLOS[] = {"📝 Edit Terms", "📝 编辑术语"};
const char *TIP_BTN_EDIT_GLOS[] = {"Open editor to modify glossary", "打开悬浮窗实时编辑当前术语表"};

const char *STR_GLOS_TITLE[] = {"📝 Glossary Editor", "📝 术语表编辑"};
const char *STR_GLOS_SAVE[] = {"💾 Save & Apply", "💾 保存并应用"};
const char *STR_GLOS_CLOSE[] = {"Close", "关闭"};
const char *STR_GLOS_WARN_TITLE[] = {"Warning", "警告"};
const char *STR_GLOS_WARN_MSG[] = {"Please select a glossary first!", "请先选择一个术语表！"};
const char *LOG_GLOS_SAVED[] = {"✅ Glossary updated and applied!", "✅ 术语表内容已更新并生效！"};

// Control button text / 控制按钮文本
const char *STR_START[] = {"Start Service", "启动服务"};
const char *STR_RELOAD[] = {"Hot Reload", "热重载配置"};
const char *STR_STOP[] = {"Stop Service", "停止服务"};
const char *STR_HUD[] = {"HUD Mode", "HUD 模式"};
const char *STR_TEST[] = {"Test Config", "测试配置"};
const char *STR_LOAD[] = {"Load Config", "读取配置"};
const char *STR_SAVE[] = {"Save Config", "保存配置"};
const char *STR_EXPORT[] = {"Export Log", "导出日志"};

// Theme and language text / 主题和语言文本
const char *STR_THEME_LIGHT[] = {"Light Mode", "切换亮色"};
const char *STR_THEME_DARK[] = {"Dark Mode", "切换暗色"};
const char *STR_LANG_BTN[] = {"中文", "English"};

// Glossary related text / 术语表相关文本
const char *STR_GLOSSARY[] = {"Glossary:", "术语表:"};
const char *STR_CHK_GLOSSARY[] = {"Self-Evolve", "启用自进化"};
const char *STR_CLEAR_LOG[] = {"Clear Log", "清空日志"};
const char *STR_REMOVE_PATH[] = {"Remove Current Path", "移除当前路径"};
const char *STR_CLEAR_HISTORY[] = {"Clear All History", "清空历史记录"};

// Token statistics text / Token统计文本
const char *STR_TOKENS[] = {"Tokens:", "消耗:"};
const char *TIP_TOKENS[] = {"Total Usage (Prompt + Completion)", "本次运行总消耗 (输入+输出)"};

// Context clearing related text / 上下文清除相关文本
const char *STR_CLEAR_CTX[] = {"Clr", "清空"};
const char *TIP_CLEAR_CTX[] = {"Clear Context", "清除历史对话记忆"};

// New button text / 新按钮文本
const char *STR_BTN_AUTO[] = {"Edit", "编辑"};
const char *TIP_BTN_AUTO[] = {
    "Open _AutoGeneratedTranslations.txt in same folder",
    "打开同目录下的 _AutoGeneratedTranslations.txt (自动翻译结果)"};

const char *STR_LOCK_SYS[] = {"Lock", "锁定"};
const char *TIP_LOCK_SYS[] = {
    "Lock System Prompt to prevent overwriting when loading config",
    "锁定系统提示词，防止在读取配置时被覆盖"};

// ==========================================
// Log message definitions (merged for completeness)
// 日志消息定义（合并以确保完整性）
// ==========================================

const char *LOG_TEST_START[] = {"📡 === Testing API Keys ===", "📡 === 开始测试所有 API Key ==="};
const char *LOG_NO_KEY[] = {"❌ No API Key", "❌ 未找到 API Key"};
const char *LOG_PASS[] = {"Pass", "测试通过"};
const char *LOG_FAIL[] = {"Fail", "失败"};
const char *LOG_FETCH_SUCCESS[] = {"Fetch Models Success", "模型列表获取成功"};
const char *LOG_FETCH_FAIL[] = {"Fetch Failed: ", "获取失败: "};
const char *LOG_PARSE_ERR[] = {"Parse Error", "解析错误"};
const char *LOG_CFG_SAVED[] = {"⚙️ Config Saved: ", "⚙️ 配置已保存: "};
const char *LOG_CFG_LOADED[] = {"📙 Config Loaded: ", "📙 配置已加载: "};
const char *LOG_EXPORTED[] = {"✒️ Log Exported to run_log.txt", "✒️ 日志已导出到 run_log.txt"};

// Enhanced Hot Reload logs (from robust logic) / 增强的热重载日志
const char *LOG_RELOADED[] = {"⚡ Config Hot Reloaded!", "⚡ 配置已热重载生效！"};
const char *LOG_MODEL_SWITCH[] = {"🔄 Model Switch: [%1] -> [%2]", "🔄 模型切换: [%1] -> [%2]"};
const char *LOG_MODEL_SAME[] = {"⚡ Reloaded (Model: %1)", "⚡ 热重载成功 (模型: %1)"};
const char *LOG_AUTO_TESTING[] = {"🛠️ Auto-testing new config...", "🛠️ 正在自动测试新配置..."};

// ==========================================
// Detailed tooltips
// 详细工具提示
// ==========================================

const char *TIP_PORT[] = {
    "Local Listening Port (Restart required to change)\nEnsure XUnity Endpoint is set to http://localhost:port",
    "本地监听端口 (修改需重启服务)\n请确保 XUnity 配置文件 Endpoint 设置为 http://localhost:端口号"};

const char *TIP_THREAD[] = {
    "Concurrent Threads\nThe effective range is 64 to 256\nThe default value is 100.",
    "并发线程数\n有效值：64 ~ 256\n默认为100"};

const char *TIP_TEMP[] = {
    "Sampling Temperature\n0.0-0.3: Strict\n0.7-1.0: Standard\n>1.0: Creative/Random",
    "采样温度 (Temperature)\n0.0-0.3: 严谨\n0.7-1.0: 标准\n>1.0: 随机/创造性"};

const char *TIP_CTX[] = {
    "Context Memory\nNumber of history turns to carry.\nNote: More context consumes more tokens.",
    "上下文记忆 (Context)\n携带的历史对话轮数。\n注意：上下文越多，消耗 Token 越多。"};

const char *TIP_GLOSSARY[] = {
    "Select XUnity's _Substitutions.txt.\nLLM will reference and append to it.  ",
    "选择 XUnity 的 _Substitutions.txt 文件。\nLLM 将自动参考并补充该文件。"};

const char *TIP_COMBO_MAIN[] = {
    "Enter API Address or select from list.\nMust support /v1/chat/completions format.",
    "在此输入 API 地址，或从下拉列表中选择主流服务商。\n所有地址必须兼容 OpenAI 接口格式 (/v1/chat/completions)。"};

// ==========================================
// API Presets
// 预设API地址
// ==========================================

struct ApiPresetDef
{
    const char *url;     // API URL / API地址
    const char *tips[2]; // Tooltip text in both languages / 双语工具提示文本
};

const ApiPresetDef PRESETS_DATA[] = {
    {"https://api.openai.com/v1", {"OpenAI Official API", "OpenAI 官方接口"}},
    {"https://api.deepseek.com", {"DeepSeek Official API", "DeepSeek 官方接口"}},
    {"https://api.x.ai/v1", {"Grok (xAI) Official API", "Grok (xAI) 官方接口"}},
    {"https://api.siliconflow.cn/v1", {"SiliconFlow", "硅基流动 (SiliconFlow)"}},
    {"https://openrouter.ai/api/v1", {"OpenRouter Aggregator", "OpenRouter 聚合平台"}},
    {"https://generativelanguage.googleapis.com/v1beta/openai", {"Google Gemini", "Google Gemini (OpenAI 兼容端点)"}},
    {"http://localhost:11434/v1", {"Ollama Local Service", "Ollama 本地服务"}},
    {"http://localhost:1234/v1", {"LM Studio Local Service", "LM Studio 本地服务"}}};

// ==========================================
// Implementation of MainWindow
// MainWindow 的实现
// ==========================================

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // 1. Initialize member variables / 初始化成员变量
    m_isClosing = false;
    m_isDarkTheme = true; // Default temporarily ; 临时设为true
    m_currentLang = 1;
    m_isServerRunning = false;

    setFixedSize(500, 800);

    // 2. Create core components / 创建核心组件
    m_tokenManager = new TokenManager(this);
    server = new TranslationServer(this);
    m_hudWindow = new HudWindow(nullptr);

    // 3. Set up UI / 设置UI
    setupUi();

    // 4. Connect signals and slots / 连接信号与槽
    connect(&LogManager::instance(), &LogManager::newLogAvailable, this, &MainWindow::onLogMessage);
    connect(&LogManager::instance(), &LogManager::logsCleared, logArea, &QTextEdit::clear);
    connect(server, &TranslationServer::tokenUsageReceived, m_tokenManager, &TokenManager::addUsage);
    connect(m_tokenManager, &TokenManager::tokensUpdated, this, &MainWindow::updateTokenDisplay);
    connect(m_hudWindow, &HudWindow::requestRestore, this, &MainWindow::restoreFromHud);
    connect(m_tokenManager, &TokenManager::tokensUpdated, [this](long long t, long long, long long)
            {
        if(m_hudWindow) m_hudWindow->updateTokens(t); });
    connect(server, &TranslationServer::serverStarted, this, [this]()
            { toggleControls(true); });
    connect(server, &TranslationServer::serverStopped, this, [this]()
            { toggleControls(false); });

    connect(server, &TranslationServer::workStarted, this, &MainWindow::onServerWorkStarted);
    connect(server, &TranslationServer::workFinished, this, &MainWindow::onServerWorkFinished);

    // 5. Load configuration / 加载配置
    loadConfigToUi();

    updateUIText();

    // Apply theme based on loaded configuration / 根据加载的配置应用主题
    applyTheme(m_isDarkTheme);

    // 6. Fade-in animation / 淡入动画
    setWindowOpacity(0.0);
    fadeAnim = new QPropertyAnimation(this, "windowOpacity");
    fadeAnim->setDuration(500);
    fadeAnim->setStartValue(0.0);
    fadeAnim->setEndValue(1.0);
    fadeAnim->start();
}

MainWindow::~MainWindow()
{
    // Clean up resources / 清理资源
    if (m_hudWindow)
    {
        m_hudWindow->close();
        delete m_hudWindow;
    }
    server->stopServer();
}

/**
 * Fade out the window and close the application.
 * 淡出窗口并关闭应用程序。
 */
void MainWindow::fadeOutAndClose()
{
    fadeAnim->setDirection(QAbstractAnimation::Backward);
    connect(fadeAnim, &QPropertyAnimation::finished, this, &QMainWindow::close);
    connect(fadeAnim, &QPropertyAnimation::finished, qApp, &QApplication::quit);
    fadeAnim->start();
}

/**
 * Perform a smooth visual transition when changing UI state.
 * 在更改UI状态时执行平滑视觉过渡。
 * @param changeLogic The actual UI change to perform.
 */
void MainWindow::smoothSwitch(std::function<void()> changeLogic)
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
    anim->setDuration(300);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);

    connect(anim, &QPropertyAnimation::finished, overlay, &QLabel::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

/**
 * Toggle the interface language.
 * 切换界面语言。
 */
void MainWindow::toggleLanguage()
{
    AppConfig currentCfg = getUiConfig();

    smoothSwitch([this, currentCfg]()
                 {
                     m_currentLang = (m_currentLang == 0) ? 1 : 0;

                     updateUIText();

                     apiAddressCombo->setCurrentText(currentCfg.api_address);

                     if (themeBtn)
                         themeBtn->setText(m_isDarkTheme ? STR_THEME_LIGHT[m_currentLang] : STR_THEME_DARK[m_currentLang]);
                     toggleControls(m_isServerRunning);

                     AppConfig finalCfg = currentCfg;
                     finalCfg.language = m_currentLang;
                     server->updateConfig(finalCfg); });
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // Synchronize UI with server's live configuration
    // 将UI与服务端的实时配置同步
    if (server)
    {
        AppConfig liveCfg = server->getConfig();

        // 1. Language
        if (m_currentLang != liveCfg.language)
        {
            m_currentLang = liveCfg.language;
            updateUIText();
        }

        // 2. Glossary path (ignore lock because this is internal sync)
        if (!liveCfg.glossary_path.isEmpty())
        {
            if (glossaryCombo->findText(liveCfg.glossary_path) == -1)
            {
                glossaryCombo->insertItem(0, liveCfg.glossary_path);
            }
            glossaryCombo->setCurrentText(liveCfg.glossary_path);
        }

        // 3. Self-evolution checkbox
        chkGlossary->setChecked(liveCfg.enable_glossary);

        // 4. Lock states
        chkLockGlossary->setChecked(liveCfg.lock_glossary);
        chkLockSysPrompt->setChecked(liveCfg.lock_system_prompt);

        // 5. API address
        apiAddressCombo->setCurrentText(liveCfg.api_address);

        // Move cursor to start of path line edit
        if (glossaryCombo->lineEdit())
        {
            glossaryCombo->lineEdit()->setCursorPosition(0);
        }
    }

    // Adjust token label position
    if (logGroup && lblTokens)
    {
        lblTokens->adjustSize();
        if (logGroup->width() > 0)
        {
            lblTokens->move(logGroup->width() - lblTokens->width() - 10, 0);
        }
    }
}

/**
 * Toggle the application theme (dark/light).
 * 切换应用程序主题（暗色/亮色）。
 */
void MainWindow::toggleTheme()
{
    smoothSwitch([this]()
                 {
        applyTheme(!m_isDarkTheme);
        toggleControls(m_isServerRunning); });
}

/**
 * Add a path to the glossary history combo box.
 * 将路径添加到术语表历史组合框中。
 * @param path The file path to add.
 */
void MainWindow::addToGlossaryHistory(const QString &path)
{
    if (path.isEmpty())
        return;

    QStringList items;
    for (int i = 0; i < glossaryCombo->count(); ++i)
    {
        items << glossaryCombo->itemText(i);
    }

    items.removeAll(path);
    items.insert(0, path);

    while (items.size() > 10)
    {
        items.removeLast();
    }

    glossaryCombo->clear();
    glossaryCombo->addItems(items);
    glossaryCombo->setCurrentIndex(0);

    if (glossaryCombo->lineEdit())
    {
        glossaryCombo->lineEdit()->setCursorPosition(0);
    }
}

/**
 * Open a file dialog to select a glossary file.
 * 打开文件对话框选择术语表文件。
 */
void MainWindow::onSelectGlossary()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Glossary"), "", "_Substitutions (*.txt);;All Files (*)");
    if (!fileName.isEmpty())
    {
        addToGlossaryHistory(fileName);
        if (m_isServerRunning && server)
        {
            AppConfig cfg = getUiConfig();
            server->updateConfig(cfg);
            LogManager::instance().addLog(m_currentLang == 0 ? "✅ New glossary applied." : "✅ 新术语表已应用。");
        }
    }
}

/**
 * Open the automatically generated translations file (_AutoGeneratedTranslations.txt)
 * located in the same directory as the current glossary.
 * 打开与当前术语表同一目录下的自动翻译文件 (_AutoGeneratedTranslations.txt)。
 */
void MainWindow::onOpenAutoTranslations()
{
    QString currentPath = glossaryCombo->currentText();

    if (currentPath.isEmpty() || currentPath == "未选择" || currentPath == "None")
    {
        QMessageBox::warning(this,
                             (m_currentLang == 1 ? "⚠️ 提示" : "⚠️ Notice"),
                             (m_currentLang == 1 ? "请先选择一个有效的术语表路径！\n程序需要基于该路径来定位自动翻译记录文件。"
                                                 : "Please select a valid glossary path first!\nThe program needs it to locate the auto-translation file."));
        return;
    }

    QFileInfo fi(currentPath);
    QString dir = fi.absolutePath();
    QString targetFile = dir + "/_AutoGeneratedTranslations.txt";

    QFileInfo targetFi(targetFile);
    if (!targetFi.exists())
    {
        QMessageBox::warning(this,
                             (m_currentLang == 1 ? "文件未找到" : "File Not Found"),
                             (m_currentLang == 1 ? "未找到 _AutoGeneratedTranslations.txt。\n请确认游戏是否已经运行并生成了翻译。"
                                                 : "Could not find _AutoGeneratedTranslations.txt.\nPlease ensure the game has run and generated translations."));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(targetFile));
}

/**
 * Update all UI texts according to the current language.
 * 根据当前语言更新所有UI文本。
 */
void MainWindow::updateUIText()
{
    int i = m_currentLang;

    QString currentApi = apiAddressCombo->currentText();
    QSignalBlocker blocker(apiAddressCombo);

    setWindowTitle(STR_TITLE[i]);

    cfgGroup->setTitle(STR_API_CFG[i]);
    logGroup->setTitle(STR_LOG_AREA[i]);

    lblApiAddr->setText(STR_API_ADDR[i]);
    lblApiKey->setText(STR_API_KEY[i]);
    lblModel->setText(STR_MODEL[i]);
    fetchModelBtn->setText(STR_FETCH[i]);

    lblPort->setText(STR_PORT[i]);
    lblThread->setText(STR_THREAD[i]);
    lblTemp->setText(STR_TEMP[i]);
    lblCtx->setText(STR_CTX[i]);

    lblSysPrompt->setText(STR_SYS_PROMPT[i]);

    chkLockSysPrompt->setText(STR_LOCK_SYS[i]);
    chkLockSysPrompt->setToolTip(TIP_LOCK_SYS[i]);

    lblPrePrompt->setText(STR_PRE_PROMPT[i]);

    clearCtxBtn->setText(STR_CLEAR_CTX[i]);
    clearCtxBtn->setToolTip(TIP_CLEAR_CTX[i]);

    lblGlossary->setText(STR_GLOSSARY[i]);
    chkGlossary->setText(STR_CHK_GLOSSARY[i]);

    chkDebug->setText(STR_DEBUG_MODE[i]);
    chkDebug->setToolTip(TIP_DEBUG_MODE[i]);

    chkBatch->setText(STR_BATCH_MODE[i]);
    chkBatch->setToolTip(TIP_BATCH_MODE[i]);

    if (m_isServerRunning)
    {
        startBtn->setText(STR_RELOAD[i]);
    }
    else
    {
        startBtn->setText(STR_START[i]);
    }

    stopBtn->setText(STR_STOP[i]);
    hudBtn->setText(STR_HUD[i]);
    testBtn->setText(STR_TEST[i]);
    loadBtn->setText(STR_LOAD[i]);
    saveBtn->setText(STR_SAVE[i]);
    exportBtn->setText(STR_EXPORT[i]);
    langBtn->setText(STR_LANG_BTN[i]);

    if (themeBtn)
    {
        if (m_isDarkTheme)
        {
            themeBtn->setText(STR_THEME_LIGHT[i]);
        }
        else
        {
            themeBtn->setText(STR_THEME_DARK[i]);
        }
    }

    modernBtn->setText(STR_BTN_MODERN[i]);
    modernBtn->setToolTip(TIP_BTN_MODERN[i]);

    if (editGlossaryBtn)
    {
        editGlossaryBtn->setText(STR_BTN_EDIT_GLOS[i]);
        editGlossaryBtn->setToolTip(TIP_BTN_EDIT_GLOS[i]);
    }

    if (btnOpenAuto)
    {
        btnOpenAuto->setText(STR_BTN_AUTO[i]);
        btnOpenAuto->setToolTip(TIP_BTN_AUTO[i]);
    }

    portEdit->setToolTip(TIP_PORT[i]);
    lblPort->setToolTip(TIP_PORT[i]);
    threadSpin->setToolTip(TIP_THREAD[i]);
    lblThread->setToolTip(TIP_THREAD[i]);
    tempSpin->setToolTip(TIP_TEMP[i]);
    lblTemp->setToolTip(TIP_TEMP[i]);
    contextSpin->setToolTip(TIP_CTX[i]);
    lblCtx->setToolTip(TIP_CTX[i]);

    lblGlossary->setToolTip(TIP_GLOSSARY[i]);
    chkGlossary->setToolTip(TIP_GLOSSARY[i]);
    btnSelectGlossary->setToolTip(TIP_GLOSSARY[i]);
    if (glossaryCombo)
        glossaryCombo->setToolTip(TIP_GLOSSARY[i]);

    hudBtn->setToolTip(i == 0 ? "Switch to Mini-HUD mode" : "切换至迷你悬浮窗模式");

    if (apiAddressCombo)
    {
        apiAddressCombo->setToolTip(TIP_COMBO_MAIN[i]);
        for (int k = 0; k < apiAddressCombo->count(); ++k)
        {
            QString itemUrl = apiAddressCombo->itemText(k);
            for (const auto &preset : PRESETS_DATA)
            {
                if (itemUrl == preset.url)
                {
                    apiAddressCombo->setItemData(k, preset.tips[i], Qt::ToolTipRole);
                    break;
                }
            }
        }
        apiAddressCombo->setCurrentText(currentApi);
    }

    long long t = lblTokens->property("total").toLongLong();
    long long p = lblTokens->property("prompt").toLongLong();
    long long c = lblTokens->property("completion").toLongLong();

    updateTokenDisplay(t, p, c);

    chkLockGlossary->setText(STR_LOCK_SYS[i]);
    chkLockGlossary->setToolTip(i == 1 ? "锁定术语表路径，防止读取配置时被覆盖" : "Lock Glossary Path to prevent overwriting");

    if (logGroup && lblTokens)
    {
        lblTokens->adjustSize();
        if (logGroup->width() > 0)
        {
            lblTokens->move(logGroup->width() - lblTokens->width() - 10, 0);
        }
    }

    if (m_glossaryEditor)
    {
        m_glossaryEditor->setWindowTitle(STR_GLOS_TITLE[i]);
        m_glossarySaveBtn->setText(STR_GLOS_SAVE[i]);
        m_glossaryCancelBtn->setText(STR_GLOS_CLOSE[i]);
    }
}

/**
 * Apply the specified theme (dark/light) to the application.
 * 将指定的主题（暗色/亮色）应用到应用程序。
 * @param isDark True for dark theme, false for light theme.
 */
void MainWindow::applyTheme(bool isDark)
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QColor windowColor, baseColor, textColor, btnColor, highlightColor, linkColor;
    QString qssBtnBorder, qssBtnBg, qssBtnHover;
    QString dropDownBg, dropDownHover;

    if (isDark)
    {
        windowColor = QColor(30, 30, 30);
        baseColor = QColor(37, 37, 38);
        textColor = QColor(220, 220, 220);
        btnColor = QColor(60, 60, 60);
        highlightColor = QColor(0, 122, 204);
        linkColor = QColor(86, 156, 214);
        qssBtnBorder = "#555555";
        qssBtnBg = "#3C3C3C";
        qssBtnHover = "#505050";
        dropDownBg = "#C0C0C0";
        dropDownHover = "#FFFFFF";
        if (themeBtn)
            themeBtn->setText(STR_THEME_LIGHT[m_currentLang]);
    }
    else
    {
        windowColor = QColor(240, 240, 240);
        baseColor = QColor(255, 255, 255);
        textColor = QColor(0, 0, 0);
        btnColor = QColor(225, 225, 225);
        highlightColor = QColor(0, 120, 215);
        linkColor = QColor(0, 0, 255);
        qssBtnBorder = "#C0C0C0";
        qssBtnBg = "#E1E1E1";
        qssBtnHover = "#D0D0D0";
        dropDownBg = "#4D4D4D";
        dropDownHover = "#2D2D2D";
        if (themeBtn)
            themeBtn->setText(STR_THEME_DARK[m_currentLang]);
    }

    QPalette p;
    p.setColor(QPalette::Window, windowColor);
    p.setColor(QPalette::WindowText, textColor);
    p.setColor(QPalette::Base, baseColor);
    p.setColor(QPalette::AlternateBase, windowColor);
    p.setColor(QPalette::ToolTipBase, baseColor);
    p.setColor(QPalette::ToolTipText, textColor);
    p.setColor(QPalette::Text, textColor);
    p.setColor(QPalette::Button, btnColor);
    p.setColor(QPalette::ButtonText, textColor);
    p.setColor(QPalette::Link, linkColor);
    p.setColor(QPalette::Highlight, highlightColor);
    p.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(p);

    QString qss = QString(R"(
        QGroupBox { border: 1px solid %1; border-radius: 5px; margin-top: 1.2em; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 3px; color: %6; }
        QPushButton { border: 1px solid %3; border-radius: 4px; background-color: %4; padding: 5px; font-weight: bold; }
        QPushButton:hover { background-color: %5; border-color: %2; }
        QPushButton:pressed { background-color: %2; color: white; border-color: %2; }
        QPushButton:disabled { background-color: transparent; border: 1px solid %1; color: gray; }
        
        QLineEdit, QComboBox { border: 1px solid %3; border-radius: 4px; background-color: %7; padding: 4px; color: palette(text); selection-background-color: %2; }
        QComboBox:hover, QLineEdit:hover { border-color: %2; }
        
        QComboBox::drop-down { 
            subcontrol-origin: padding; 
            subcontrol-position: top right; 
            width: 20px; 
            background: transparent;
            border-left: 1px solid %3; 
        }
        QComboBox::drop-down:hover { background: rgba(128, 128, 128, 0.15); }
        QComboBox::down-arrow { 
            image: none; 
            width: 10px; 
            height: 2px;
            background-color: %8;
        }
        QComboBox QAbstractItemView { border: 1px solid %2; selection-background-color: %2; background-color: %7; outline: none; }
        
        QPushButton#btnStart { background-color: #388E3C; color: white; border: 1px solid #2E7D32; }
        QPushButton#btnStart:hover { background-color: #4CAF50; border-color: #43A047; }
        QPushButton#btnStart:pressed { background-color: #1B5E20; border-color: #1B5E20; }
        QPushButton#btnStart:disabled { background-color: transparent; border: 1px solid %1; color: gray; }

        QPushButton#btnReload { background-color: #0078D4; color: white; border: 1px solid #005A9E; }
        QPushButton#btnReload:hover { background-color: #2B88D8; border-color: #005A9E; }
        QPushButton#btnReload:pressed { background-color: #004578; border-color: #004578; }

        QLabel#lblTokens { color: #E6B422; font-weight: bold; }
        QToolTip { border: 1px solid %2; background-color: %7; color: #E6B422; opacity: 230; padding: 4px; border-radius: 3px; }
    )")
                      .arg(qssBtnBorder)
                      .arg(highlightColor.name())
                      .arg(qssBtnBorder)
                      .arg(qssBtnBg)
                      .arg(qssBtnHover)
                      .arg(highlightColor.name())
                      .arg(baseColor.name())
                      .arg(textColor.name())
                      .arg(dropDownBg)
                      .arg(dropDownHover);

    qApp->setStyleSheet(qss);
    m_isDarkTheme = isDark;

    // Update glossary editor theme
    if (m_glossaryTextEdit)
    {
        if (isDark)
        {
            m_glossaryTextEdit->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; font-family: Consolas, monospace; font-size: 13px; border-radius: 4px; border: 1px solid #555555;}");
        }
        else
        {
            m_glossaryTextEdit->setStyleSheet("QTextEdit { background-color: #f5f5f5; color: #333333; font-family: Consolas, monospace; font-size: 13px; border-radius: 4px; border: 1px solid #cccccc;}");
        }

        auto highlighters = m_glossaryTextEdit->document()->findChildren<QSyntaxHighlighter *>();
        if (!highlighters.isEmpty())
        {
            static_cast<GlossaryHighlighter *>(highlighters.first())->setTheme(isDark);
        }
    }
}

// ==========================================
// Complete setupUi function
// 完整的setupUi函数
// ==========================================
void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    cfgGroup = new QGroupBox(this);
    cfgGroup->setStyleSheet("");

    modernBtn = new QPushButton(STR_BTN_MODERN[m_currentLang], cfgGroup);
    modernBtn->setCursor(Qt::PointingHandCursor);
    modernBtn->setFixedSize(88, 17);
    modernBtn->setToolTip(TIP_BTN_MODERN[m_currentLang]);
    modernBtn->setStyleSheet(
        "QPushButton { background-color: rgba(156, 39, 176, 0.08); color: #9C27B0; border: 1px solid rgba(156, 39, 176, 0.5); border-radius: 4px; font-size: 11px; font-weight: bold; text-align: center; padding: 0px; } "
        "QPushButton:hover { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 #E6B422, stop:1 #FFD700); color: #222222; border: 1px solid #FFD700; } "
        "QPushButton:pressed { background-color: #B8860B; border-color: #B8860B; }");
    connect(modernBtn, &QPushButton::clicked, this, &MainWindow::onSwitchToModern);
    modernBtn->move(300, 0);
    modernBtn->raise();

    editGlossaryBtn = new QPushButton(STR_BTN_EDIT_GLOS[m_currentLang], cfgGroup);
    editGlossaryBtn->setCursor(Qt::PointingHandCursor);
    editGlossaryBtn->setFixedSize(88, 17);
    editGlossaryBtn->setToolTip(TIP_BTN_EDIT_GLOS[m_currentLang]);
    editGlossaryBtn->setStyleSheet(
        "QPushButton { background-color: rgba(33, 150, 243, 0.08); color: #2196F3; border: 1px solid rgba(33, 150, 243, 0.5); border-radius: 4px; font-size: 11px; font-weight: bold; text-align: center; padding: 0px; } "
        "QPushButton:hover { background-color: rgba(33, 150, 243, 0.8); color: white; border: 1px solid #2196F3; } "
        "QPushButton:pressed { background-color: #1976D2; }");
    connect(editGlossaryBtn, &QPushButton::clicked, this, &MainWindow::openGlossaryEditor);
    editGlossaryBtn->move(391, 0);
    editGlossaryBtn->raise();

    QGridLayout *grid = new QGridLayout(cfgGroup);
    grid->setColumnStretch(1, 1);
    grid->setVerticalSpacing(8);
    grid->setHorizontalSpacing(10);

    auto createLabel = [this](QLabel *&memberPtr)
    {
        memberPtr = new QLabel(this);
        memberPtr->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return memberPtr;
    };

    // Row 0: API Address
    apiAddressCombo = new QComboBox(this);
    apiAddressCombo->setEditable(true);
    apiAddressCombo->setMinimumHeight(28);
    for (const auto &p : PRESETS_DATA)
    {
        apiAddressCombo->addItem(p.url);
        apiAddressCombo->setItemData(apiAddressCombo->count() - 1, p.tips[m_currentLang], Qt::ToolTipRole);
    }
    apiAddressCombo->setCurrentIndex(0);

    grid->addWidget(createLabel(lblApiAddr), 0, 0);
    grid->addWidget(apiAddressCombo, 0, 1);

    // Row 1: API Key
    apiKeyEdit = new QLineEdit(this);
    apiKeyEdit->setEchoMode(QLineEdit::Normal);
    grid->addWidget(createLabel(lblApiKey), 1, 0);
    grid->addWidget(apiKeyEdit, 1, 1);

    // Row 2: Model Selection
    QWidget *modelContainer = new QWidget(this);
    QHBoxLayout *modelLayout = new QHBoxLayout(modelContainer);
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelLayout->setSpacing(5);

    modelCombo = new QComboBox(this);
    modelCombo->setEditable(true);
    modelCombo->setMinimumHeight(28);

    fetchModelBtn = new QPushButton(this);
    connect(fetchModelBtn, &QPushButton::clicked, this, &MainWindow::onFetchModels);

    modelLayout->addWidget(modelCombo, 1);
    modelLayout->addWidget(fetchModelBtn);
    grid->addWidget(createLabel(lblModel), 2, 0);
    grid->addWidget(modelContainer, 2, 1);

    // Row 3: Server Parameters
    grid->addWidget(createLabel(lblPort), 3, 0);

    QWidget *paramContainer = new QWidget(this);
    QHBoxLayout *paramLayout = new QHBoxLayout(paramContainer);
    paramLayout->setContentsMargins(0, 0, 0, 0);
    paramLayout->setSpacing(4);

    portEdit = new QLineEdit(this);
    portEdit->setMinimumWidth(48);
    portEdit->setAlignment(Qt::AlignCenter);

    lblThread = new QLabel(this);
    threadSpin = new QSpinBox(this);
    threadSpin->setRange(0, 1000);
    threadSpin->setMinimumWidth(50);
    threadSpin->setAlignment(Qt::AlignCenter);

    lblTemp = new QLabel(this);
    tempSpin = new QDoubleSpinBox(this);
    tempSpin->setRange(0, 2);
    tempSpin->setSingleStep(0.1);
    tempSpin->setMinimumWidth(50);
    tempSpin->setAlignment(Qt::AlignCenter);

    lblCtx = new QLabel(this);
    contextSpin = new QSpinBox(this);
    contextSpin->setRange(0, 20);
    contextSpin->setMinimumWidth(50);
    contextSpin->setAlignment(Qt::AlignCenter);

    clearCtxBtn = new QPushButton(this);
    clearCtxBtn->setFixedWidth(40);
    connect(clearCtxBtn, &QPushButton::clicked, this, &MainWindow::onClearContext);

    lblTokens = new QLabel(this);
    lblTokens->setObjectName("lblTokens");

    paramLayout->addWidget(portEdit, 1);
    paramLayout->addWidget(lblThread);
    paramLayout->addWidget(threadSpin, 1);
    paramLayout->addWidget(lblTemp);
    paramLayout->addWidget(tempSpin, 1);
    paramLayout->addWidget(lblCtx);
    paramLayout->addWidget(contextSpin, 1);
    paramLayout->addWidget(clearCtxBtn);

    grid->addWidget(paramContainer, 3, 1);

    // Row 4: System Prompt
    systemPromptEdit = new QTextEdit(this);
    systemPromptEdit->setMinimumHeight(100);

    QWidget *sysLabelContainer = new QWidget(this);
    QVBoxLayout *sysLabelLayout = new QVBoxLayout(sysLabelContainer);
    sysLabelLayout->setContentsMargins(0, 0, 0, 0);
    sysLabelLayout->setSpacing(4);

    lblSysPrompt = new QLabel(this);
    lblSysPrompt->setAlignment(Qt::AlignRight | Qt::AlignTop);

    chkLockSysPrompt = new QCheckBox(this);
    chkLockSysPrompt->setLayoutDirection(Qt::RightToLeft);
    QWidget *lockContainer = new QWidget(this);
    QHBoxLayout *lockLayout = new QHBoxLayout(lockContainer);
    lockLayout->setContentsMargins(0, 0, 0, 0);
    lockLayout->addStretch();
    lockLayout->addWidget(chkLockSysPrompt);

    chkDebug = new QCheckBox(this);
    chkDebug->setObjectName("chkDebug");
    chkDebug->setCursor(Qt::PointingHandCursor);
    chkDebug->setLayoutDirection(Qt::RightToLeft);

    QWidget *debugContainer = new QWidget(this);
    QHBoxLayout *debugLayout = new QHBoxLayout(debugContainer);
    debugLayout->setContentsMargins(0, 0, 0, 0);
    debugLayout->addStretch();
    debugLayout->addWidget(chkDebug);

    connect(chkDebug, &QCheckBox::toggled, this, [this](bool checked)
            {
        QString msg = (m_currentLang == 1) ?
            QString("🛠️ 测速模式: %1").arg(checked ? "已开启" : "已关闭") :
            QString("🛠️ Speed Test Mode: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);

        if (m_isServerRunning && server) {
            server->updateConfig(getUiConfig());
        } });

    chkBatch = new QCheckBox(this);
    chkBatch->setObjectName("chkBatch");
    chkBatch->setCursor(Qt::PointingHandCursor);
    chkBatch->setLayoutDirection(Qt::RightToLeft);

    connect(chkBatch, &QCheckBox::toggled, this, [this](bool checked)
            {
        QString msg = (m_currentLang == 1) ?
            QString("📦 多行模式: %1").arg(checked ? "已开启" : "已关闭") :
            QString("📦 Batch Mode: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);

        if (m_isServerRunning && server) {
            server->updateConfig(getUiConfig());
        } });

    QWidget *batchContainer = new QWidget(this);
    QHBoxLayout *batchLayout = new QHBoxLayout(batchContainer);
    batchLayout->setContentsMargins(0, 0, 0, 0);
    batchLayout->addStretch();
    batchLayout->addWidget(chkBatch);

    sysLabelLayout->addWidget(lblSysPrompt);
    sysLabelLayout->addWidget(lockContainer);
    sysLabelLayout->addWidget(debugContainer);
    sysLabelLayout->addWidget(batchContainer);
    sysLabelLayout->addStretch();

    grid->addWidget(sysLabelContainer, 4, 0, Qt::AlignTop);
    grid->addWidget(systemPromptEdit, 4, 1);

    // Row 5: Pre-Prompt
    prePromptEdit = new QLineEdit(this);
    grid->addWidget(createLabel(lblPrePrompt), 5, 0);
    grid->addWidget(prePromptEdit, 5, 1);

    // Row 6: Glossary
    QWidget *glossaryContainer = new QWidget(this);
    QHBoxLayout *glossaryLayout = new QHBoxLayout(glossaryContainer);
    glossaryLayout->setContentsMargins(0, 0, 0, 0);

    chkGlossary = new QCheckBox(this);

    glossaryCombo = new QComboBox(this);
    glossaryCombo->setEditable(true);
    glossaryCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(glossaryCombo, &QComboBox::customContextMenuRequested, this, &MainWindow::onGlossaryContextMenu);

    glossaryCombo->setMinimumHeight(28);
    glossaryCombo->setMinimumWidth(0);
    glossaryCombo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    btnOpenAuto = new QPushButton(STR_BTN_AUTO[m_currentLang], this);
    btnOpenAuto->setFixedWidth(50);
    connect(btnOpenAuto, &QPushButton::clicked, this, &MainWindow::onOpenAutoTranslations);

    btnSelectGlossary = new QPushButton("...", this);
    btnSelectGlossary->setFixedWidth(35);
    connect(btnSelectGlossary, &QPushButton::clicked, this, &MainWindow::onSelectGlossary);
    btnSelectGlossary->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(btnSelectGlossary, &QPushButton::customContextMenuRequested, this, &MainWindow::onGlossaryContextMenu);

    glossaryLayout->addWidget(chkGlossary);
    glossaryLayout->addWidget(glossaryCombo, 1);
    glossaryLayout->addWidget(btnOpenAuto);
    glossaryLayout->addWidget(btnSelectGlossary);

    QWidget *glossaryLabelContainer = new QWidget(this);
    QVBoxLayout *glossaryLabelLayout = new QVBoxLayout(glossaryLabelContainer);
    glossaryLabelLayout->setContentsMargins(0, 0, 0, 0);
    glossaryLabelLayout->setSpacing(2);

    lblGlossary = new QLabel(this);
    lblGlossary->setAlignment(Qt::AlignRight | Qt::AlignTop);

    chkLockGlossary = new QCheckBox(this);
    chkLockGlossary->setLayoutDirection(Qt::RightToLeft);

    QWidget *gLockContainer = new QWidget(this);
    QHBoxLayout *gLockLayout = new QHBoxLayout(gLockContainer);
    gLockLayout->setContentsMargins(0, 0, 0, 0);
    gLockLayout->addStretch();
    gLockLayout->addWidget(chkLockGlossary);

    glossaryLabelLayout->addWidget(lblGlossary);
    glossaryLabelLayout->addWidget(gLockContainer);
    glossaryLabelLayout->addStretch();

    grid->addWidget(glossaryLabelContainer, 6, 0);
    grid->addWidget(glossaryContainer, 6, 1);

    mainLayout->addWidget(cfgGroup);

    // Control buttons grid
    QGridLayout *btnGridLayout = new QGridLayout();
    btnGridLayout->setSpacing(10);

    auto createBtn = [this](QPushButton *&btnPtr)
    {
        btnPtr = new QPushButton(this);
        btnPtr->setMinimumHeight(32);
        btnPtr->setCursor(Qt::PointingHandCursor);
        return btnPtr;
    };

    btnGridLayout->addWidget(createBtn(startBtn), 0, 0);
    startBtn->setObjectName("btnStart");

    btnGridLayout->addWidget(createBtn(stopBtn), 0, 1);
    stopBtn->setEnabled(false);

    hudBtn = new QPushButton(this);
    hudBtn->setMinimumHeight(32);
    hudBtn->setCursor(Qt::PointingHandCursor);
    hudBtn->setEnabled(false);
    connect(hudBtn, &QPushButton::clicked, this, &MainWindow::switchToHud);
    btnGridLayout->addWidget(hudBtn, 0, 2);

    btnGridLayout->addWidget(createBtn(testBtn), 1, 0);
    btnGridLayout->addWidget(createBtn(exportBtn), 1, 1);
    btnGridLayout->addWidget(createBtn(loadBtn), 1, 2);

    btnGridLayout->addWidget(createBtn(saveBtn), 2, 0);
    btnGridLayout->addWidget(createBtn(themeBtn), 2, 1);
    btnGridLayout->addWidget(createBtn(langBtn), 2, 2);

    connect(themeBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(langBtn, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(testBtn, &QPushButton::clicked, this, &MainWindow::onTestConfig);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadConfig);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveConfig);
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportLog);

    mainLayout->addLayout(btnGridLayout);

    logGroup = new QGroupBox(this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(10, 20, 10, 10);

    logArea = new QTextEdit(this);
    logArea->setReadOnly(true);
    logArea->setMinimumHeight(150);
    logArea->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(logArea, &QTextEdit::customContextMenuRequested, this, &MainWindow::onLogContextMenu);
    logLayout->addWidget(logArea);
    mainLayout->addWidget(logGroup);

    lblTokens->setParent(logGroup);
    lblTokens->raise();

    fetchLoadingOverlay = new LoadingOverlay(fetchModelBtn);
    testLoadingOverlay = new LoadingOverlay(testBtn);

    connect(glossaryCombo, &QComboBox::activated, this, &MainWindow::onGlossaryChanged);

    connect(glossaryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index)
            {
        Q_UNUSED(index);
        QTimer::singleShot(0, [this](){
            if (glossaryCombo->lineEdit()) {
                glossaryCombo->lineEdit()->setCursorPosition(0);
            }
        }); });
}

/**
 * Handle context menu request for the log area.
 * 处理日志区域的上下文菜单请求。
 * @param pos Position where the menu was requested.
 */
void MainWindow::onLogContextMenu(const QPoint &pos)
{
    QMenu *menu = logArea->createStandardContextMenu();
    menu->addSeparator();

    QAction *clearAction = menu->addAction(STR_CLEAR_LOG[m_currentLang]);
    connect(clearAction, &QAction::triggered, []()
            { LogManager::instance().clear(); });

    menu->exec(logArea->mapToGlobal(pos));
    delete menu;
}

/**
 * Get the current UI state as an AppConfig structure.
 * 将当前UI状态作为AppConfig结构体获取。
 * @return AppConfig containing current settings.
 */
AppConfig MainWindow::getUiConfig()
{
    AppConfig cfg;

    // Load saved config to retain modern_opacity and other persistent values
    AppConfig savedCfg = ConfigManager::loadConfig();
    cfg.modern_opacity = savedCfg.modern_opacity;

    cfg.api_address = apiAddressCombo->currentText();
    cfg.api_key = apiKeyEdit->text();
    cfg.model_name = modelCombo->currentText();
    cfg.port = portEdit->text().toInt();
    cfg.temperature = tempSpin->value();
    cfg.context_num = contextSpin->value();
    cfg.max_threads = threadSpin->value();
    cfg.system_prompt = systemPromptEdit->toPlainText();
    cfg.pre_prompt = prePromptEdit->text();
    cfg.enable_glossary = chkGlossary->isChecked();
    cfg.enable_debug_mode = chkDebug->isChecked();
    cfg.enable_batch = chkBatch->isChecked();

    cfg.glossary_path = glossaryCombo->currentText();
    QStringList history;
    for (int i = 0; i < glossaryCombo->count(); ++i)
    {
        history << glossaryCombo->itemText(i);
    }
    cfg.glossary_history = history;
    cfg.language = m_currentLang;

    cfg.is_dark = m_isDarkTheme;
    cfg.ui_mode = 0; // Classic mode

    cfg.lock_system_prompt = chkLockSysPrompt->isChecked();
    cfg.lock_glossary = chkLockGlossary->isChecked();

    return cfg;
}

/**
 * Load configuration from file and update the UI accordingly.
 * 从文件加载配置并相应更新UI。
 */
void MainWindow::loadConfigToUi()
{
    AppConfig cfg = ConfigManager::loadConfig();

    m_currentLang = cfg.language;
    if (m_isDarkTheme != cfg.is_dark)
    {
        m_isDarkTheme = cfg.is_dark;
        applyTheme(m_isDarkTheme);
    }

    logArea->clear();
    QStringList logs = LogManager::instance().getHistory();
    for (const QString &msg : logs)
    {
        onLogMessage(msg);
    }

    chkLockSysPrompt->setChecked(cfg.lock_system_prompt);
    chkLockGlossary->setChecked(cfg.lock_glossary);

    apiAddressCombo->setCurrentText(cfg.api_address);
    apiKeyEdit->setText(cfg.api_key);
    modelCombo->setCurrentText(cfg.model_name);
    portEdit->setText(QString::number(cfg.port));
    tempSpin->setValue(cfg.temperature);
    contextSpin->setValue(cfg.context_num);
    threadSpin->setValue(cfg.max_threads);

    prePromptEdit->setText(cfg.pre_prompt);

    chkDebug->setChecked(cfg.enable_debug_mode);
    chkBatch->setChecked(cfg.enable_batch);
    chkGlossary->setChecked(cfg.enable_glossary);

    if (!cfg.lock_system_prompt || systemPromptEdit->toPlainText().isEmpty())
    {
        systemPromptEdit->setText(cfg.system_prompt);
    }

    if (!chkLockGlossary->isChecked())
    {
        if (!cfg.glossary_history.isEmpty())
        {
            glossaryCombo->clear();
            glossaryCombo->addItems(cfg.glossary_history);
        }

        if (!cfg.glossary_path.isEmpty())
        {
            int index = glossaryCombo->findText(cfg.glossary_path);
            if (index != -1)
            {
                glossaryCombo->setCurrentIndex(index);
            }
            else
            {
                addToGlossaryHistory(cfg.glossary_path);
            }
        }
    }

    if (glossaryCombo->lineEdit())
    {
        glossaryCombo->lineEdit()->setCursorPosition(0);
    }

    updateUIText();
    apiAddressCombo->setCurrentText(cfg.api_address);

    toggleControls(server->isRunning());

    if (server)
    {
        server->updateConfig(cfg);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isClosing)
    {
        event->accept();
        return;
    }

    ConfigManager::saveConfig(getUiConfig(), "config.ini");

    if (m_hudWindow->isVisible())
    {
        m_hudWindow->close();
    }

    event->ignore();
    m_isClosing = true;
    fadeOutAndClose();
}

/**
 * Enable/disable UI controls based on server running state.
 * 根据服务器运行状态启用/禁用UI控件。
 * @param running True if server is running.
 */
void MainWindow::toggleControls(bool running)
{
    m_isServerRunning = running;

    apiAddressCombo->setEnabled(true);
    apiKeyEdit->setEnabled(true);
    tempSpin->setEnabled(true);
    modelCombo->setEnabled(true);
    contextSpin->setEnabled(true);

    portEdit->setEnabled(!running);
    threadSpin->setEnabled(!running);

    glossaryCombo->setEnabled(true);
    hudBtn->setEnabled(running);
    modernBtn->setEnabled(true);

    QString baseDescription = (m_currentLang == 0)
                                  ? "ℹ️ Select glossary files (.txt)\n\n💡 Tips:\n• Left-click: Browse files\n• Right-click: Open clean menu (Remove path / Clear history)"
                                  : "ℹ️ 选择术语表文件 (.txt)\n\n💡 提示：\n• 左键点击：浏览文件\n• 右键点击：打开清理菜单 (移除路径/清空历史)";

    if (running)
    {
        QString runningNotice = (m_currentLang == 0)
                                    ? "\n\n(ℹ️ Server is running: You can still click to change paths \n\n 💡 You can also left-click to select different glossaries or right-click to open the clean menu)"
                                    : "\n\n(ℹ️ 服务运行中：您仍可点击此处更改路径 \n\n 💡 您依旧可以左键选择不同的术语表 右键以打开清理菜单)";
        btnSelectGlossary->setToolTip(baseDescription + runningNotice);

        btnSelectGlossary->setStyleSheet(
            "QPushButton { "
            "  color: rgba(200, 200, 200, 120); "
            "  border: 1px solid rgba(150, 150, 150, 60); "
            "  background-color: transparent; "
            "} "
            "QPushButton:hover { "
            "  color: #ffffff; "
            "  background-color: #3d8af7; "
            "  border: 1px solid #3d8af7; "
            "}");
    }
    else
    {
        btnSelectGlossary->setToolTip(baseDescription);
        btnSelectGlossary->setStyleSheet("");
    }

    themeBtn->setEnabled(true);
    langBtn->setEnabled(true);
    clearCtxBtn->setEnabled(true);

    if (running)
    {
        startBtn->setText(STR_RELOAD[m_currentLang]);
        startBtn->setEnabled(true);
    }
    else
    {
        startBtn->setText(STR_START[m_currentLang]);
    }
    stopBtn->setEnabled(running);
}

/**
 * Handle start/reload button click.
 * 处理启动/重载按钮点击。
 */
void MainWindow::onStartClicked()
{
    AppConfig newCfg = getUiConfig();

    if (m_isServerRunning)
    {
        // Hot reload
        AppConfig oldCfg = server->getConfig();

        if (oldCfg.model_name != newCfg.model_name)
        {
            LogManager::instance().addLog(QString(LOG_MODEL_SWITCH[m_currentLang])
                                              .arg(oldCfg.model_name)
                                              .arg(newCfg.model_name));
        }
        else
        {
            LogManager::instance().addLog(QString(LOG_MODEL_SAME[m_currentLang]).arg(newCfg.model_name));
        }

        server->updateConfig(newCfg);

        LogManager::instance().addLog(LOG_RELOADED[m_currentLang]);

        QPropertyAnimation *anim = new QPropertyAnimation(startBtn, "windowOpacity");
        anim->setDuration(150);
        anim->setStartValue(0.5);
        anim->setEndValue(1.0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        LogManager::instance().addLog(LOG_AUTO_TESTING[m_currentLang]);
        onTestConfig();
    }
    else
    {
        // Start server
        server->updateConfig(newCfg);
        server->startServer();
    }
}

void MainWindow::onStopClicked()
{
    server->stopServer();
}

/**
 * Append a log message to the log area, with language adaptation.
 * 将日志消息追加到日志区域，并进行语言适配。
 * @param msg The log message.
 */
void MainWindow::onLogMessage(QString msg)
{
    if (!logArea)
        return;

    // Adapt messages for the current language
    if (m_currentLang == 0) // English
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
    else if (m_currentLang == 1) // Chinese
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

    const int MAX_LOG_LINES = 2000;
    if (logArea->document()->blockCount() > MAX_LOG_LINES)
    {
        QTextCursor cursor(logArea->document());
        cursor.movePosition(QTextCursor::Start);
        for (int i = 0; i < 500; ++i)
        {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
    }

    logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
}

void MainWindow::onSaveConfig()
{
    QString fileName = QFileDialog::getSaveFileName(this, STR_SAVE[m_currentLang], "config.ini", "Config Files (*.ini)");
    if (!fileName.isEmpty())
    {
        ConfigManager::saveConfig(getUiConfig(), fileName);
        server->injectLog(QString(LOG_CFG_SAVED[m_currentLang]) + fileName);
    }
}

void MainWindow::onLoadConfig()
{
    QString fileName = QFileDialog::getOpenFileName(this, STR_LOAD[m_currentLang], "", "Config Files (*.ini)");
    if (fileName.isEmpty())
        return;

    AppConfig cfg = ConfigManager::loadConfig(fileName);

    apiAddressCombo->setCurrentText(cfg.api_address);
    apiKeyEdit->setText(cfg.api_key);
    modelCombo->setCurrentText(cfg.model_name);
    portEdit->setText(QString::number(cfg.port));
    tempSpin->setValue(cfg.temperature);
    contextSpin->setValue(cfg.context_num);
    threadSpin->setValue(cfg.max_threads);

    if (!chkLockSysPrompt->isChecked())
    {
        systemPromptEdit->setText(cfg.system_prompt);
    }

    prePromptEdit->setText(cfg.pre_prompt);
    chkGlossary->setChecked(cfg.enable_glossary);

    if (!chkLockGlossary->isChecked())
    {
        if (!cfg.glossary_history.isEmpty())
        {
            glossaryCombo->clear();
            glossaryCombo->addItems(cfg.glossary_history);
        }

        if (!cfg.glossary_path.isEmpty())
        {
            glossaryCombo->setCurrentText(cfg.glossary_path);
        }
    }

    if (glossaryCombo->lineEdit())
    {
        glossaryCombo->lineEdit()->setCursorPosition(0);
    }

    server->injectLog(QString(LOG_CFG_LOADED[m_currentLang]) + fileName);
}

void MainWindow::onExportLog()
{
    QString fileName = "run_log.txt";
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif
        out << logArea->toPlainText();
        server->injectLog(LOG_EXPORTED[m_currentLang]);
    }
}

void MainWindow::onFetchModels()
{
    QString url = apiAddressCombo->currentText();
    if (url.isEmpty())
        return;
    if (url.endsWith("/"))
        url.chop(1);

    if (fetchLoadingOverlay)
    {
        fetchLoadingOverlay->setGeometry(fetchModelBtn->rect());
        fetchLoadingOverlay->raise();
        fetchLoadingOverlay->start();
    }

    server->injectLog(m_currentLang == 1 ? "🔍 正在获取模型列表..." : "🔍 Fetching models...");

    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    QNetworkRequest req(url + "/models");
    req.setTransferTimeout(10000);

    QString key = apiKeyEdit->text().split(',')[0].trimmed();
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());

    QNetworkReply *reply = mgr->get(req);
    connect(reply, &QNetworkReply::finished, [this, reply, mgr]()
            {
        if(fetchLoadingOverlay) fetchLoadingOverlay->stop();

        if(reply->error() == QNetworkReply::NoError) {
            try {
                auto jsonDoc = nlohmann::json::parse(reply->readAll().toStdString());
                modelCombo->clear();
                int count = 0;
                for(const auto& item : jsonDoc["data"]) {
                    modelCombo->addItem(QString::fromStdString(item["id"]));
                    count++;
                }
                server->injectLog(m_currentLang == 1 ? QString("✅ 获取成功！共发现 %1 个可用模型。").arg(count) 
                                                 : QString("✅ Success! Found %1 available models.").arg(count));
                modelCombo->setFocus();
            } catch(...) {
                server->injectLog("❌ " + QString(LOG_PARSE_ERR[m_currentLang]));
            }
        } else {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() == QNetworkReply::TimeoutError) statusCode = 999;
            server->injectLog("❌ " + getFriendlyErrorMessage(statusCode, m_currentLang));
        }
        reply->deleteLater();
        mgr->deleteLater(); });
}

void MainWindow::onTestConfig()
{
    server->injectLog(LOG_TEST_START[m_currentLang]);

    QStringList keys = apiKeyEdit->text().split(',', Qt::SkipEmptyParts);
    if (keys.isEmpty())
    {
        server->injectLog(LOG_NO_KEY[m_currentLang]);
        return;
    }

    if (testLoadingOverlay)
    {
        testLoadingOverlay->setGeometry(testBtn->rect());
        testLoadingOverlay->raise();
        testLoadingOverlay->start();
    }

    QString url = apiAddressCombo->currentText();
    if (url.endsWith("/"))
        url.chop(1);
    url += "/chat/completions";
    QString model = modelCombo->currentText();

    auto finishedCount = std::make_shared<int>(0);
    auto successCount = std::make_shared<int>(0);
    int total = keys.size();

    for (int i = 0; i < total; ++i)
    {
        QString key = keys[i].trimmed();
        QString keyMasked = (key.length() > 8) ? ("..." + key.right(8)) : key;

        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        QNetworkRequest req(url);
        req.setTransferTimeout(10000);

        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());

        nlohmann::json j;
        j["model"] = model.toStdString();
        j["messages"] = nlohmann::json::array({{{"role", "user"}, {"content", "Hi"}}});
        j["max_tokens"] = 5;

        QNetworkReply *reply = mgr->post(req, QByteArray::fromStdString(j.dump()));

        connect(reply, &QNetworkReply::finished, [this, reply, mgr, keyMasked, i, finishedCount, successCount, total]()
                {
            (*finishedCount)++;
            bool isOk = (reply->error() == QNetworkReply::NoError);
            if(isOk) (*successCount)++;

            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() == QNetworkReply::TimeoutError) statusCode = 999;

            QString icon = isOk ? "✅" : "❌";
            QString status = isOk ? (m_currentLang == 1 ? "测试通过" : "PASS") 
                                 : getFriendlyErrorMessage(statusCode, m_currentLang);
            
            server->injectLog(QString("%1 Key-%2 (%3): %4")
                            .arg(icon).arg(i + 1).arg(keyMasked).arg(status));

            if (*finishedCount == total) {
                if(testLoadingOverlay) testLoadingOverlay->stop();

                server->injectLog("----------------------------------");
                QString summary = (m_currentLang == 1) 
                    ? QString("📊 测试结束！成功: %1, 失败: %2").arg(*successCount).arg(total - *successCount)
                    : QString("📊 Finished! Success: %1, Failed: %2").arg(*successCount).arg(total - *successCount);
                server->injectLog("<b>" + summary + "</b>");
            }

            reply->deleteLater();
            mgr->deleteLater(); });
    }
}

/**
 * Update the token display label.
 * 更新令牌显示标签。
 * @param total Total tokens used.
 * @param prompt Prompt tokens.
 * @param completion Completion tokens.
 */
void MainWindow::updateTokenDisplay(long long total, long long prompt, long long completion)
{
    lblTokens->setProperty("total", total);
    lblTokens->setProperty("prompt", prompt);
    lblTokens->setProperty("completion", completion);

    lblTokens->setText(QString("%1 %2").arg(STR_TOKENS[m_currentLang]).arg(total));

    QString strPrompt = (m_currentLang == 1) ? "输入 (Prompt):" : "Input (Prompt):";
    QString strCompletion = (m_currentLang == 1) ? "输出 (Completion):" : "Output (Completion):";

    QString fullTip = QString("<b>%1</b><br><br>%2 %3<br>%4 %5")
                          .arg(TIP_TOKENS[m_currentLang])
                          .arg(strPrompt)
                          .arg(prompt)
                          .arg(strCompletion)
                          .arg(completion);

    lblTokens->setToolTip(fullTip);

     lblTokens->adjustSize();
    if (logGroup->width() > 0)
    {
        int tokensX = logGroup->width() - lblTokens->width() - 10;
        lblTokens->move(tokensX, 0);
    }
}

void MainWindow::onClearContext()
{
    server->clearAllContexts();
}

/**
 * Switch to HUD (heads-up display) mode.
 * 切换到HUD（平视显示）模式。
 */
void MainWindow::switchToHud()
{
    if (!server)
        return;

    QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(300);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);

    connect(anim, &QPropertyAnimation::finished, [this]()
            {
        this->hide();
        m_hudWindow->move(this->geometry().topRight() - QPoint(280, -20)); 
        m_hudWindow->show();
        m_hudWindow->setStatus(false);
        m_hudWindow->updateTokens(m_tokenManager->getTotal()); });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::restoreFromHud()
{
    m_hudWindow->hide();
    this->setWindowOpacity(0.0);
    this->show();

    QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onServerWorkStarted()
{
    if (m_hudWindow && m_hudWindow->isVisible())
    {
        m_hudWindow->setStatus(true);
    }
}

void MainWindow::onServerWorkFinished(bool success)
{
    if (m_hudWindow && m_hudWindow->isVisible())
    {
        m_hudWindow->setStatus(false, !success);
    }
}

/**
 * Handle context menu request for glossary-related widgets.
 * 处理术语表相关控件的上下文菜单请求。
 * @param pos Position where the menu was requested.
 */
void MainWindow::onGlossaryContextMenu(const QPoint &pos)
{
    int lang = m_currentLang;

    QWidget *senderWidget = qobject_cast<QWidget *>(sender());
    if (!senderWidget)
        return;

    QMenu menu(this);

    QAction *removeAction = menu.addAction(STR_REMOVE_PATH[lang]);
    QAction *clearAction = menu.addAction(STR_CLEAR_HISTORY[lang]);

    if (glossaryCombo->currentText().isEmpty())
    {
        removeAction->setEnabled(false);
    }

    QPoint globalPos = senderWidget->mapToGlobal(pos);

    QAction *selectedAction = menu.exec(globalPos);

    if (selectedAction == removeAction)
    {
        int index = glossaryCombo->currentIndex();
        if (index != -1)
        {
            glossaryCombo->removeItem(index);
            server->injectLog(lang == 1 ? "🧽 已从历史记录中移除该路径。" : "🧽 Removed current path from history.");
        }

        if (glossaryCombo->count() == 0 || index == -1)
        {
            glossaryCombo->setEditText("");
        }
    }
    else if (selectedAction == clearAction)
    {
        glossaryCombo->clear();
        glossaryCombo->setEditText("");
        server->injectLog(lang == 1 ? "🗑️ 术语表历史记录已清空。" : "🗑️ Glossary history cleared.");
    }
}

/**
 * Return a user-friendly error message based on HTTP status code.
 * 根据HTTP状态码返回用户友好的错误信息。
 * @param code HTTP status code.
 * @param lang Language (0=English, 1=Chinese).
 * @return Friendly error string.
 */
QString MainWindow::getFriendlyErrorMessage(int code, int lang)
{
    if (lang == 1)
    {
        switch (code)
        {
        case 0:
            return "网络连接超时: 无法连接到服务器，请检查 API 地址或网络代理。";
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
    else
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
 * Handle glossary selection change.
 * 处理术语表选择变化。
 */
void MainWindow::onGlossaryChanged()
{
    AppConfig currentCfg = getUiConfig();
    if (m_isServerRunning && server)
    {
        server->updateConfig(currentCfg);
        QString msg = (m_currentLang == 0)
                          ? QString("🔄 Glossary switched to: %1").arg(currentCfg.glossary_path)
                          : QString("🔄 术语表已切换至: %1").arg(currentCfg.glossary_path);

        LogManager::instance().addLog(msg);
    }
}

/**
 * Emit signal to switch to modern mode.
 * 发射信号以切换到流光模式。
 */
void MainWindow::onSwitchToModern()
{
    if (m_glossaryEditor && m_glossaryEditor->isVisible())
    {
        m_glossaryEditor->hide();
    }

    emit requestModernView();
}

// ==========================================
// Glossary Editor logic (enhanced smooth appearance)
// 术语表编辑器逻辑（增强平滑浮现）
// ==========================================
void MainWindow::openGlossaryEditor()
{
    if (m_glossaryEditor && m_glossaryEditor->isVisible())
    {
        m_glossaryCancelBtn->click();
        return;
    }

    QString path = glossaryCombo->currentText();
    if (path.isEmpty() || !QFile::exists(path))
    {
        QMessageBox::warning(this, STR_GLOS_WARN_TITLE[m_currentLang], STR_GLOS_WARN_MSG[m_currentLang]);
        return;
    }
    m_currentEditingPath = path;

    if (!m_glossaryEditor)
    {
        m_glossaryEditor = new QDialog(this);
        m_glossaryEditor->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        m_glossaryEditor->setFixedSize(320, 707);

        auto *dlgLayout = new QVBoxLayout(m_glossaryEditor);
        m_glossaryTextEdit = new QTextEdit(m_glossaryEditor);

        GlossaryHighlighter *highlighter = new GlossaryHighlighter(m_glossaryTextEdit->document());
        highlighter->setTheme(m_isDarkTheme);

        auto *bottomLayout = new QHBoxLayout();
        m_glossarySaveBtn = new QPushButton();
        m_glossaryCancelBtn = new QPushButton();
        bottomLayout->addStretch();
        bottomLayout->addWidget(m_glossarySaveBtn);
        bottomLayout->addWidget(m_glossaryCancelBtn);
        dlgLayout->addWidget(m_glossaryTextEdit);
        dlgLayout->addLayout(bottomLayout);

        connect(m_glossarySaveBtn, &QPushButton::clicked, this, &MainWindow::saveGlossaryEditor);

        connect(m_glossaryCancelBtn, &QPushButton::clicked, this, [this]()
                {
            QPropertyAnimation *fadeOut = new QPropertyAnimation(m_glossaryEditor, "windowOpacity");
            fadeOut->setDuration(250);
            fadeOut->setStartValue(1.0);
            fadeOut->setEndValue(0.0);
            connect(fadeOut, &QPropertyAnimation::finished, m_glossaryEditor, &QDialog::hide);
            fadeOut->start(QAbstractAnimation::DeleteWhenStopped); });
    }

    QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    int spacing = 1;
    int targetY = this->geometry().y() + (this->height() - 707) / 2;

    bool canShowLeft = (this->geometry().x() - 320 - spacing >= screen.left());
    int targetX = canShowLeft ? (this->geometry().left() - 320 - spacing) : (this->geometry().right() + spacing);

    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_glossaryTextEdit->setPlainText(f.readAll());
        f.close();
    }
    updateUIText();
    applyTheme(m_isDarkTheme);

    m_glossaryEditor->setGeometry(targetX, targetY, 320, 707);
    m_glossaryEditor->setWindowOpacity(0.0);
    m_glossaryEditor->show();

    QPropertyAnimation *fadeIn = new QPropertyAnimation(m_glossaryEditor, "windowOpacity");
    fadeIn->setDuration(400);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::saveGlossaryEditor()
{
    if (m_currentEditingPath.isEmpty())
        return;

    QFile outFile(m_currentEditingPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        outFile.write(m_glossaryTextEdit->toPlainText().toUtf8());
        outFile.close();
        LogManager::instance().addLog(LOG_GLOS_SAVED[m_currentLang]);

        if (m_isServerRunning && server)
        {
            server->updateConfig(getUiConfig());
        }
    }
}