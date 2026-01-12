/**
 * MainWindow.cpp - Moil的XUnity大模型翻译GUI主窗口实现
 * MainWindow.cpp - Main window implementation for Moil's XUnity LLM Translator GUI
 */

#include "MainWindow.h"
#include "json.hpp" 
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
#include <QPixmap> // 用于截图 / Used for screenshots
#include <QMenu>


// ==========================================
// 🌍 多语言字典定义 (UI 文本)
// 🌍 Multi-language Dictionary Definitions (UI Text)
// ==========================================
// 索引 0: English, 索引 1: 中文
const char* STR_TITLE[] = {"Moil的XUnity大模型翻译GUI", "Moil's XUnity LLM Translator GUI"};
const char* STR_API_CFG[] = {"API 配置", "API Configuration"};
const char* STR_LOG_AREA[] = {"运行日志", "Runtime Logs"};
const char* STR_API_ADDR[] = {"API 地址:", "API Address:"};
const char* STR_API_KEY[] = {"API 密钥:", "API Key:"};
const char* STR_MODEL[] = {"模型名称:", "Model Name:"};
const char* STR_FETCH[] = {"获取列表", "Fetch Models"};
const char* STR_PORT[] = {"端口:", "Port:"};
const char* STR_THREAD[] = {"线程:", "Threads:"};
const char* STR_TEMP[] = {"温度:", "Temp:"};
const char* STR_CTX[] = {"上下文:", "Context:"};
const char* STR_SYS_PROMPT[] = {"系统提示:", "System Prompt:"};
const char* STR_PRE_PROMPT[] = {"前置文本:", "Pre-Prompt:"};
const char* STR_START[] = {"启动服务", "Start Server"};
const char* STR_STOP[] = {"停止服务", "Stop Server"};
const char* STR_TEST[] = {"测试配置", "Test Config"};
const char* STR_LOAD[] = {"读取配置", "Load Config"};
const char* STR_SAVE[] = {"保存配置", "Save Config"};
const char* STR_EXPORT[] = {"导出日志", "Export Log"};
const char* STR_THEME_LIGHT[] = {"切换亮色", "Light Mode"};
const char* STR_THEME_DARK[] = {"切换暗色", "Dark Mode"};
const char* STR_LANG_BTN[] = {"English", "中文"}; 
const char* STR_GLOSSARY[] = {"术语表:", "Glossary:"}; 
const char* STR_CHK_GLOSSARY[] = {"启用自进化 (实验性)", "Enable Self-Evolution (Exp)"};
const char* STR_CLEAR_LOG[] = {"清空日志", "Clear Log"};
const char* STR_TOKENS[] = {"消耗:", "Tokens:"};
const char* TIP_TOKENS[] = {"本次运行总消耗 (输入+输出)", "Total Usage (Prompt + Completion)"};
// ==========================================
// 📝 多语言字典定义 (日志文本)
// 📝 Multi-language Dictionary Definitions (Log Text)
// ==========================================
const char* LOG_TEST_START[] = {"=== 开始测试所有 API Key ===", "=== Testing API Keys ==="};
const char* LOG_NO_KEY[] = {"❌ 未找到 API Key", "❌ No API Key"};
const char* LOG_PASS[] = {"测试通过", "Pass"};
const char* LOG_FAIL[] = {"失败", "Fail"};
const char* LOG_FETCH_SUCCESS[] = {"模型列表获取成功", "Fetch Models Success"};
const char* LOG_FETCH_FAIL[] = {"获取失败: ", "Fetch Failed: "};
const char* LOG_PARSE_ERR[] = {"解析错误", "Parse Error"};
const char* LOG_CFG_SAVED[] = {"配置已保存: ", "Config Saved: "};
const char* LOG_CFG_LOADED[] = {"配置已加载: ", "Config Loaded: "};
const char* LOG_EXPORTED[] = {"日志已导出到 run_log.txt", "Log Exported to run_log.txt"};

