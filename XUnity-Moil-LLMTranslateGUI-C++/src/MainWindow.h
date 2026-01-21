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
#include <QGraphicsOpacityEffect> 
#include <functional>             
#include "TranslationServer.h"
#include <QMenu>
#include "TokenManager.h"
#include "HudWindow.h" // 引入 HUD 头文件

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // 基础按钮槽
    void onStartClicked();
    void onStopClicked();
    void onTestConfig();
    void onFetchModels();
    void onSaveConfig();
    void onLoadConfig();
    void onExportLog();
    void updateTokenDisplay(long long total, long long prompt, long long completion);
    void onClearContext();

    // 日志
    void onLogMessage(QString msg);
    void onLogContextMenu(const QPoint &pos);

    // 动画与界面
    void fadeOutAndClose();
    void toggleTheme();
    void toggleLanguage();
    void onSelectGlossary();
    
    // --- HUD 模式相关槽函数 (新增) ---
    void switchToHud();             // 切换到 HUD
    void restoreFromHud();          // 从 HUD 还原
    void onServerWorkStarted();     // 服务器开始忙碌
    void onServerWorkFinished(bool success); // 服务器工作结束

private:
    void setupUi();
    void loadConfigToUi();
    AppConfig getUiConfig();
    void toggleControls(bool running); 
    void applyTheme(bool isDark);      
    void updateUIText();               
    
    void smoothSwitch(std::function<void()> changeLogic);

    bool m_isClosing = false;
    bool m_isDarkTheme = true;
    int m_currentLang = 0;

    // UI 组件
    QComboBox *apiAddressCombo;
    QLineEdit *apiKeyEdit;
    QComboBox *modelCombo;
    QLineEdit *portEdit;
    QDoubleSpinBox *tempSpin;
    QSpinBox *contextSpin;
    QSpinBox *threadSpin;
    QTextEdit *systemPromptEdit;
    QLineEdit *prePromptEdit;
    QTextEdit *logArea;
    
    QCheckBox *chkGlossary;       
    QLineEdit *glossaryPathEdit;  
    QPushButton *btnSelectGlossary; 

    // 按钮
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QPushButton *hudBtn; // 新增 HUD 按钮
    QPushButton *fetchModelBtn;
    QPushButton *themeBtn;
    QPushButton *testBtn;
    QPushButton *loadBtn;
    QPushButton *saveBtn;
    QPushButton *exportBtn;
    QPushButton *langBtn;
    QPushButton *clearCtxBtn;  

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
    QLabel *lblTokens;

    TranslationServer *server;
    QPropertyAnimation *fadeAnim; 
    TokenManager *m_tokenManager; 
    
    // --- HUD 窗口实例 (新增) ---
    HudWindow *m_hudWindow = nullptr;
};
