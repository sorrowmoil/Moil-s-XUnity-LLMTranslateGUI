#pragma once

// Qt 核心模块 / Qt Core Modules
#include <QMainWindow>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox> 
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect> 
#include <functional>             // For std::function 用于std::function

// 自定义类 / Custom Classes
#include "TranslationServer.h"
#include <QMenu>
#include "TokenManager.h"
#include "HudWindow.h"
#include "LoadingOverlay.h" 

/**
 * 主窗口类 - MainWindow Class
 * 
 * Moil's XUnity LLM Translator 应用程序的主GUI窗口。
 * 处理API配置、服务器控制、日志记录、主题管理和HUD模式。
 * 
 * Main GUI window for Moil's XUnity LLM Translator application.
 * Handles API configuration, server control, logging, theme management, and HUD mode.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT  // Qt元对象系统宏，启用信号槽机制 / Qt Meta-Object System macro, enables signals/slots

public:
    /**
     * 构造函数 / Constructor
     * @param parent 父部件（可选） / Parent widget (optional)
     */
    MainWindow(QWidget *parent = nullptr);
    
    /**
     * 析构函数 / Destructor
     * 清理资源并确保正确关闭 / Cleans up resources and ensures proper shutdown
     */
    ~MainWindow();

protected:
    /**
     * 关闭事件处理函数 / Close event handler
     * 重写以处理带有淡出动画的自定义关闭行为
     * Override to handle custom close behavior with fade-out animation
     * @param event 关闭事件 / The close event
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    // ==========================================
    // 基础按钮槽 / Basic Button Slots
    // ==========================================
    
    /**
     * 启动/重载按钮处理函数 / Start/Reload button handler
     * 这个槽函数现在兼任双重功能：启动服务或热重载配置
     * This slot now serves dual purpose: start service or hot reload config
     */
    void onStartClicked();
    
    /**
     * 停止按钮处理函数 / Stop button handler
     * 停止翻译服务器 / Stops the translation server
     */
    void onStopClicked();
    
    /**
     * 测试配置按钮处理函数 / Test configuration button handler
     * 测试所有已配置的API密钥的连接性
     * Tests all configured API keys for connectivity
     */
    void onTestConfig();
    
    /**
     * 获取模型按钮处理函数 / Fetch models button handler
     * 从配置的API端点检索可用模型
     * Retrieves available models from the configured API endpoint
     */
    void onFetchModels();
    
    /**
     * 保存配置按钮处理函数 / Save configuration button handler
     * 将当前UI设置保存到配置文件
     * Saves current UI settings to a configuration file
     */
    void onSaveConfig();
    
    /**
     * 加载配置按钮处理函数 / Load configuration button handler
     * 从配置文件加载设置到UI
     * Loads settings from a configuration file into the UI
     */
    void onLoadConfig();
    
    /**
     * 导出日志按钮处理函数 / Export log button handler
     * 将运行时日志导出到文本文件
     * Exports the runtime log to a text file
     */
    void onExportLog();
    
    /**
     * 更新Token显示槽 / Update token display slot
     * 使用当前统计数据更新Token使用量显示
     * Updates the token usage display with current statistics
     * @param total 使用的总Token数 / Total tokens used
     * @param prompt 使用的提示Token数 / Prompt tokens used
     * @param completion 使用的完成Token数 / Completion tokens used
     */
    void updateTokenDisplay(long long total, long long prompt, long long completion);
    
    /**
     * 清除上下文按钮处理函数 / Clear context button handler
     * 清除服务器中的所有对话上下文记忆
     * Clears all conversation context memory in the server
     */
    void onClearContext();

    // ==========================================
    // 日志相关槽 / Log Related Slots
    // ==========================================
    
    /**
     * 日志消息处理函数 / Log message handler
     * 接收并显示来自服务器的日志消息
     * Receives and displays log messages from the server
     * @param msg 要显示的日志消息 / The log message to display
     */
    void onLogMessage(QString msg);
    
    /**
     * 日志上下文菜单处理函数 / Log context menu handler
     * 为日志区域显示自定义上下文菜单
     * Shows a custom context menu for the log area
     * @param pos 上下文菜单被请求的位置 / The position where the context menu was requested
     */
    void onLogContextMenu(const QPoint &pos);

    // ==========================================
    // 动画与界面槽 / Animation & UI Slots
    // ==========================================
    
    /**
     * 淡出并关闭动画 / Fade out and close animation
     * 在关闭应用程序前执行淡出动画
     * Performs fade-out animation before closing the application
     */
    void fadeOutAndClose();
    
    /**
     * 切换主题 / Toggle theme
     * 在亮色和暗色主题之间切换
     * Switches between light and dark themes
     */
    void toggleTheme();
    
    /**
     * 切换语言 / Toggle language
     * 在英文和中文UI文本之间切换
     * Switches between English and Chinese UI text
     */
    void toggleLanguage();
    
    /**
     * 选择术语表文件 / Select glossary file
     * 打开文件对话框选择术语表文件
     * Opens a file dialog to select the glossary file
     */
    void onSelectGlossary();
    
    /**
     * 打开自动生成的翻译文件 / Open auto-generated translations file
     * 在默认编辑器中打开 _AutoGeneratedTranslations.txt 文件
     * Opens the _AutoGeneratedTranslations.txt file in the default editor
     */
    void onOpenAutoTranslations();
    
    // ==========================================
    // HUD 模式相关槽 / HUD Mode Related Slots
    // ==========================================
    
    /**
     * 切换到HUD模式 / Switch to HUD mode
     * 最小化主窗口并切换到HUD覆盖层模式
     * Minimizes main window and switches to HUD overlay mode
     */
    void switchToHud();
    
    /**
     * 从HUD模式恢复 / Restore from HUD mode
     * 从HUD模式返回主窗口
     * Returns from HUD mode to the main window
     */
    void restoreFromHud();
    
    /**
     * 服务器工作开始处理函数 / Server work started handler
     * 当服务器开始处理翻译工作时调用
     * Called when the server starts processing translation work
     */
    void onServerWorkStarted();
    
    /**
     * 服务器工作完成处理函数 / Server work finished handler
     * 当服务器完成处理翻译工作时调用
     * Called when the server finishes processing translation work
     * @param success 操作是否成功 / Whether the operation was successful
     */
    void onServerWorkFinished(bool success);

    // ==========================================
    // 🔥 CAN: 新增功能槽 / New Feature Slots
    // ==========================================
    
    /**
     * 🔥 术语表右键菜单槽 / Glossary context menu slot
     * 为术语表组合框显示自定义上下文菜单
     * Shows a custom context menu for the glossary combo box
     * @param pos 上下文菜单被请求的位置 / The position where the context menu was requested
     */
    void onGlossaryContextMenu(const QPoint &pos);

    /**
     * 🔥 术语表更改槽函数 / Glossary changed slot function
     * 当术语表路径改变时调用的槽函数
     * Slot function called when glossary path changes
     */
    void onGlossaryChanged();