// --- Tooltips / 工具提示 ---
const char* TIP_PORT[] = {
    "本地监听端口\n请确保 XUnity 配置文件 Endpoint 设置为 http://localhost:端口号",
    "Local Listening Port\nEnsure XUnity Endpoint is set to http://localhost:port"
};
const char* TIP_THREAD[] = {
    "并发线程数 (Max Threads)\n建议值: 取决于你电脑的线程数\n注意: 一定程度上可以加快翻译工作，过多会导致系统卡顿",
    "Concurrent Threads\nRecommended: Depends on your CPU\nNote: Can speed up translation to some extent, too many may cause system lag"
};
const char* TIP_TEMP[] = {
    "采样温度 (Temperature)\n0.0-0.3: 严谨\n0.7-1.0: 标准\n>1.0: 随机/创造性",
    "Sampling Temperature\n0.0-0.3: Strict\n0.7-1.0: Standard\n>1.0: Creative/Random"
};
const char* TIP_CTX[] = {
    "上下文记忆 (Context)\n携带的历史对话轮数。\n注意：上下文越多，消耗 Token 越多。",
    "Context Memory\nNumber of history turns to carry.\nNote: More context consumes more tokens."
};
const char* TIP_GLOSSARY[] = {
    "选择 XUnity 的 _Substitutions.txt 文件。\nLLM 将自动参考并补充该文件。",
    "Select XUnity's _Substitutions.txt.\nLLM will reference and append to it."
};

/**
 * 构造函数：初始化主窗口
 * Constructor: Initialize the main window
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // 1. 基础变量初始化 (推荐使用初始化列表，但这里放在这里也行)
    // 1. Basic variable initialization (preferably using initializer list, but here is okay)
    m_isClosing = false;
    m_isDarkTheme = true;
    m_currentLang = 0; 
    resize(650, 800); 

    // ============================================================
    // 第一阶段：创建核心对象 (The Logic Layer)
    // Phase 1: Create Core Objects (The Logic Layer)
    // ============================================================
    // 必须先创建它们，因为后续的 connect 依赖它们
    // They must be created first because subsequent connect statements depend on them
    m_tokenManager = new TokenManager(this);
    server = new TranslationServer(this);

    // ============================================================
    // 第二阶段：构建 UI (The View Layer)
    // Phase 2: Build UI (The View Layer)
    // ============================================================
    // ⚠️ 关键点：setupUi 会执行 new QLabel 等操作。
    // ⚠️ Key Point: setupUi will execute new QLabel, etc.
    // 在这行代码执行完之前，绝对不能调用 updateUIText 或访问 lblTokens。
    // Before this line completes, do not call updateUIText or access lblTokens.
    setupUi(); 

    // ============================================================
    // 第三阶段：连接信号槽 (The Controller Layer)
    // Phase 3: Connect Signals and Slots (The Controller Layer)
    // ============================================================
    // 此时 Server(数据源) 和 lblTokens(显示目标) 都已经存在了，连接是绝对安全的。
    // At this point, both Server (data source) and lblTokens (display target) exist, connection is absolutely safe.
    
    // 日志 / Logging
    connect(server, &TranslationServer::logMessage, this, &MainWindow::onLogMessage);
    
    // 数据流: Server -> TokenManager
    // Data flow: Server -> TokenManager
    connect(server, &TranslationServer::tokenUsageReceived, m_tokenManager, &TokenManager::addUsage);
    
    // 显示流: TokenManager -> UI
    // Display flow: TokenManager -> UI
    connect(m_tokenManager, &TokenManager::tokensUpdated, this, &MainWindow::updateTokenDisplay);

    // ============================================================
    // 第四阶段：初始化状态 (State Initialization)
    // Phase 4: Initialize State
    // ============================================================
    // 此时所有指针都已分配内存，直接调用，不需要 if 检查。
    // At this point, all pointers have allocated memory, call directly without if checks.
    
    loadConfigToUi(); // 加载配置到输入框 / Load config to input fields
    updateUIText();   // 设置 Label 的文字 / Set label texts
    applyTheme(true); // 设置颜色 / Set colors

    // ============================================================
    // 第五阶段：启动特效
    // Phase 5: Startup Effects
    // ============================================================
    setWindowOpacity(0.0);
    fadeAnim = new QPropertyAnimation(this, "windowOpacity");
    fadeAnim->setDuration(500);
    fadeAnim->setStartValue(0.0);
    fadeAnim->setEndValue(1.0);
    fadeAnim->start();
}

/**
 * 析构函数：停止服务器
 * Destructor: Stop the server
 */
