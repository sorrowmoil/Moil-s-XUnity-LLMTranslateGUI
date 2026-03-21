#pragma once
// Prevent multiple inclusion of this header file
// 防止头文件被重复包含

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
#include <functional>             
#include "TranslationServer.h"    // Translation core service / 翻译核心服务
#include <QMenu>
#include <QDialog>                // Include QDialog for standalone windows / 引入QDialog以支持独立窗口
#include "TokenManager.h"         // Token count manager / Token计数管理器
#include "HudWindow.h"            // Floating window / HUD mode / 悬浮窗/HUD模式
#include "LoadingOverlay.h"       // Loading overlay widget / 加载遮罩层控件

// Main window class: classic mode interface
// 主窗口类：经典模式界面
class MainWindow : public QMainWindow {
    Q_OBJECT // Enable Qt meta‑object system (signals/slots) / 启用Qt元对象系统（信号/槽）

public:
    // Constructor
    // 构造函数
    MainWindow(QWidget *parent = nullptr);
    // Destructor
    // 析构函数
    ~MainWindow();

    // Public interface for main.cpp to synchronize state
    // 公开接口：供main.cpp同步状态使用
    TranslationServer* getServer() { return server; }
    TranslationServer* getServer() const { return server; }
    
    // Get current UI configuration as an AppConfig structure
    // 获取当前UI配置（以AppConfig结构体形式）
    AppConfig getUiConfig(); 
    // Load UI state from a configuration file (made public for external calls)
    // 从配置文件加载UI状态（设为公开以便外部调用）
    void loadConfigToUi(); 

    // Switch to modern mode (internal trigger)
    // 切换到流光模式（内部触发）
    void switchToModernMode();

protected:
    // Override close event for custom exit animation
    // 重写关闭事件以实现自定义退出动画
    void closeEvent(QCloseEvent *event) override;
    // Override show event to synchronize UI after being shown
    // 重写显示事件以在显示后同步UI
    void showEvent(QShowEvent *event) override; 

private slots:
    // Button click slots
    // 按钮点击槽函数
    void onStartClicked();
    void onStopClicked();
    void onTestConfig();
    void onFetchModels();
    void onSaveConfig();
    void onLoadConfig();
    void onExportLog();
    
    // Status update slots
    // 状态更新槽函数
    void updateTokenDisplay(long long total, long long prompt, long long completion);
    void onClearContext();
    void onLogMessage(QString msg);
    
    // Context menu slots
    // 上下文菜单槽函数
    void onLogContextMenu(const QPoint &pos);
    void onGlossaryContextMenu(const QPoint &pos); 
    
    // Animation and UI control
    // 动画与界面控制
    void fadeOutAndClose();
    void toggleTheme();
    void toggleLanguage();
    
    // Glossary features
    // 术语表功能
    void onSelectGlossary();
    void onOpenAutoTranslations();
    void onGlossaryChanged();
    
    // HUD mode switching
    // HUD模式切换
    void switchToHud();             
    void restoreFromHud();          
    
    // Server status listening
    // 服务器状态监听
    void onServerWorkStarted();     
    void onServerWorkFinished(bool success); 
    
    // UI mode switching
    // 界面模式切换
    void onSwitchToModern();

    // Glossary editor slots (new)
    // 术语表编辑器专用槽函数（新增）
    void openGlossaryEditor();
    void saveGlossaryEditor();

signals:
    // Signal to request switch to modern view (notifies main.cpp to perform window switching)
    // 请求切换到流光模式的信号（通知main.cpp进行窗口切换）
    void requestModernView(); 

private:
    // Initialize UI layout
    // 初始化UI布局
    void setupUi();
    // Enable/disable controls based on server running state
    // 根据服务器运行状态启用/禁用控件
    void toggleControls(bool running); 
    // Apply theme (dark/light)
    // 应用主题（深色/浅色）
    void applyTheme(bool isDark);      
    // Update all UI texts according to current language
    // 根据当前语言更新所有界面文本
    void updateUIText();        
    // Add a glossary path to the history combo box
    // 添加术语表路径到历史记录下拉框
    void addToGlossaryHistory(const QString& path);   
    // Smooth transition logic (used for visual effects during UI changes)
    // 平滑切换逻辑（用于界面变化时的视觉效果）
    void smoothSwitch(std::function<void()> changeLogic);

    // State flags
    // 状态标志
    bool m_isClosing = false;           // Whether the window is closing / 是否正在关闭
    bool m_isDarkTheme = true;          // Current theme (dark/light) / 当前主题
    int m_currentLang = 0;              // Current language index (0=English, 1=Chinese) / 当前语言索引
    bool m_isServerRunning = false;     // Server running status / 服务器运行状态
    
