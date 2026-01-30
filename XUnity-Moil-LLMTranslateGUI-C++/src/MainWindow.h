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
#include "HudWindow.h" 

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
    
    // 📝 新增：打开自动翻译文件的槽函数
    void onOpenAutoTranslations();
    
    // --- HUD 模式相关槽函数 ---
    void switchToHud();             
    void restoreFromHud();          
    void onServerWorkStarted();     
    void onServerWorkFinished(bool success); 

private:
    void setupUi();
    void loadConfigToUi();
    AppConfig getUiConfig();
    void toggleControls(bool running); 
    void applyTheme(bool isDark);      
    void updateUIText();        
    
    // 添加路径到历史记录的辅助函数
    void addToGlossaryHistory(const QString& path);   
    
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
    QComboBox *glossaryCombo;  
    
    QPushButton *btnSelectGlossary; 
    // 📝 新增：编辑按钮
    QPushButton *btnOpenAuto;

    // 按钮
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QPushButton *hudBtn; 
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
    
    HudWindow *m_hudWindow = nullptr;
};