MainWindow::~MainWindow() {
    server->stopServer();
}

/**
 * 窗口关闭事件处理：执行退出动画并保存配置
 * Window close event handling: Execute exit animation and save config
 */
void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_isClosing) {
        event->accept();
        return;
    }
    // 关闭前自动保存配置
    // Auto-save config before closing
    ConfigManager::saveConfig(getUiConfig(), "config.ini");
    event->ignore(); 
    m_isClosing = true;
    // 执行淡出动画后再退出
    // Execute fade-out animation before quitting
    fadeOutAndClose();
}

/**
 * 淡出动画并关闭应用程序
 * Fade out animation and close the application
 */
void MainWindow::fadeOutAndClose() {
    fadeAnim->setDirection(QAbstractAnimation::Backward);
    connect(fadeAnim, &QPropertyAnimation::finished, this, &QMainWindow::close); 
    connect(fadeAnim, &QPropertyAnimation::finished, qApp, &QApplication::quit);
    fadeAnim->start();
}

// ==========================================
// ✨ 平滑切换核心逻辑 (Smooth Transition)
// ✨ Smooth Transition Core Logic
// ==========================================
void MainWindow::smoothSwitch(std::function<void()> changeLogic) {
    // 1. 截图：捕获当前窗口的样子
    // 1. Screenshot: Capture the current appearance of the window
    QPixmap pixmap = this->grab();
    
    // 2. 创建遮罩层：一个覆盖全窗口的 Label，显示刚才的截图
    // 2. Create overlay: A Label covering the full window, displaying the screenshot
    QLabel* overlay = new QLabel(this);
    overlay->setPixmap(pixmap);
    overlay->setGeometry(0, 0, this->width(), this->height());
    overlay->show(); // 遮住一切 / Cover everything

    // 3. 在遮罩层底下执行切换逻辑 (用户看不见变化)
    // 3. Execute switch logic under the overlay (user sees no change yet)
    changeLogic();

    // 4. 创建透明度动画：让遮罩层慢慢消失，显露底下的新界面
    // 4. Create opacity animation: Fade out overlay to reveal the new UI underneath
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(overlay);
    overlay->setGraphicsEffect(effect);
    
    QPropertyAnimation* anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(350); // 350ms 的过渡时间 / 350ms transition duration
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->setEasingCurve(QEasingCurve::OutQuad); // 缓动曲线，让动画更自然 / Easing curve for natural animation

    // 5. 动画结束后销毁遮罩层
    // 5. Destroy the overlay after animation finishes
    connect(anim, &QPropertyAnimation::finished, overlay, &QLabel::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

/**
 * 切换界面语言
 * Toggle the UI language
 */
void MainWindow::toggleLanguage() {
    // 使用平滑切换 / Use smooth switching
    smoothSwitch([this](){
        m_currentLang = (m_currentLang == 0) ? 1 : 0;
        updateUIText();
        if(themeBtn) themeBtn->setText(m_isDarkTheme ? STR_THEME_LIGHT[m_currentLang] : STR_THEME_DARK[m_currentLang]);
        // 实时更新服务器配置（为了更新 pre_prompt 等）
        // Update server config in real-time (to update pre_prompt etc.)
        server->updateConfig(getUiConfig());
    });
}

/**
 * 切换主题（亮色/暗色）
 * Toggle theme (Light/Dark)
 */
void MainWindow::toggleTheme() {
    // 使用平滑切换 / Use smooth switching
    smoothSwitch([this](){
        applyTheme(!m_isDarkTheme);
    });
}

/**
 * 选择术语表文件
 * Select glossary file
 */
void MainWindow::onSelectGlossary() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select File", "", "Text Files (*.txt);;All Files (*.*)");
    if (!fileName.isEmpty()) {
        glossaryPathEdit->setText(fileName);
    }
}

/**
 * 更新所有UI控件的文本（根据当前语言）
 * Update text of all UI controls (based on current language)
 */
void MainWindow::updateUIText() {
    int i = m_currentLang;
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
    lblPrePrompt->setText(STR_PRE_PROMPT[i]);
    
    lblGlossary->setText(STR_GLOSSARY[i]);
    chkGlossary->setText(STR_CHK_GLOSSARY[i]);
    
    startBtn->setText(STR_START[i]);
    stopBtn->setText(STR_STOP[i]);
    testBtn->setText(STR_TEST[i]);
    loadBtn->setText(STR_LOAD[i]);
    saveBtn->setText(STR_SAVE[i]);
    exportBtn->setText(STR_EXPORT[i]);
    langBtn->setText(STR_LANG_BTN[i]);
    
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
    glossaryPathEdit->setToolTip(TIP_GLOSSARY[i]);
    btnSelectGlossary->setToolTip(TIP_GLOSSARY[i]);

    // ✅ 自信的代码：直接调用，无需判空
    // ✅ Confident code: Call directly without null checks
    // 因为根据构造函数的顺序，运行到这里时，lblTokens 必然活着
    // Because according to the constructor order, lblTokens must be alive when running here
    lblTokens->setText(QString("%1 %2").arg(STR_TOKENS[i]).arg(m_tokenManager->getTotal()));
    lblTokens->setToolTip(TIP_TOKENS[i]);
}

/**
 * 应用主题（深色/浅色）
 * Apply theme (Dark/Light)
 */
void MainWindow::applyTheme(bool isDark) {
    // Use Fusion style for consistent cross-platform look
    // 使用 Fusion 风格以获得一致的跨平台外观
    qApp->setStyle(QStyleFactory::create("Fusion"));
    
    // Critical: Clear stylesheet to prevent QSS residue
    // 关键：清空样式表，防止之前的 QSS 残留影响原生渲染
    qApp->setStyleSheet(""); 

    QPalette p;
    if (isDark) {
        // 🌑 Pure Dark Theme / 纯黑深色主题
        p.setColor(QPalette::Window, QColor(18, 18, 18)); 
        p.setColor(QPalette::WindowText, Qt::white);      
        p.setColor(QPalette::Text, Qt::white);            
        p.setColor(QPalette::ButtonText, Qt::white);      
        p.setColor(QPalette::Base, QColor(30, 30, 30));   
        p.setColor(QPalette::AlternateBase, QColor(18, 18, 18));
        p.setColor(QPalette::Button, QColor(45, 45, 45)); 
        p.setColor(QPalette::ToolTipBase, Qt::white);
        p.setColor(QPalette::ToolTipText, Qt::black);
        p.setColor(QPalette::Link, QColor(64, 156, 255)); 
        p.setColor(QPalette::Highlight, QColor(64, 156, 255)); 
        p.setColor(QPalette::HighlightedText, Qt::black);      
        p.setColor(QPalette::PlaceholderText, QColor(150, 150, 150));

        if(themeBtn) themeBtn->setText(STR_THEME_LIGHT[m_currentLang]);

    } else {
        // ☀️ Standard Light Theme / 标准亮色主题
        p.setColor(QPalette::Window, QColor(240, 240, 240));
        p.setColor(QPalette::WindowText, Qt::black); 
        p.setColor(QPalette::Text, Qt::black);       
        p.setColor(QPalette::ButtonText, Qt::black); 
        p.setColor(QPalette::Base, Qt::white);
        p.setColor(QPalette::AlternateBase, QColor(233, 233, 233));
        p.setColor(QPalette::Button, QColor(225, 225, 225));
        p.setColor(QPalette::ToolTipBase, Qt::black);
        p.setColor(QPalette::ToolTipText, Qt::white);
        p.setColor(QPalette::Link, QColor(0, 0, 255));
        p.setColor(QPalette::Highlight, QColor(0, 120, 215)); 
        p.setColor(QPalette::HighlightedText, Qt::white);
        p.setColor(QPalette::PlaceholderText, QColor(100, 100, 100));

        if(themeBtn) themeBtn->setText(STR_THEME_DARK[m_currentLang]);
    }
    
    qApp->setPalette(p);
    m_isDarkTheme = isDark;
}

/**
 * 创建UI布局和控件
 * Create UI layout and controls
 */
void MainWindow::setupUi() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(6); 
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // === Configuration Group ===
    cfgGroup = new QGroupBox(this); 
    QGridLayout *grid = new QGridLayout(cfgGroup);
    grid->setColumnStretch(1, 1);
    grid->setVerticalSpacing(8); 

    auto createLabel = [this](QLabel*& memberPtr) {
        memberPtr = new QLabel(this);
        memberPtr->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return memberPtr;
    };

    // Row 0: API Address
    apiAddressEdit = new QLineEdit(this);
    grid->addWidget(createLabel(lblApiAddr), 0, 0);
    grid->addWidget(apiAddressEdit, 0, 1);

    // Row 1: API Key
    apiKeyEdit = new QLineEdit(this);
    grid->addWidget(createLabel(lblApiKey), 1, 0);
    grid->addWidget(apiKeyEdit, 1, 1);

    // Row 2: Model Selection (Combo + Fetch Button)
    QWidget *modelContainer = new QWidget(this);
    QHBoxLayout *modelLayout = new QHBoxLayout(modelContainer);
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelCombo = new QComboBox(this);
    modelCombo->setEditable(true); // 允许手动输入模型名 / Allow manual entry of model name
    modelCombo->setMinimumHeight(28); 
    fetchModelBtn = new QPushButton(this); 
    connect(fetchModelBtn, &QPushButton::clicked, this, &MainWindow::onFetchModels);
    modelLayout->addWidget(modelCombo, 1);
    modelLayout->addWidget(fetchModelBtn);
    grid->addWidget(createLabel(lblModel), 2, 0);
    grid->addWidget(modelContainer, 2, 1);

     // === Row 3: Parameters (重点修改区域) ===
    // === Row 3: Parameters (Key Modification Area) ===
    QWidget *paramContainer = new QWidget(this);
    QHBoxLayout *paramLayout = new QHBoxLayout(paramContainer);
    paramLayout->setContentsMargins(0, 0, 0, 0);
    
    // 初始化各个控件 / Initialize each control
    lblPort = new QLabel(this);
    portEdit = new QLineEdit(this);
    portEdit->setFixedWidth(50);
    portEdit->setAlignment(Qt::AlignCenter);
    
    lblThread = new QLabel(this);
    threadSpin = new QSpinBox(this);
    threadSpin->setRange(1, 200);
    threadSpin->setFixedWidth(50);
    threadSpin->setAlignment(Qt::AlignCenter);

    lblTemp = new QLabel(this);
    tempSpin = new QDoubleSpinBox(this);
    tempSpin->setRange(0, 2);
    tempSpin->setSingleStep(0.1);
    tempSpin->setFixedWidth(50);
    tempSpin->setAlignment(Qt::AlignCenter);

    lblCtx = new QLabel(this);
    contextSpin = new QSpinBox(this);
    contextSpin->setRange(0, 20);
    contextSpin->setFixedWidth(50);
    contextSpin->setAlignment(Qt::AlignCenter);

    // ⚠️ 关键：在这里创建 lblTokens
    // ⚠️ Key: Create lblTokens here
    lblTokens = new QLabel(this);
    lblTokens->setStyleSheet("color: #DAA520; font-weight: bold;"); 

    // 添加到布局 / Add to layout
    paramLayout->addWidget(lblPort);
    paramLayout->addWidget(portEdit);
    paramLayout->addSpacing(15);
    paramLayout->addWidget(lblThread);
    paramLayout->addWidget(threadSpin);
    paramLayout->addSpacing(15);
    paramLayout->addWidget(lblTemp);
    paramLayout->addWidget(tempSpin);
    paramLayout->addSpacing(15);
    paramLayout->addWidget(lblCtx);
    paramLayout->addWidget(contextSpin);
    
    // 添加 Tokens 消耗器 / Add Tokens consumption display
    paramLayout->addSpacing(15);
    paramLayout->addWidget(lblTokens);

    paramLayout->addStretch(); // 弹簧，保持左对齐 / Spring to keep left alignment

    grid->addWidget(paramContainer, 3, 0, 1, 2);


    // Row 4: System Prompt
    systemPromptEdit = new QTextEdit(this);
    systemPromptEdit->setMinimumHeight(100); 
    lblSysPrompt = new QLabel(this);
    lblSysPrompt->setAlignment(Qt::AlignRight | Qt::AlignTop);
    grid->addWidget(lblSysPrompt, 4, 0);
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
    glossaryPathEdit = new QLineEdit(this);
    glossaryPathEdit->setPlaceholderText("_Substitutions.txt Path");
    btnSelectGlossary = new QPushButton("...", this);
    btnSelectGlossary->setFixedWidth(30);
    connect(btnSelectGlossary, &QPushButton::clicked, this, &MainWindow::onSelectGlossary);
    glossaryLayout->addWidget(chkGlossary);
    glossaryLayout->addWidget(glossaryPathEdit);
    glossaryLayout->addWidget(btnSelectGlossary);
    grid->addWidget(createLabel(lblGlossary), 6, 0);
    grid->addWidget(glossaryContainer, 6, 1);
    
    mainLayout->addWidget(cfgGroup);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    auto createBtn = [this](QPushButton*& btnPtr) { btnPtr = new QPushButton(this); btnPtr->setMinimumHeight(32); return btnPtr; };
    btnLayout->addWidget(createBtn(startBtn));
    btnLayout->addWidget(createBtn(stopBtn));
    stopBtn->setEnabled(false);
    btnLayout->addWidget(createBtn(testBtn));
    btnLayout->addWidget(createBtn(loadBtn));
    btnLayout->addWidget(createBtn(saveBtn));
    btnLayout->addWidget(createBtn(exportBtn));
    btnLayout->addWidget(createBtn(langBtn)); 
    btnLayout->addWidget(createBtn(themeBtn));
    
    connect(themeBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(langBtn, &QPushButton::clicked, this, &MainWindow::toggleLanguage); 
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(testBtn, &QPushButton::clicked, this, &MainWindow::onTestConfig);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadConfig);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveConfig);
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportLog);
    mainLayout->addLayout(btnLayout);

    // Log Area
    logGroup = new QGroupBox(this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logArea = new QTextEdit(this);
    logArea->setReadOnly(true);
    logArea->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(logArea, &QTextEdit::customContextMenuRequested, this, &MainWindow::onLogContextMenu);
    logLayout->addWidget(logArea);
    mainLayout->addWidget(logGroup);
}