    // Get a user-friendly error message based on HTTP status code
    // 根据HTTP状态码获取用户友好的错误信息
    QString getFriendlyErrorMessage(int code, int lang);
    
    // Loading overlay pointers
    // 加载遮罩指针
    LoadingOverlay *fetchLoadingOverlay = nullptr; 
    LoadingOverlay *testLoadingOverlay = nullptr;

    // Core UI component definitions (ensuring MainWindow.cpp can access them)
    // 核心UI组件定义（确保MainWindow.cpp能够访问它们）
    
    // Configuration input area
    // 配置输入区
    QComboBox *apiAddressCombo;         // API address / API地址
    QLineEdit *apiKeyEdit;              // API key / API密钥
    QComboBox *modelCombo;              // Model selection / 模型选择
    QLineEdit *portEdit;                // Port number / 端口号
    QDoubleSpinBox *tempSpin;           // Temperature parameter / 温度参数
    QSpinBox *contextSpin;              // Context length / 上下文长度
    QSpinBox *threadSpin;               // Thread count / 线程数
    QTextEdit *systemPromptEdit;        // System prompt / 系统提示词
    QLineEdit *prePromptEdit;           // Pre‑prompt / 预提示词
    
    // Log area
    // 日志区域
    QTextEdit *logArea;                 // Log display / 日志显示
    
    // Glossary controls
    // 术语表控制
    QCheckBox *chkGlossary;             // Enable glossary / 启用术语表
    QComboBox *glossaryCombo;           // Glossary selection / 术语表选择 (restored / 已补回)
    QCheckBox *chkLockSysPrompt;        // Lock system prompt / 锁定系统提示
    QCheckBox *chkLockGlossary;         // Lock glossary path / 锁定术语表路径 (restored / 已补回)

    // Function buttons
    // 功能按钮
    QPushButton *btnSelectGlossary;     // Select glossary file / 选择术语表文件
    QPushButton *btnOpenAuto;           // Open auto‑generated translations file / 打开自动翻译文件

    // Main control buttons
    // 主控制按钮
    QPushButton *startBtn;              // Start / 启动
    QPushButton *stopBtn;               // Stop / 停止
    QPushButton *hudBtn;                // HUD mode / 悬浮窗模式
    QPushButton *fetchModelBtn;         // Fetch models list / 获取模型列表
    QPushButton *themeBtn;              // Toggle theme / 切换主题
    QPushButton *testBtn;               // Test configuration / 测试配置
    QPushButton *loadBtn;               // Load configuration / 加载配置
    QPushButton *saveBtn;               // Save configuration / 保存配置
    QPushButton *exportBtn;             // Export log / 导出日志
    QPushButton *langBtn;               // Toggle language / 切换语言
    QPushButton *clearCtxBtn;           // Clear context / 清除上下文
    QPushButton *modernBtn;             // Switch to modern mode / 切换到流光模式
    QPushButton *editGlossaryBtn;       // Edit glossary (new) / 编辑术语表按钮（新增）

    // Group boxes
    // 分组框
    QGroupBox *cfgGroup;                // Configuration group / 配置组
    QGroupBox *logGroup;                // Log group / 日志组
    
    // Labels
    // 标签
    QLabel *lblApiAddr;
    QLabel *lblApiKey;
    QLabel *lblModel;
    QLabel *lblPort;
    QLabel *lblThread;
    QLabel *lblTemp;
    QLabel *lblCtx;
    QLabel *lblSysPrompt;
    QLabel *lblPrePrompt;
    QLabel *lblGlossary; 
    QLabel *lblTokens;                  // Token display / Token显示
    QCheckBox *chkDebug;                // Speed test mode switch / 测速模式开关
    QCheckBox *chkBatch;                // Batch translation switch / 打包翻译开关

    // Persistent members for the glossary editor (new)
    // 术语表编辑器的持久化成员（新增）
    QDialog *m_glossaryEditor = nullptr;
    QTextEdit *m_glossaryTextEdit = nullptr;
    QPushButton *m_glossarySaveBtn = nullptr;
    QPushButton *m_glossaryCancelBtn = nullptr;
    QString m_currentEditingPath;       // Path of the glossary being edited / 当前正在编辑的术语表路径

    // Core logic objects
    // 核心逻辑对象
    TranslationServer *server;          // Translation service instance / 翻译服务实例
    QPropertyAnimation *fadeAnim;       // Fade animation for window transitions / 窗口淡入淡出动画
    TokenManager *m_tokenManager;       // Token manager / Token管理器
    HudWindow *m_hudWindow = nullptr;   // HUD window instance / 悬浮窗实例
};