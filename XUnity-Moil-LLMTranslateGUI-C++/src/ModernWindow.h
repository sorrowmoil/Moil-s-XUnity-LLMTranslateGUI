#pragma once

#include <QMainWindow>
#include <QPointer>

// 🔥 新增：补全 C++ 编译器需要的基础控件定义
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QScrollArea>

#include "TranslationServer.h"
#include "TokenManager.h"
#include "ModernUI.h"

class ModernWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ModernWindow(TranslationServer *server, QWidget *parent = nullptr);
    ~ModernWindow();

    TranslationServer *getServer() const { return m_server; }
    AppConfig getUiConfig();
    void loadConfigToUi();

    // 🎨 毛玻璃参数访问器（供调色板适配器使用）
    bool getIsDark() const { return m_isDark; }
    int getAlpha() const { return m_alpha; }
    bool getIsRounded() const { return m_storedIsRounded; }
    int getHueShift() const { return m_hueShift; }
    int getTintIntensity() const { return m_tintIntensity; }
    GlassRenderMode getGlassRenderMode() const { return m_glassRenderMode; }

signals:
    void requestClassicMode();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    // 防误触拖动支持
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void leaveEvent(QEvent *event) override; // ✨ 新增：用于鼠标离开时恢复光标

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private slots:
    void onLoadConfig();
    void onSaveConfig();
    void toggleLanguage();
    void toggleTheme();
    void onExportLog();
    void onSwitchClicked();
    void onPowerClicked();
    void onStopClicked();
    void onFetchModels();
    void onTestConfig();
    void onOpacityChange(int val);
    void updateLog(QString msg);
    void updateToken(long long total, long long p, long long c);
    void onClearContext();
    void onOpenAutoTranslations();
    void onEditGlossaryClicked(); // 📝 编辑术语表
    void onGlossaryContextMenu(const QPoint &pos);
    void onLogContextMenu(const QPoint &pos);
    void onBatchContextMenu(const QPoint &pos); // 多行模式右键菜单
    void onApiComboContextMenu(const QPoint &pos);
    void onGlossaryChanged();
    void onSelectGlossary();
    void toggleShape();
    void toggleDebugMode();
    void toggleGlassStyle(); // 🎨 切换毛玻璃渲染模式
    void onHueChanged(int hueOffset); // 🎨 色相偏移调节
    void onTintIntensityChanged(int intensity); // 🎨 色彩流光浓度调节
    void toggleHuePalette(); // 🎨 打开色相调色板
    
    