// ==========================================
// ✨ 新增：右键菜单实现函数
// ✨ New: Context Menu Implementation
// ==========================================
void MainWindow::onLogContextMenu(const QPoint &pos) {
    // 1. 获取 QTextEdit 默认的标准菜单 (包含复制、全选等功能)
    // 1. Get the standard menu of QTextEdit (includes Copy, Select All, etc.)
    // 这样我们就不需要自己重新写复制功能了 / So we don't need to rewrite copy function
    QMenu *menu = logArea->createStandardContextMenu();
    
    // 2. 添加分隔线
    // 2. Add separator
    menu->addSeparator();
    
    // 3. 添加“清空日志”动作
    // 3. Add "Clear Log" action
    QAction *clearAction = menu->addAction(STR_CLEAR_LOG[m_currentLang]);
    
    // 4. 连接动作到 logArea 的 clear 槽函数
    // 4. Connect action to logArea's clear slot
    connect(clearAction, &QAction::triggered, logArea, &QTextEdit::clear);
    
    // 5. 在鼠标位置显示菜单
    // 5. Show menu at mouse position
    menu->exec(logArea->mapToGlobal(pos));
    
    // 6. 清理内存
    // 6. Clean up memory
    delete menu;
}

/**
 * 从配置管理器加载配置到UI控件
 * Load configuration from ConfigManager to UI controls
 */
