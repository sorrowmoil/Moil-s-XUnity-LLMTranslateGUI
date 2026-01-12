#pragma once
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
#include <QGraphicsOpacityEffect> // <--- 新增：用于透明度动画 / New: For opacity animation
#include <functional>             // <--- 新增：用于回调函数 / New: For callback functions
#include "TranslationServer.h"
#include <QMenu>

// 主窗口类，负责 UI 展示和用户交互
// Main Window class, responsible for UI presentation and user interaction
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // 重写关闭事件，处理退出动画和配置保存
    // Override close event to handle exit animation and config saving
    void closeEvent(QCloseEvent *event) override;

private slots:
    // 按钮点击槽函数 / Button click slots
    void onStartClicked();       // 启动服务 / Start Service
    void onStopClicked();        // 停止服务 / Stop Service
    void onTestConfig();         // 测试 API 配置 / Test API Configuration
    void onFetchModels();        // 从服务器获取模型列表 / Fetch model list from server
    void onSaveConfig();         // 手动保存配置 / Manually save config
    void onLoadConfig();         // 手动加载配置 / Manually load config
    void onExportLog();          // 导出日志到文件 / Export logs to file
    
    // 日志接收槽函数 / Log reception slot
    void onLogMessage(QString msg);
    
    // 动画与界面相关槽函数 / Animation and UI related slots
    void fadeOutAndClose();      // 淡出并关闭窗口 / Fade out and close window
    void toggleTheme();          // 切换深色/浅色主题 / Toggle Dark/Light theme
    void toggleLanguage();       // 切换中/英文界面 / Toggle Chinese/English UI
    void onSelectGlossary();     // 选择术语表文件 / Select glossary file
    
    // 右键菜单槽函数 / Context menu slot
    void onLogContextMenu(const QPoint &pos); 

private:
    // UI 初始化与更新逻辑 / UI Initialization and Update Logic
    void setupUi();
    void loadConfigToUi();
    AppConfig getUiConfig();
    void toggleControls(bool running); // 运行时锁定控件 / Lock controls when running
    void applyTheme(bool isDark);      // 应用主题样式 / Apply theme style
    void updateUIText();               // 更新界面文本（多语言） / Update UI text (Multi-language)
    
    // ✨ 新增：平滑切换辅助函数
    // ✨ New: Helper function for smooth transition
    // 用于在切换主题或语言时提供视觉上的平滑过渡
    // Used to provide a smooth visual transition when switching themes or languages
    void smoothSwitch(std::function<void()> changeLogic);

    bool m_isClosing = false;   // 是否正在关闭中 / Is currently closing
    bool m_isDarkTheme = true;  // 当前是否为深色主题 / Is currently dark theme
    int m_currentLang = 0;      // 当前语言索引 (0: English, 1: Chinese)

    // UI Components (UI 组件指针)
    QLineEdit *apiAddressEdit;
    QLineEdit *apiKeyEdit;
    QComboBox *modelCombo;
    QLineEdit *portEdit;
    QDoubleSpinBox *tempSpin;
    QSpinBox *contextSpin;
    QSpinBox *threadSpin;
    QTextEdit *systemPromptEdit;
    QLineEdit *prePromptEdit;
    QTextEdit *logArea;
    
    // Glossary UI (术语表组件)
    QCheckBox *chkGlossary;       
    QLineEdit *glossaryPathEdit;  
    QPushButton *btnSelectGlossary; 

    // Buttons (控制按钮)
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QPushButton *fetchModelBtn;
    QPushButton *themeBtn;
    QPushButton *testBtn;
    QPushButton *loadBtn;
    QPushButton *saveBtn;
    QPushButton *exportBtn;
    QPushButton *langBtn;

    // Labels & Groups (标签和分组框)
    QGroupBox *cfgGroup;
    QGroupBox *logGroup;
    
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

    // 核心逻辑对象 / Core Logic Objects
    TranslationServer *server;
    QPropertyAnimation *fadeAnim; // 窗口透明度动画 / Window opacity animation
};