private:
    void setupApiKeyMemory();
    void handleApiBaseUrlChanged();
    void persistCurrentApiKeyMemory();

    void setupUi();
    void updateUIText();
    void updatePowerButtonState(bool running);
    void addToGlossaryHistory(const QString &path);
    QString getFriendlyErrorMessage(int code, int lang);
    bool m_isRounded = true; // 状态标记
    QPushButton *btnShape;   // 新按钮

    void resetAllCursors()
    {
        QTimer::singleShot(20, [this]()
                           {
            if (apiAddressCombo && apiAddressCombo->lineEdit()) apiAddressCombo->lineEdit()->setCursorPosition(0);
            if (modelCombo && modelCombo->lineEdit()) modelCombo->lineEdit()->setCursorPosition(0);
            if (glossaryCombo && glossaryCombo->lineEdit()) glossaryCombo->lineEdit()->setCursorPosition(0);
            if (apiKeyEdit) apiKeyEdit->setCursorPosition(0);
            if (prePromptEdit) prePromptEdit->setCursorPosition(0); });
    }

    void updateComboEnv()
    {
        if (apiAddressCombo) {
            apiAddressCombo->setEnv(m_isDark, m_alpha, m_isRounded);
            apiAddressCombo->setRenderMode(m_glassRenderMode);
            apiAddressCombo->setHueShift(m_hueShift);
            apiAddressCombo->setTintIntensity(m_tintIntensity);
        }
        if (modelCombo) {
            modelCombo->setEnv(m_isDark, m_alpha, m_isRounded);
            modelCombo->setRenderMode(m_glassRenderMode);
            modelCombo->setHueShift(m_hueShift);
            modelCombo->setTintIntensity(m_tintIntensity);
        }
        if (glossaryCombo) {
            glossaryCombo->setEnv(m_isDark, m_alpha, m_isRounded);
            glossaryCombo->setRenderMode(m_glassRenderMode);
            glossaryCombo->setHueShift(m_hueShift);
            glossaryCombo->setTintIntensity(m_tintIntensity);
        }
        for (GlassCard *card : m_glassCards) {
            if (card) {
                card->setRenderMode(m_glassRenderMode);
                card->setHueShift(m_hueShift);
            }
        }
        if (m_glossaryDrawer) {
            static_cast<GlossaryDrawer*>(m_glossaryDrawer.data())->setRenderMode(m_glassRenderMode);
            static_cast<GlossaryDrawer*>(m_glossaryDrawer.data())->setHueShift(m_hueShift);
            static_cast<GlossaryDrawer*>(m_glossaryDrawer.data())->setTintIntensity(m_tintIntensity);
        }
        if (btnPalette) {
            btnPalette->setVisible(m_glassRenderMode == GlassRenderMode::Frosted);
        }
    }

    void smoothSwitch(std::function<void()> changeLogic);

    // 核心对象
    TranslationServer *m_server;
    TokenManager *m_tokenManager;

    // UI 状态
    QPoint m_dragPos;
    bool m_isDragging = false; // 防误触标志
    int m_alpha;
    int m_hueShift = 0; // 🎨 全局色相偏移量
    int m_tintIntensity = 100; // 🎨 全局色彩流光浓度 (0~200)
    int m_lang;
    bool m_isDark;
    bool m_isServerRunning;
    int m_storedModernOpacity = 210;
    bool m_storedIsRounded = true;
    bool m_isDebugMode = false; // 测速模式状态
    bool m_isHandleRichText = false; // 文本处理模式状态
    bool m_isExtractNewline = true; // ↩️ 提取换行模式状态
    GlassRenderMode m_glassRenderMode = GlassRenderMode::Frosted; // 🎨 毛玻璃渲染模式

    int m_resizeEdge = 0;
    QRect m_dragStartGeom;
    QPoint m_dragStartGlobalPos;
    bool m_apiKeyMemoryEnabled = false;
    QString m_lastApiBaseUrl;
    void updateCursorShape(const QPoint &pos);

    // UI 组件指针
    QPointer<QWidget> m_glossaryDrawer; // 悬浮抽屉
    QPointer<QWidget> m_palettePopup;   // 色相调色板
    class PaletteUpdateAdapter* m_paletteAdapter; // 💎 调色板实时同步适配器
    QSlider *m_opacitySlider;
    QPushButton *btnClose, *btnMin;
    QPushButton *btnDebug, *btnLoad, *btnSave, *btnLang, *btnTheme, *btnExport, *btnBack;
    QPushButton *btnGlassStyle; // 🎨 毛玻璃风格切换按钮
    QPushButton *btnPalette;    // 🎨 色相调色板按钮

    SideGlassCombo *apiAddressCombo, *modelCombo, *glossaryCombo;
    
    // 🌫️ 毛玻璃效果：跟踪所有GlassCard实例
    QList<GlassCard*> m_glassCards;
    QLineEdit *apiKeyEdit, *portEdit, *prePromptEdit;
    QSpinBox *threadSpin, *contextSpin;
    QDoubleSpinBox *tempSpin;
    QTextEdit *systemPromptEdit, *logArea;
    QCheckBox *chkGlossary, *chkLockGlossary, *chkLockSysPrompt, *chkBatch, *chkHandleRichText, *chkExtractNewline;
    QPushButton *btnPower, *btnFetch, *btnTest, *btnClearCtx, *btnEditAuto, *btnEditGlossary, *btnSelectGlossary, *btnStop;
    QLabel *lblTokens, *lblApi, *lblKey, *lblMod, *lblPrt, *lblThd, *lblTmp, *lblCtx, *lblSys, *lblPre, *lblGlo;
};