void MainWindow::loadConfigToUi() {
    AppConfig cfg = ConfigManager::loadConfig();
    apiAddressEdit->setText(cfg.api_address);
    apiKeyEdit->setText(cfg.api_key);
    modelCombo->setCurrentText(cfg.model_name);
    portEdit->setText(QString::number(cfg.port));
    tempSpin->setValue(cfg.temperature);
    contextSpin->setValue(cfg.context_num);
    threadSpin->setValue(cfg.max_threads);
    systemPromptEdit->setText(cfg.system_prompt);
    prePromptEdit->setText(cfg.pre_prompt);
    
    chkGlossary->setChecked(cfg.enable_glossary);
    glossaryPathEdit->setText(cfg.glossary_path);
    
    m_currentLang = cfg.language; 
}

/**
 * 从UI控件获取当前配置
 * Get current configuration from UI controls
 */
AppConfig MainWindow::getUiConfig() {
    AppConfig cfg;
    cfg.api_address = apiAddressEdit->text();
    cfg.api_key = apiKeyEdit->text();
    cfg.model_name = modelCombo->currentText();
    cfg.port = portEdit->text().toInt();
    cfg.temperature = tempSpin->value();
    cfg.context_num = contextSpin->value();
    cfg.max_threads = threadSpin->value();
    cfg.system_prompt = systemPromptEdit->toPlainText();
    cfg.pre_prompt = prePromptEdit->text();
    
    cfg.enable_glossary = chkGlossary->isChecked();
    cfg.glossary_path = glossaryPathEdit->text();
    
    cfg.language = m_currentLang; 
    return cfg;
}

