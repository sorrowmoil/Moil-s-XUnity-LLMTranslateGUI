#ifndef ADVANCEDSETTINGS_H
#define ADVANCEDSETTINGS_H

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSpinBox>
#include <QString>
#include <QTabWidget>
#include <functional> // 👈 引入 std::function

class AdvancedSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdvancedSettingsDialog(
        QWidget *parent = nullptr,
        int lang = 1,
        bool isDark = true,
        bool isModern = false,
        int alpha = 255,
        int renderMode = 0,
        int hueShift = 0,
        int tintIntensity = 100,
        bool isRounded = true);

    int getRetries() const;
    int getTimeoutMs() const;

    QString getHijackFromLang() const;
    QString getHijackToLang() const;
    QString getHijackEndpoint() const;

    bool getHijackTextGetter() const;
    bool getHijackEnableImGui() const;
    bool getHijackEnableUGui() const;
    bool getHijackEnableUIElements() const;
    bool getHijackEnableNGUI() const;
    bool getHijackEnableTextMeshPro() const;
    bool getHijackEnableTextMesh() const;
    bool getHijackEnableFairyGUI() const;

    void updateLanguage(int lang);
    void updateTheme(bool isDark);

    void updateGlassEnv(
        bool isDark,
        int alpha,
        bool isRounded,
        int renderMode,
        int hueShift,
        int tintIntensity);

public slots:
    void accept() override;
    void reject() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // 🌟 平滑淡入淡出过渡引擎
    void smoothSwitch(std::function<void()> changeLogic);

    void setComboValue(QComboBox *combo, const QString &value);

    void applyNetworkPreset(const QString &preset);
    void updateNetworkPresetFromValues();
    void updateNetworkSummary();

    QTabWidget *m_tabWidget;

    // 网络页
    QWidget *m_tabNetwork;
    QLabel *m_lblNetworkSection;
    QLabel *m_lblNetworkHint;
    QLabel *m_lblNetworkSummary;
    QComboBox *m_comboNetworkPreset;
    QFrame *m_networkInfoFrame;

    QSpinBox *m_retrySpin;
    QSpinBox *m_timeoutSpin;
    QLabel *m_lblRetry;
    QLabel *m_lblTimeout;

    // 劫持页
    QWidget *m_tabHijack;

    QLabel *m_lblFromLang;
    QComboBox *m_comboFromLang;

    QLabel *m_lblToLang;
    QComboBox *m_comboToLang;

    QLabel *m_lblEndpoint;
    QComboBox *m_comboEndpoint;

    QCheckBox *m_chkTextGetter;

    QCheckBox *m_chkEnableImGui;
    QCheckBox *m_chkEnableUGui;
    QCheckBox *m_chkEnableUIElements;
    QCheckBox *m_chkEnableNGUI;
    QCheckBox *m_chkEnableTextMeshPro;
    QCheckBox *m_chkEnableTextMesh;
    QCheckBox *m_chkEnableFairyGUI;

    QDialogButtonBox *m_btnBox;
    QLabel *m_modernTitle;

    int m_lang;
    bool m_isDark;
    bool m_isClosing;

    bool m_isModern;
    int m_alpha;
    int m_renderMode;
    int m_hueShift;
    int m_tintIntensity;
    bool m_isRounded;

    QPoint m_dragPos;
    bool m_isDragging;
};

#endif // ADVANCEDSETTINGS_H