private:
    // ==========================================
    // 初始化与设置方法 / Initialization & Setup Methods
    // ==========================================
    
    /**
     * 设置用户界面 / Setup user interface
     * 创建并排列所有UI部件和布局
     * Creates and arranges all UI widgets and layouts
     */
    void setupUi();
    
    /**
     * 加载配置到UI / Load configuration to UI
     * 从文件读取配置并填充UI元素
     * Reads configuration from file and populates UI elements
     */
    void loadConfigToUi();
    
    /**
     * 获取UI配置 / Get UI configuration
     * 将当前UI状态收集到配置对象中
     * Collects current UI state into a configuration object
     * @return 当前应用程序配置 / Current application configuration
     */
    AppConfig getUiConfig();
    
    /**
     * 切换控件状态 / Toggle control states
     * 根据服务器运行状态启用/禁用UI控件
     * Enables/disables UI controls based on server running state
     * @param running 服务器当前是否正在运行 / Whether the server is currently running
     */
    void toggleControls(bool running);
    
    /**
     * 应用主题 / Apply theme
     * 将颜色主题（亮色或暗色）应用到应用程序
     * Applies color theme (light or dark) to the application
     * @param isDark true为暗色主题，false为亮色主题 / true for dark theme, false for light theme
     */
    void applyTheme(bool isDark);
    
    /**
     * 更新UI文本 / Update UI text
     * 根据当前语言更新所有UI文本元素
     * Updates all UI text elements based on current language
     */
    void updateUIText();
    
    /**
     * 添加路径到术语表历史记录 / Add path to glossary history
     * 管理术语表文件路径历史记录的辅助函数
     * Helper function to manage glossary file path history
     * @param path 要添加的术语表文件路径 / The glossary file path to add
     */
    void addToGlossaryHistory(const QString& path);
    
    /**
     * 平滑切换动画 / Smooth switch animation
     * 在更改UI状态时执行平滑视觉过渡
     * Performs a smooth visual transition when changing UI state
     * @param changeLogic 要执行的实际UI更改逻辑 / The actual UI change logic to execute
     */
    void smoothSwitch(std::function<void()> changeLogic);

    /**
     * 获取人性化错误信息 / Get friendly error message
     * 将HTTP错误代码映射到当前语言的用户友好消息
     * Maps HTTP error codes to user-friendly messages in the current language
     * @param code HTTP错误代码或自定义超时代码（999）
     *            HTTP error code or custom timeout code (999)
     * @param lang 语言索引（0=英文，1=中文）
     *            Language index (0=English, 1=Chinese)
     * @return 用户友好的错误信息 / User-friendly error message
     */
    QString getFriendlyErrorMessage(int code, int lang);

    // ==========================================
    // 成员变量 / Member Variables
    // ==========================================
    
    // 状态标志 / State Flags
    bool m_isClosing = false;          // 防止多次关闭事件的标志 / Flag to prevent multiple close events
    bool m_isDarkTheme = true;         // 当前主题（true=暗色，false=亮色）/ Current theme (true=dark, false=light)
    int m_currentLang = 0;             // 当前语言（0=英文，1=中文）/ Current language (0=English, 1=Chinese)
    bool m_isServerRunning = false;    // 服务器运行状态追踪 / Server running state tracking

    // UI组件 / UI Components
    
    // 主要配置控件 / Main Configuration Controls
    QComboBox *apiAddressCombo;        // API地址组合框 / API address combo box
    QLineEdit *apiKeyEdit;             // API密钥编辑框 / API key line edit
    QComboBox *modelCombo;             // 模型组合框 / Model combo box
    QLineEdit *portEdit;               // 端口编辑框 / Port line edit
    QDoubleSpinBox *tempSpin;          // 温度微调框 / Temperature spin box
    QSpinBox *contextSpin;             // 上下文微调框 / Context spin box
    QSpinBox *threadSpin;              // 线程微调框 / Thread spin box
    QTextEdit *systemPromptEdit;       // 系统提示词编辑框 / System prompt text edit
    QLineEdit *prePromptEdit;          // 前置文本编辑框 / Pre-prompt line edit
    
    // 日志区域 / Log Area
    QTextEdit *logArea;                // 日志文本编辑框 / Log text edit
    
    // 术语表控件 / Glossary Controls
    QCheckBox *chkGlossary;            // 术语表启用复选框 / Glossary enable check box
    QComboBox *glossaryCombo;          // 术语表路径组合框 / Glossary path combo box
    
    // 🔥 新增锁定控件 / New Lock Controls
    QCheckBox *chkLockSysPrompt;       // ✅ 锁定系统提示词的复选框 / Lock system prompt check box
    QCheckBox *chkLockGlossary;        // 锁定术语表复选框 / Lock glossary check box

    // 文件选择按钮 / File Selection Buttons
    QPushButton *btnSelectGlossary;    // 选择术语表按钮 / Select glossary button
    QPushButton *btnOpenAuto;          // 打开自动翻译文件按钮 / Open auto-translations button

    // 控制按钮 / Control Buttons
    QPushButton *startBtn;             // 启动/重载按钮 / Start/Reload button
    QPushButton *stopBtn;              // 停止按钮 / Stop button
    QPushButton *hudBtn;               // HUD模式按钮 / HUD mode button
    QPushButton *fetchModelBtn;        // 获取模型按钮 / Fetch models button
    QPushButton *themeBtn;             // 主题切换按钮 / Theme toggle button
    QPushButton *testBtn;              // 测试配置按钮 / Test configuration button
    QPushButton *loadBtn;              // 加载配置按钮 / Load configuration button
    QPushButton *saveBtn;              // 保存配置按钮 / Save configuration button
    QPushButton *exportBtn;            // 导出日志按钮 / Export log button
    QPushButton *langBtn;              // 语言切换按钮 / Language toggle button
    QPushButton *clearCtxBtn;          // 清除上下文按钮 / Clear context button

    // 分组框 / Group Boxes
    QGroupBox *cfgGroup;               // 配置分组框 / Configuration group box
    QGroupBox *logGroup;               // 日志分组框 / Log group box

    // 标签 / Labels
    QLabel *lblApiAddr;                // API地址标签 / API address label
    QLabel *lblApiKey;                 // API密钥标签 / API key label
    QLabel *lblModel;                  // 模型标签 / Model label
    QLabel *lblPort;                   // 端口标签 / Port label
    QLabel *lblThread;                 // 线程标签 / Thread label
    QLabel *lblTemp;                   // 温度标签 / Temperature label
    QLabel *lblCtx;                    // 上下文标签 / Context label
    QLabel *lblSysPrompt;              // 系统提示词标签 / System prompt label
    QLabel *lblPrePrompt;              // 前置文本标签 / Pre-prompt label
    QLabel *lblGlossary;               // 术语表标签 / Glossary label
    QLabel *lblTokens;                 // Token统计标签 / Token statistics label

    // 核心组件 / Core Components
    TranslationServer *server;         // 翻译服务器实例 / Translation server instance
    QPropertyAnimation *fadeAnim;      // 淡入淡出动画 / Fade animation
    TokenManager *m_tokenManager;      // Token管理器 / Token manager
    
    // HUD窗口 / HUD Window
    HudWindow *m_hudWindow = nullptr;  // HUD窗口实例 / HUD window instance

    // 加载覆盖层 / Loading Overlay
    LoadingOverlay *fetchLoadingOverlay = nullptr; // 获取模型时的加载动画覆盖层 / Loading overlay for fetching models
};