/**
 * 根据服务器运行状态切换控件可用性
 * Toggle control availability based on server running state
 */
void MainWindow::toggleControls(bool running) {
    startBtn->setEnabled(!running);
    stopBtn->setEnabled(running);
    // 运行时禁用配置修改 / Disable config modification while running
    apiAddressEdit->setEnabled(!running);
    apiKeyEdit->setEnabled(!running);
    portEdit->setEnabled(!running);
    threadSpin->setEnabled(!running);
    chkGlossary->setEnabled(!running);
    glossaryPathEdit->setEnabled(!running);
    btnSelectGlossary->setEnabled(!running);
}

/**
 * 启动翻译服务器
 * Start the translation server
 */
void MainWindow::onStartClicked() {
    AppConfig cfg = getUiConfig();
    server->updateConfig(cfg);
    server->startServer();
    toggleControls(true);
}

/**
 * 停止翻译服务器
 * Stop the translation server
 */
void MainWindow::onStopClicked() {
    server->stopServer();
    toggleControls(false);
}

/**
 * 处理日志消息并显示在日志区域
 * Process log message and display in log area
 */
void MainWindow::onLogMessage(QString msg) {
    logArea->append(msg);
}

/**
 * 保存当前配置到文件
 * Save current configuration to file
 */
void MainWindow::onSaveConfig() {
    QString fileName = QFileDialog::getSaveFileName(this, STR_SAVE[m_currentLang], "config.ini", "Config Files (*.ini)");
    if (!fileName.isEmpty()) {
        ConfigManager::saveConfig(getUiConfig(), fileName);
        logArea->append(QString(LOG_CFG_SAVED[m_currentLang]) + fileName);
    }
}

/**
 * 从文件加载配置并更新UI
 * Load configuration from file and update UI
 */
void MainWindow::onLoadConfig() {
    QString fileName = QFileDialog::getOpenFileName(this, STR_LOAD[m_currentLang], "", "Config Files (*.ini)");
    if (!fileName.isEmpty()) {
        AppConfig cfg = ConfigManager::loadConfig(fileName);
        // 更新 UI / Update UI
        apiAddressEdit->setText(cfg.api_address);
        apiKeyEdit->setText(cfg.api_key);
        modelCombo->setCurrentText(cfg.model_name);
        portEdit->setText(QString::number(cfg.port));
        tempSpin->setValue(cfg.temperature);
        contextSpin->setValue(cfg.context_num);
        threadSpin->setValue(cfg.max_threads);
        systemPromptEdit->setText(cfg.system_prompt);
        prePromptEdit->setText(cfg.pre_prompt);
        
        chkGlossary->setChecked(cfg.enable_glossary);
        glossaryPathEdit->setText(cfg.glossary_path);
        
        logArea->append(QString(LOG_CFG_LOADED[m_currentLang]) + fileName);
    }
}

/**
 * 导出日志到文件
 * Export log to file
 */
void MainWindow::onExportLog() {
    QString fileName = "run_log.txt";
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8); 
        out << logArea->toPlainText();
        logArea->append(LOG_EXPORTED[m_currentLang]);
    }
}

/**
 * 获取可用的模型列表 (网络请求)
 * Fetch available model list (Network Request)
 */
void MainWindow::onFetchModels() {
    QString url = apiAddressEdit->text();
    if(url.endsWith("/")) url.chop(1); // 移除末尾斜杠 / Remove trailing slash
    url += "/models";
    
    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    QNetworkRequest req(url);
    // 处理多 Key 情况，仅取第一个 / Handle multiple Keys, take the first one
    QString key = apiKeyEdit->text().split(',')[0].trimmed();
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    
    QNetworkReply *reply = mgr->get(req);
    
    // 异步处理响应 / Handle response asynchronously
    connect(reply, &QNetworkReply::finished, [this, reply, mgr](){
        if(reply->error() == QNetworkReply::NoError) {
            try {
                // 解析 JSON 响应 / Parse JSON response
                auto jsonDoc = nlohmann::json::parse(reply->readAll().toStdString());
                modelCombo->clear();
                for(const auto& item : jsonDoc["data"]) {
                    modelCombo->addItem(QString::fromStdString(item["id"]));
                }
                logArea->append(LOG_FETCH_SUCCESS[m_currentLang]);
            } catch(...) {
                logArea->append(LOG_PARSE_ERR[m_currentLang]);
            }
        } else {
            logArea->append(QString(LOG_FETCH_FAIL[m_currentLang]) + reply->errorString());
        }
        reply->deleteLater();
        mgr->deleteLater();
    });
}

/**
 * 测试所有API连接 (网络请求)
 * Test all API connections (Network Request)
 */
void MainWindow::onTestConfig() {
    
    logArea->append(LOG_TEST_START[m_currentLang]);
    
    // 支持逗号分隔的多个 Key / Support multiple keys separated by comma
    QStringList keys = apiKeyEdit->text().split(',', Qt::SkipEmptyParts);
    if (keys.isEmpty()) {
        logArea->append(LOG_NO_KEY[m_currentLang]);
        return;
    }

    QString url = apiAddressEdit->text();
    if(url.endsWith("/")) url.chop(1);
    url += "/chat/completions";
    QString model = modelCombo->currentText();

    // 遍历所有 Key 进行测试
    // Iterate through all Keys for testing
    for (int i = 0; i < keys.size(); ++i) {
        QString key = keys[i].trimmed();
        // 对 Key 进行脱敏显示 (只显示后8位) / Mask the Key (show only last 8 chars)
        QString keyMasked = (key.length() > 8) ? ("..." + key.right(8)) : key;

        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());

        // 构造最小测试负载 / Construct minimal test payload
        nlohmann::json j;
        j["model"] = model.toStdString();
        j["messages"] = nlohmann::json::array({{{"role", "user"}, {"content", "Hi"}}});
        j["max_tokens"] = 5; 

        QNetworkReply *reply = mgr->post(req, QByteArray::fromStdString(j.dump()));

        connect(reply, &QNetworkReply::finished, [this, reply, mgr, keyMasked, i](){
            if(reply->error() == QNetworkReply::NoError) {
                logArea->append(QString("✅ Key-%1 (%2): %3").arg(i+1).arg(keyMasked).arg(LOG_PASS[m_currentLang]));
            } else {
                logArea->append(QString("❌ Key-%1 (%2): %3 - %4").arg(i+1).arg(keyMasked).arg(LOG_FAIL[m_currentLang]).arg(reply->errorString()));
            }
            reply->deleteLater();
            mgr->deleteLater();
        });
    }
}

/**
 * 更新令牌消耗显示
 * Update token consumption display
 */
void MainWindow::updateTokenDisplay(long long total, long long prompt, long long completion) {
    lblTokens->setText(QString("%1 %2").arg(STR_TOKENS[m_currentLang]).arg(total));
    lblTokens->setToolTip(QString("Input: %1\nOutput: %2").arg(prompt).arg(completion));
}