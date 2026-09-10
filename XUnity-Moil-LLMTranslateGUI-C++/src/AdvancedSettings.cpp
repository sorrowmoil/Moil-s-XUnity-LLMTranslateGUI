#include "AdvancedSettings.h"
#include "ConfigManager.h"
#include "ModernUI.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEasingCurve>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <QGraphicsOpacityEffect>

namespace
{
QString cleanLanguageCode(const QString &raw)
{
    QString value = raw.trimmed();
    int bracketPos = value.indexOf('(');
    if (bracketPos >= 0) value = value.left(bracketPos).trimmed();
    bracketPos = value.indexOf(QChar(0xFF08));
    if (bracketPos >= 0) value = value.left(bracketPos).trimmed();
    return value;
}

QString cleanEndpointValue(const QString &raw) { return raw.trimmed(); }

QString rgbaString(const QColor &color) { return QString("rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha()); }

class NativeMoveLabel final : public QLabel
{
public:
    explicit NativeMoveLabel(bool moveEnabled, QWidget *parent = nullptr) : QLabel(parent), m_moveEnabled(moveEnabled) {
        if (m_moveEnabled) setCursor(Qt::SizeAllCursor);
    }
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (m_moveEnabled && event->button() == Qt::LeftButton) {
            window()->winId();
            if (window()->windowHandle()->startSystemMove()) { event->accept(); return; }
        }
        QLabel::mousePressEvent(event);
    }
private:
    bool m_moveEnabled;
};
}

AdvancedSettingsDialog::AdvancedSettingsDialog(QWidget *parent, int lang, bool isDark, bool isModern, int alpha, int renderMode, int hueShift, int tintIntensity, bool isRounded)
    : QDialog(parent), m_lang(lang), m_isDark(isDark), m_isClosing(false), m_isModern(isModern), m_alpha(alpha), m_renderMode(renderMode), m_hueShift(hueShift), m_tintIntensity(tintIntensity), m_isRounded(isRounded), m_isDragging(false)
{
    setObjectName("AdvancedSettingsDialog");
    setSizeGripEnabled(false);

    if (m_isModern) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
    } else {
        setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        QPixmap transparentPixmap(1, 1);
        transparentPixmap.fill(Qt::transparent);
        setWindowIcon(QIcon(transparentPixmap));
    }

    const AppConfig cfg = ConfigManager::loadConfig();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, m_isModern ? 10 : 12, 14, 10);
    mainLayout->setSpacing(7);

    m_modernTitle = new NativeMoveLabel(m_isModern, this);
    m_modernTitle->setAlignment(Qt::AlignCenter);
    m_modernTitle->setFixedHeight(22);
    m_modernTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_modernTitle->setVisible(m_isModern);
    mainLayout->addWidget(m_modernTitle);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setObjectName("settingTabs");
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // =========================================================
    // Tab 1：网络参数
    // =========================================================
    m_tabNetwork = new QWidget(m_tabWidget);
    m_tabNetwork->setObjectName("networkPage");

    QVBoxLayout *networkLayout = new QVBoxLayout(m_tabNetwork);
    networkLayout->setContentsMargins(14, 9, 14, 9);
    networkLayout->setSpacing(7);

    m_lblNetworkSection = new QLabel(m_tabNetwork);
    m_lblNetworkSection->setObjectName("networkSectionTitle");
    m_lblNetworkSection->setFixedHeight(20);

    m_lblNetworkHint = new QLabel(m_tabNetwork);
    m_lblNetworkHint->setObjectName("networkSectionHint");
    m_lblNetworkHint->setWordWrap(true);
    m_lblNetworkHint->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_lblNetworkHint->setMinimumHeight(26);
    m_lblNetworkHint->setMaximumHeight(34);

    m_comboNetworkPreset = new QComboBox(m_tabNetwork);
    m_comboNetworkPreset->setObjectName("networkPresetCombo");
    m_comboNetworkPreset->setEditable(false);
    m_comboNetworkPreset->setFixedHeight(28);
    m_comboNetworkPreset->addItem("标准模式", "standard");
    m_comboNetworkPreset->addItem("保守模式", "safe");
    m_comboNetworkPreset->addItem("快速模式", "fast");
    m_comboNetworkPreset->addItem("自定义模式", "custom");

    QFrame *networkParamFrame = new QFrame(m_tabNetwork);
    networkParamFrame->setObjectName("networkParamFrame");
    networkParamFrame->setFixedHeight(86);
    networkParamFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QGridLayout *networkGrid = new QGridLayout(networkParamFrame);
    networkGrid->setContentsMargins(10, 7, 10, 7);
    networkGrid->setHorizontalSpacing(10);
    networkGrid->setVerticalSpacing(6);
    networkGrid->setColumnStretch(0, 0);
    networkGrid->setColumnStretch(1, 1);

    m_lblRetry = new QLabel(networkParamFrame);
    m_lblRetry->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_retrySpin = new QSpinBox(networkParamFrame);
    m_retrySpin->setRange(0, 9999);
    m_retrySpin->setValue(cfg.max_retries);
    m_retrySpin->setAlignment(Qt::AlignCenter);
    m_retrySpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_retrySpin->setFixedHeight(28);

    m_lblTimeout = new QLabel(networkParamFrame);
    m_lblTimeout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_timeoutSpin = new QSpinBox(networkParamFrame);
    m_timeoutSpin->setRange(1, 9999);
    m_timeoutSpin->setValue(qMax(1, cfg.timeout_ms / 1000));
    m_timeoutSpin->setAlignment(Qt::AlignCenter);
    m_timeoutSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_timeoutSpin->setFixedHeight(28);

    networkGrid->addWidget(m_lblRetry, 0, 0);
    networkGrid->addWidget(m_retrySpin, 0, 1);
    networkGrid->addWidget(m_lblTimeout, 1, 0);
    networkGrid->addWidget(m_timeoutSpin, 1, 1);

    m_networkInfoFrame = new QFrame(m_tabNetwork);
    m_networkInfoFrame->setObjectName("networkInfoFrame");
    m_networkInfoFrame->setMinimumHeight(58);
    m_networkInfoFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *networkInfoLayout = new QVBoxLayout(m_networkInfoFrame);
    networkInfoLayout->setContentsMargins(10, 8, 10, 8);
    networkInfoLayout->setSpacing(0);

    m_lblNetworkSummary = new QLabel(m_networkInfoFrame);
    m_lblNetworkSummary->setObjectName("networkSummary");
    m_lblNetworkSummary->setWordWrap(true);
    m_lblNetworkSummary->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    networkInfoLayout->addWidget(m_lblNetworkSummary);

    networkLayout->addWidget(m_lblNetworkSection);
    networkLayout->addWidget(m_lblNetworkHint);
    networkLayout->addWidget(m_comboNetworkPreset);
    networkLayout->addWidget(networkParamFrame);
    networkLayout->addWidget(m_networkInfoFrame, 1);

    m_tabWidget->addTab(m_tabNetwork, lang == 1 ? "网络参数" : "Network");

    // =========================================================
    // Tab 2：配置劫持
    // =========================================================
    m_tabHijack = new QWidget(m_tabWidget);
    m_tabHijack->setObjectName("hijackPage");

    QVBoxLayout *hijackPageLayout = new QVBoxLayout(m_tabHijack);
    hijackPageLayout->setContentsMargins(14, 7, 14, 7);
    hijackPageLayout->setSpacing(7);
    hijackPageLayout->setAlignment(Qt::AlignTop);

    // ---------------------------------------------------------
    // 语言和端点
    // ---------------------------------------------------------
    QFrame *hijackParamFrame = new QFrame(m_tabHijack);
    hijackParamFrame->setObjectName("hijackParamFrame");
    hijackParamFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QGridLayout *hijackParamGrid = new QGridLayout(hijackParamFrame);
    hijackParamGrid->setContentsMargins(9, 6, 9, 6);
    hijackParamGrid->setHorizontalSpacing(10);
    hijackParamGrid->setVerticalSpacing(5);
    hijackParamGrid->setColumnStretch(0, 0);
    hijackParamGrid->setColumnStretch(1, 1);

    constexpr int labelWidth = 124;

    m_lblFromLang = new QLabel(hijackParamFrame);
    m_lblToLang = new QLabel(hijackParamFrame);
    m_lblEndpoint = new QLabel(hijackParamFrame);

    for (QLabel *label : {m_lblFromLang, m_lblToLang, m_lblEndpoint}) {
        label->setFixedWidth(labelWidth);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    m_comboFromLang = new QComboBox(hijackParamFrame);
    m_comboFromLang->setEditable(true);
    m_comboFromLang->setInsertPolicy(QComboBox::NoInsert);
    m_comboFromLang->setFixedHeight(28);
    m_comboFromLang->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_comboFromLang->addItem("ja (Japanese)", "ja");
    m_comboFromLang->addItem("en (English)", "en");
    m_comboFromLang->addItem("auto (Auto Detect)", "auto");
    setComboValue(m_comboFromLang, cfg.hijack_from_lang);

    m_comboToLang = new QComboBox(hijackParamFrame);
    m_comboToLang->setEditable(true);
    m_comboToLang->setInsertPolicy(QComboBox::NoInsert);
    m_comboToLang->setFixedHeight(28);
    m_comboToLang->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_comboToLang->addItem("zh (Simplified Chinese)", "zh");
    m_comboToLang->addItem("zh-TW (Traditional Chinese)", "zh-TW");
    m_comboToLang->addItem("en (English)", "en");
    m_comboToLang->addItem("ja (Japanese)", "ja");
    setComboValue(m_comboToLang, cfg.hijack_to_lang);

    m_comboEndpoint = new QComboBox(hijackParamFrame);
    m_comboEndpoint->setEditable(true);
    m_comboEndpoint->setInsertPolicy(QComboBox::NoInsert);
    m_comboEndpoint->setFixedHeight(28);
    m_comboEndpoint->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_comboEndpoint->addItem("GoogleTranslate", "GoogleTranslate");
    m_comboEndpoint->addItem("CustomTranslate", "CustomTranslate");
    setComboValue(m_comboEndpoint, cfg.hijack_endpoint);

    hijackParamGrid->addWidget(m_lblFromLang, 0, 0);
    hijackParamGrid->addWidget(m_comboFromLang, 0, 1);
    hijackParamGrid->addWidget(m_lblToLang, 1, 0);
    hijackParamGrid->addWidget(m_comboToLang, 1, 1);
    hijackParamGrid->addWidget(m_lblEndpoint, 2, 0);
    hijackParamGrid->addWidget(m_comboEndpoint, 2, 1);

    hijackPageLayout->addWidget(hijackParamFrame);

    // ---------------------------------------------------------
    // TextGetterCompatibilityMode
    // ---------------------------------------------------------
    QFrame *compatibilityFrame = new QFrame(m_tabHijack);
    compatibilityFrame->setObjectName("compatibilityFrame");
    compatibilityFrame->setFixedHeight(28);
    compatibilityFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *compatibilityLayout = new QHBoxLayout(compatibilityFrame);
    compatibilityLayout->setContentsMargins(9, 3, 9, 3);
    compatibilityLayout->setSpacing(0);

    m_chkTextGetter = new QCheckBox(compatibilityFrame);
    m_chkTextGetter->setChecked(cfg.hijack_text_getter);

    compatibilityLayout->addWidget(m_chkTextGetter);
    compatibilityLayout->addStretch(1);

    hijackPageLayout->addWidget(compatibilityFrame);

    // ---------------------------------------------------------
    // [TextFrameworks]
    // ---------------------------------------------------------
    QFrame *frameworkFrame = new QFrame(m_tabHijack);
    frameworkFrame->setObjectName("frameworkFrame");
    frameworkFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *frameworkLayout = new QVBoxLayout(frameworkFrame);
    frameworkLayout->setContentsMargins(9, 6, 9, 6);
    frameworkLayout->setSpacing(4);

    QLabel *frameworkTitle = new QLabel(frameworkFrame);
    frameworkTitle->setObjectName("frameworkTitle");
    frameworkTitle->setFixedHeight(18);
    frameworkLayout->addWidget(frameworkTitle);

    QGridLayout *frameworkGrid = new QGridLayout();
    frameworkGrid->setContentsMargins(0, 0, 0, 0);
    frameworkGrid->setHorizontalSpacing(22);
    frameworkGrid->setVerticalSpacing(3);
    frameworkGrid->setColumnStretch(0, 1);
    frameworkGrid->setColumnStretch(1, 1);

    m_chkEnableImGui = new QCheckBox(frameworkFrame);
    m_chkEnableUGui = new QCheckBox(frameworkFrame);
    m_chkEnableUIElements = new QCheckBox(frameworkFrame);
    m_chkEnableNGUI = new QCheckBox(frameworkFrame);
    m_chkEnableTextMeshPro = new QCheckBox(frameworkFrame);
    m_chkEnableTextMesh = new QCheckBox(frameworkFrame);
    m_chkEnableFairyGUI = new QCheckBox(frameworkFrame);

    m_chkEnableImGui->setChecked(cfg.hijack_enable_imgui);
    m_chkEnableUGui->setChecked(cfg.hijack_enable_ugui);
    m_chkEnableUIElements->setChecked(cfg.hijack_enable_ui_elements);
    m_chkEnableNGUI->setChecked(cfg.hijack_enable_ngui);
    m_chkEnableTextMeshPro->setChecked(cfg.hijack_enable_text_mesh_pro);
    m_chkEnableTextMesh->setChecked(cfg.hijack_enable_text_mesh);
    m_chkEnableFairyGUI->setChecked(cfg.hijack_enable_fairy_gui);

    for (QCheckBox *checkBox : {m_chkEnableUGui, m_chkEnableUIElements, m_chkEnableNGUI, m_chkEnableTextMeshPro, m_chkEnableTextMesh, m_chkEnableFairyGUI, m_chkEnableImGui}) {
        checkBox->setFixedHeight(18);
        checkBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    frameworkGrid->addWidget(m_chkEnableUGui, 0, 0);
    frameworkGrid->addWidget(m_chkEnableUIElements, 0, 1);
    frameworkGrid->addWidget(m_chkEnableNGUI, 1, 0);
    frameworkGrid->addWidget(m_chkEnableTextMeshPro, 1, 1);
    frameworkGrid->addWidget(m_chkEnableTextMesh, 2, 0);
    frameworkGrid->addWidget(m_chkEnableFairyGUI, 2, 1);
    frameworkGrid->addWidget(m_chkEnableImGui, 3, 0);

    frameworkLayout->addLayout(frameworkGrid);
    hijackPageLayout->addWidget(frameworkFrame);
    hijackPageLayout->addStretch(1);

    m_tabWidget->addTab(m_tabHijack, lang == 1 ? "配置劫持" : "Hijack");

    mainLayout->addWidget(m_tabWidget, 1);

    // =========================================================
    // 按钮
    // =========================================================
    m_btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_btnBox, &QDialogButtonBox::accepted, this, &AdvancedSettingsDialog::accept);
    connect(m_btnBox, &QDialogButtonBox::rejected, this, &AdvancedSettingsDialog::reject);
    mainLayout->addWidget(m_btnBox);

    // 初始化时仅识别当前值，不应用预设。
    {
        QSignalBlocker blocker(m_comboNetworkPreset);
        QString preset = "custom";
        const int retries = m_retrySpin->value();
        const int timeout = m_timeoutSpin->value();
        if (retries == 20 && timeout == 10) preset = "standard";
        else if (retries == 30 && timeout == 15) preset = "safe";
        else if (retries == 3 && timeout == 5) preset = "fast";
        m_comboNetworkPreset->setCurrentIndex(m_comboNetworkPreset->findData(preset));
    }

    connect(m_comboNetworkPreset, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (index >= 0) applyNetworkPreset(m_comboNetworkPreset->itemData(index).toString());
    });

    connect(m_retrySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
        updateNetworkPresetFromValues();
        updateNetworkSummary();
    });

    connect(m_timeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
        updateNetworkPresetFromValues();
        updateNetworkSummary();
    });

    updateLanguage(m_lang);
    updateTheme(m_isDark);
    updateNetworkSummary();

    m_tabWidget->setCurrentIndex(0);
    mainLayout->activate();
    const QSize initialSize = m_isModern ? QSize(400, 410) : QSize(380, 390);
    setMinimumSize(initialSize);
    resize(initialSize);
}

int AdvancedSettingsDialog::getRetries() const { return m_retrySpin->value(); }
int AdvancedSettingsDialog::getTimeoutMs() const { return m_timeoutSpin->value() * 1000; }
QString AdvancedSettingsDialog::getHijackFromLang() const { return cleanLanguageCode(m_comboFromLang->currentText()); }
QString AdvancedSettingsDialog::getHijackToLang() const { return cleanLanguageCode(m_comboToLang->currentText()); }
QString AdvancedSettingsDialog::getHijackEndpoint() const { return cleanEndpointValue(m_comboEndpoint->currentText()); }
bool AdvancedSettingsDialog::getHijackTextGetter() const { return m_chkTextGetter->isChecked(); }
bool AdvancedSettingsDialog::getHijackEnableImGui() const { return m_chkEnableImGui->isChecked(); }
bool AdvancedSettingsDialog::getHijackEnableUGui() const { return m_chkEnableUGui->isChecked(); }
bool AdvancedSettingsDialog::getHijackEnableUIElements() const { return m_chkEnableUIElements->isChecked(); }
bool AdvancedSettingsDialog::getHijackEnableNGUI() const { return m_chkEnableNGUI->isChecked(); }
bool AdvancedSettingsDialog::getHijackEnableTextMeshPro() const { return m_chkEnableTextMeshPro->isChecked(); }
bool AdvancedSettingsDialog::getHijackEnableTextMesh() const { return m_chkEnableTextMesh->isChecked(); }
bool AdvancedSettingsDialog::getHijackEnableFairyGUI() const { return m_chkEnableFairyGUI->isChecked(); }

void AdvancedSettingsDialog::setComboValue(QComboBox *combo, const QString &value)
{
    QString code = value.trimmed();
    if (combo == m_comboFromLang || combo == m_comboToLang) code = cleanLanguageCode(code);
    else code = cleanEndpointValue(code);
    const int index = combo->findData(code);
    if (index >= 0) { combo->setCurrentIndex(index); combo->lineEdit()->setText(code); }
    else combo->setEditText(code);
}

void AdvancedSettingsDialog::applyNetworkPreset(const QString &preset)
{
    if (preset == "custom") { updateNetworkSummary(); return; }
    QSignalBlocker retryBlocker(m_retrySpin), timeoutBlocker(m_timeoutSpin);
    if (preset == "standard") { m_retrySpin->setValue(20); m_timeoutSpin->setValue(10); }
    else if (preset == "safe") { m_retrySpin->setValue(30); m_timeoutSpin->setValue(15); }
    else if (preset == "fast") { m_retrySpin->setValue(3); m_timeoutSpin->setValue(5); }
    updateNetworkSummary();
}

void AdvancedSettingsDialog::updateNetworkPresetFromValues()
{
    const int retries = m_retrySpin->value(), timeout = m_timeoutSpin->value();
    QString preset = "custom";
    if (retries == 20 && timeout == 10) preset = "standard";
    else if (retries == 30 && timeout == 15) preset = "safe";
    else if (retries == 3 && timeout == 5) preset = "fast";
    const int index = m_comboNetworkPreset->findData(preset);
    if (index >= 0 && index != m_comboNetworkPreset->currentIndex()) {
        QSignalBlocker blocker(m_comboNetworkPreset);
        m_comboNetworkPreset->setCurrentIndex(index);
    }
}

void AdvancedSettingsDialog::updateNetworkSummary()
{
    const int retries = m_retrySpin->value(), timeout = m_timeoutSpin->value();
    const QString preset = m_comboNetworkPreset->currentData().toString();
    QString modeName;
    if (m_lang == 1) {
        if (preset == "standard") modeName = "标准模式";
        else if (preset == "safe") modeName = "保守模式";
        else if (preset == "fast") modeName = "快速模式";
        else modeName = "自定义模式";
        m_lblNetworkSummary->setText(QString("%1：每次请求最多重试 %2 次，单次等待 %3 秒。\n请求失败或返回格式异常时，程序将按照该策略重新请求。\n网络不稳定时可适当提高超时和重试次数。").arg(modeName).arg(retries).arg(timeout));
    } else {
        if (preset == "standard") modeName = "Standard";
        else if (preset == "safe") modeName = "Safe";
        else if (preset == "fast") modeName = "Fast";
        else modeName = "Custom";
        m_lblNetworkSummary->setText(QString("%1: retry up to %2 time(s), with a %3-second timeout.\nFailed requests or invalid responses will use this retry policy.\nIncrease timeout and retries when the network is unstable.").arg(modeName).arg(retries).arg(timeout));
    }
}

void AdvancedSettingsDialog::updateGlassEnv(bool isDark, int alpha, bool isRounded, int renderMode, int hueShift, int tintIntensity)
{
    m_isDark = isDark; m_alpha = alpha; m_isRounded = isRounded; m_renderMode = renderMode; m_hueShift = hueShift; m_tintIntensity = tintIntensity;
    updateTheme(m_isDark);
    update();
}

// ==========================================
// 🌟 平滑淡入淡出过渡实现
// ==========================================
void AdvancedSettingsDialog::smoothSwitch(std::function<void()> changeLogic)
{
    if (!isVisible()) {
        changeLogic();
        return;
    }
    QPixmap pixmap = this->grab();
    QLabel *overlay = new QLabel(this);
    overlay->setPixmap(pixmap);
    overlay->setGeometry(0, 0, this->width(), this->height());
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->show();
    overlay->raise();
    changeLogic();
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(overlay);
    overlay->setGraphicsEffect(effect);
    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(250);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->setEasingCurve(QEasingCurve::OutQuad);
    connect(anim, &QPropertyAnimation::finished, overlay, &QLabel::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}
// ==========================================
// 🌍 语言更新 (带平滑过渡)
// ==========================================
void AdvancedSettingsDialog::updateLanguage(int lang)
{
    if (m_lang == lang && isVisible()) return;
    auto applyLang = [this, lang]() {
        m_lang = lang;
        const QString title = lang == 1 ? QStringLiteral("⚙️ 高级设置") : QStringLiteral("⚙️ Advanced Settings");
        setWindowTitle(title);
        m_modernTitle->setText(title);
        m_tabWidget->setTabText(0, lang == 1 ? QStringLiteral("网络参数") : QStringLiteral("Network"));
        m_tabWidget->setTabText(1, lang == 1 ? QStringLiteral("配置劫持") : QStringLiteral("Hijack"));
        m_lblNetworkSection->setText(lang == 1 ? QStringLiteral("请求策略") : QStringLiteral("Request Policy"));
        m_lblNetworkHint->setText(lang == 1 ? QStringLiteral("网络异常或返回格式错误时，程序会按照下面的策略重新请求。") : QStringLiteral("When a request fails, the translator retries according to the policy below."));
        m_comboNetworkPreset->setItemText(0, lang == 1 ? QStringLiteral("标准模式") : QStringLiteral("Standard"));
        m_comboNetworkPreset->setItemText(1, lang == 1 ? QStringLiteral("保守模式") : QStringLiteral("Safe"));
        m_comboNetworkPreset->setItemText(2, lang == 1 ? QStringLiteral("快速模式") : QStringLiteral("Fast"));
        m_comboNetworkPreset->setItemText(3, lang == 1 ? QStringLiteral("自定义模式") : QStringLiteral("Custom"));
        m_lblRetry->setText(lang == 1 ? QStringLiteral("🔄 翻译重试次数:") : QStringLiteral("🔄 Max Retries:"));
        m_lblTimeout->setText(lang == 1 ? QStringLiteral("⏱️ 网络超时时间:") : QStringLiteral("⏱️ Timeout:"));
        m_timeoutSpin->setSuffix(lang == 1 ? QStringLiteral(" 秒") : QStringLiteral(" s"));
        m_lblFromLang->setText(lang == 1 ? QStringLiteral("游戏源语言:") : QStringLiteral("Game Source:"));
        m_lblToLang->setText(lang == 1 ? QStringLiteral("目标翻译语言:") : QStringLiteral("Target Language:"));
        m_lblEndpoint->setText(lang == 1 ? QStringLiteral("翻译端点:") : QStringLiteral("Translation Endpoint:"));
        m_chkTextGetter->setText(lang == 1 ? QStringLiteral("启用文本获取兼容模式") : QStringLiteral("Text Getter Compatibility Mode"));
        m_chkEnableImGui->setText("IMGUI");
        m_chkEnableUGui->setText("UGUI");
        m_chkEnableUIElements->setText("UIElements");
        m_chkEnableNGUI->setText("NGUI");
        m_chkEnableTextMeshPro->setText("TextMeshPro");
        m_chkEnableTextMesh->setText("TextMesh");
        m_chkEnableFairyGUI->setText("FairyGUI");
        if (auto *lblFramework = m_tabHijack->findChild<QLabel *>("frameworkTitle")) {
            lblFramework->setText(lang == 1 ? QStringLiteral("文本框架") : QStringLiteral("Text Frameworks"));
        }
        if (m_btnBox->button(QDialogButtonBox::Ok))
            m_btnBox->button(QDialogButtonBox::Ok)->setText(lang == 1 ? QStringLiteral("确定") : QStringLiteral("OK"));
        if (m_btnBox->button(QDialogButtonBox::Cancel))
            m_btnBox->button(QDialogButtonBox::Cancel)->setText(lang == 1 ? QStringLiteral("取消") : QStringLiteral("Cancel"));
        const QString fromTip = lang == 1 ? QStringLiteral("写入 [General] FromLanguage。\n例如：ja、en、auto。") : QStringLiteral("Writes [General] FromLanguage.\nExamples: ja, en, auto.");
        const QString toTip = lang == 1 ? QStringLiteral("写入 [General] Language。\n简体中文使用 zh。") : QStringLiteral("Writes [General] Language.\nUse zh for Simplified Chinese.");
        const QString endpointTip = lang == 1 ? QStringLiteral("写入 [Service] Endpoint。\n常用值：GoogleTranslate 或 CustomTranslate。") : QStringLiteral("Writes [Service] Endpoint.\nCommon values: GoogleTranslate or CustomTranslate.");
        m_comboFromLang->setToolTip(fromTip);
        m_comboToLang->setToolTip(toTip);
        m_comboEndpoint->setToolTip(endpointTip);
        m_lblFromLang->setToolTip(fromTip);
        m_lblToLang->setToolTip(toTip);
        m_lblEndpoint->setToolTip(endpointTip);
        m_comboNetworkPreset->setToolTip(lang == 1 ? QStringLiteral("选择预设会立即应用对应参数；手动修改参数后会自动切换为自定义模式。") : QStringLiteral("Selecting a preset applies its values immediately. Manual changes switch to Custom."));
        m_chkTextGetter->setToolTip(lang == 1 ? QStringLiteral("写入 [Behaviour] TextGetterCompatibilityMode。") : QStringLiteral("Writes [Behaviour] TextGetterCompatibilityMode."));
        auto setFrameworkTip = [lang](QCheckBox *checkBox, const QString &key) {
            checkBox->setToolTip(lang == 1 ? QString("写入 [TextFrameworks] %1。").arg(key) : QString("Writes [TextFrameworks] %1.").arg(key));
        };
        setFrameworkTip(m_chkEnableImGui, "EnableIMGUI");
        setFrameworkTip(m_chkEnableUGui, "EnableUGUI");
        setFrameworkTip(m_chkEnableUIElements, "EnableUIElements");
        setFrameworkTip(m_chkEnableNGUI, "EnableNGUI");
        setFrameworkTip(m_chkEnableTextMeshPro, "EnableTextMeshPro");
        setFrameworkTip(m_chkEnableTextMesh, "EnableTextMesh");
        setFrameworkTip(m_chkEnableFairyGUI, "EnableFairyGUI");
        updateNetworkSummary();
    };
    if (isVisible()) {
        smoothSwitch(applyLang);
    } else {
        applyLang();
    }
}

void AdvancedSettingsDialog::updateTheme(bool isDark)
{
    m_isDark = isDark;

    if (m_isModern)
    {
        const QString textColor = isDark ? "#FFFFFF" : "#111111";
        const QString secondaryText = isDark ? "rgba(255,255,255,175)" : "rgba(0,0,0,135)";
        const QString paneBg = isDark ? "rgba(0,0,0,80)" : "rgba(255,255,255,160)";
        const QString inputBg = isDark ? "rgba(0,0,0,110)" : "rgba(255,255,255,210)";
        const QString border = isDark ? "rgba(255,255,255,25)" : "rgba(0,0,0,30)";
        const QString buttonBg = isDark ? "rgba(255,255,255,12)" : "rgba(255,255,255,220)";
        const QString radius = m_isRounded ? "6" : "0";

        QColor accentColor = isDark ? QColor(255, 140, 0) : QColor(148, 0, 211);
        accentColor = shiftHue(accentColor, m_hueShift);

        QColor hoverColor = accentColor;
        hoverColor.setAlpha(isDark ? 42 : 28);

        const QString accent = accentColor.name(QColor::HexRgb);
        const QString hover = rgbaString(hoverColor);

        // QSS 每个选择器单独成行
        QString qss = QString(
            "QDialog#AdvancedSettingsDialog { background: transparent; }\n"
            "QLabel { color: %TEXT%; font-size: 12px; font-weight: bold; }\n"
            "QWidget#networkPage, QWidget#hijackPage { background: transparent; border: none; }\n"
            "QTabWidget::pane { background: %PANE%; border: 1px solid %BORDER%; border-radius: %RADIUS%px; }\n"
            "QTabBar::tab { padding: 6px 14px; margin-right: 2px; color: %TEXT%; background: transparent; border-bottom: 2px solid transparent; font-weight: bold; }\n"
            "QTabBar::tab:hover { background: rgba(128,128,128,50); }\n"
            "QTabBar::tab:selected { color: %ACCENT%; background: %HOVER%; border-bottom: 2px solid %ACCENT%; }\n"
            "QSpinBox, QComboBox { padding: 4px; color: %TEXT%; background: %INPUT%; border: 1px solid %BORDER%; border-radius: 4px; font-weight: bold; }\n"
            "QSpinBox:hover, QComboBox:hover, QSpinBox:focus, QComboBox:focus { border-color: %ACCENT%; }\n"
            "QComboBox QLineEdit { padding: 0 4px; color: %TEXT%; background: transparent; border: none; }\n"
            "QComboBox::drop-down { width: 20px; background: transparent; border: none; }\n"
            "QComboBox::drop-down:hover { background: %HOVER%; }\n"
            "QComboBox::down-arrow { image: none; width: 8px; height: 2px; background: %ACCENT%; }\n"
            "QComboBox QAbstractItemView { color: %TEXT%; background: %PANE%; border: 1px solid %ACCENT%; selection-background-color: %HOVER%; outline: none; }\n"
            "QCheckBox { color: %TEXT%; spacing: 6px; font-weight: bold; }\n"
            "QCheckBox::indicator { width: 14px; height: 14px; background: %INPUT%; border: 1px solid %BORDER%; border-radius: 3px; }\n"
            "QCheckBox::indicator:checked { background: %ACCENT%; border-color: %ACCENT%; }\n"
            "QPushButton { padding: 6px 20px; color: %TEXT%; background: %BUTTON%; border: 1px solid %BORDER%; border-radius: 4px; font-weight: bold; }\n"
            "QPushButton:hover { background: %HOVER%; border-color: %ACCENT%; }\n"
            "QLabel#networkSectionTitle { color: %ACCENT%; font-size: 13px; }\n"
            "QLabel#networkSectionHint { color: %SECONDARY%; font-size: 10px; font-weight: normal; }\n"
            "QLabel#networkSummary { color: %TEXT%; font-size: 11px; font-weight: normal; }\n"
            "QFrame#networkParamFrame, QFrame#hijackParamFrame, QFrame#frameworkFrame { background: %PANE%; border: 1px solid %BORDER%; border-radius: %RADIUS%px; }\n"
            "QFrame#compatibilityFrame { background: transparent; border: none; }\n"
            "QFrame#networkInfoFrame { background: %HOVER%; border: none; border-left: 3px solid %ACCENT%; border-radius: 4px; }\n"
            "QLabel#frameworkTitle { color: %ACCENT%; font-size: 11px; font-weight: bold; }\n"
            "QToolTip { padding: 6px; color: %TEXT%; background: %PANE%; border: 1px solid %ACCENT%; border-radius: 4px; }"
        );
        qss.replace("%SECONDARY%", secondaryText).replace("%BUTTON%", buttonBg).replace("%BORDER%", border)
           .replace("%ACCENT%", accent).replace("%RADIUS%", radius).replace("%HOVER%", hover)
           .replace("%INPUT%", inputBg).replace("%PANE%", paneBg).replace("%TEXT%", textColor);
        setStyleSheet(qss);
        m_modernTitle->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(accent));
        return;
    }

    // =========================================================
    // 经典模式：暗色赤金，亮色蓝色
    // =========================================================
    const QString bgColor = isDark ? "#1E1E1E" : "#F8F9FA";
    const QString paneBg = isDark ? "#252526" : "#FFFFFF";
    const QString inputBg = isDark ? "#2D2D30" : "#FFFFFF";
    const QString buttonBg = isDark ? "#363636" : "#E1E1E1";
    const QString textColor = isDark ? "#EAEAEA" : "#333333";
    const QString secondaryText = isDark ? "#AAA69C" : "#666666";
    const QString border = isDark ? "#554D37" : "#CCCCCC";
    const QString accent = isDark ? "#E6B422" : "#0078D4";
    const QString accentHover = isDark ? "#FFD45C" : "#005A9E";
    const QString accentPressed = isDark ? "#B88916" : "#004578";
    const QString hover = isDark ? "#3A321F" : "#E5F1FB";
    const QString onAccent = isDark ? "#191919" : "#FFFFFF";

    QString qss = QString(
        "QDialog#AdvancedSettingsDialog { color: %TEXT%; background: %BG%; }\n"
        "QLabel { color: %TEXT%; font-size: 12px; }\n"
        "QWidget#networkPage, QWidget#hijackPage { background: transparent; border: none; }\n"
        "QTabWidget::pane { background: %PANE%; border: 1px solid %BORDER%; border-radius: 4px; }\n"
        "QTabBar::tab { min-width: 64px; padding: 6px 16px; color: %TEXT%; background: %BG%; border: 1px solid transparent; border-bottom: none; }\n"
        "QTabBar::tab:hover { color: %ACCENT_HOVER%; background: %HOVER%; }\n"
        "QTabBar::tab:selected { color: %ACCENT%; background: %PANE%; border: 1px solid %BORDER%; border-bottom: 1px solid %PANE%; font-weight: bold; }\n"
        "QSpinBox, QComboBox { padding: 4px; color: %TEXT%; background: %INPUT%; border: 1px solid %BORDER%; border-radius: 3px; selection-background-color: %ACCENT%; selection-color: %ON_ACCENT%; }\n"
        "QSpinBox:hover, QComboBox:hover, QSpinBox:focus, QComboBox:focus { border-color: %ACCENT%; }\n"
        "QComboBox QLineEdit { padding: 0 4px; color: %TEXT%; background: transparent; border: none; selection-background-color: %ACCENT%; selection-color: %ON_ACCENT%; }\n"
        "QComboBox::drop-down { width: 20px; background: transparent; border: none; }\n"
        "QComboBox::drop-down:hover { background: %HOVER%; }\n"
        "QComboBox::down-arrow { image: none; width: 8px; height: 2px; background: %ACCENT%; }\n"
        "QComboBox QAbstractItemView { color: %TEXT%; background: %PANE%; border: 1px solid %ACCENT%; selection-color: %ACCENT_HOVER%; selection-background-color: %HOVER%; outline: none; }\n"
        "QCheckBox { color: %TEXT%; spacing: 6px; }\n"
        "QCheckBox:hover { color: %ACCENT_HOVER%; }\n"
        "QPushButton { padding: 5px 15px; color: %TEXT%; background: %BUTTON%; border: 1px solid %BORDER%; border-radius: 3px; font-weight: bold; }\n"
        "QPushButton:hover { color: %ACCENT_HOVER%; background: %HOVER%; border-color: %ACCENT%; }\n"
        "QPushButton:pressed { color: %ON_ACCENT%; background: %ACCENT_PRESSED%; border-color: %ACCENT_PRESSED%; }\n"
        "QLabel#networkSectionTitle { color: %ACCENT%; font-size: 13px; font-weight: bold; }\n"
        "QLabel#networkSectionHint { color: %SECONDARY%; font-size: 10px; }\n"
        "QLabel#networkSummary { color: %TEXT%; font-size: 11px; }\n"
        "QFrame#networkParamFrame, QFrame#hijackParamFrame, QFrame#frameworkFrame { background: %PANE%; border: 1px solid %BORDER%; border-radius: 4px; }\n"
        "QFrame#compatibilityFrame { background: transparent; border: none; }\n"
        "QFrame#networkInfoFrame { background: %HOVER%; border: none; border-left: 3px solid %ACCENT%; border-radius: 4px; }\n"
        "QLabel#frameworkTitle { color: %ACCENT%; font-weight: bold; }\n"
        "QToolTip { padding: 6px; color: %ACCENT%; background: %PANE%; border: 1px solid %ACCENT%; border-radius: 4px; }"
    );
    qss.replace("%ACCENT_PRESSED%", accentPressed).replace("%ACCENT_HOVER%", accentHover)
       .replace("%SECONDARY%", secondaryText).replace("%ON_ACCENT%", onAccent)
       .replace("%BUTTON%", buttonBg).replace("%BORDER%", border).replace("%ACCENT%", accent)
       .replace("%HOVER%", hover).replace("%INPUT%", inputBg).replace("%PANE%", paneBg)
       .replace("%TEXT%", textColor).replace("%BG%", bgColor);
    setStyleSheet(qss);
}

void AdvancedSettingsDialog::paintEvent(QPaintEvent *event)
{
    if (m_isModern) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const int radius = m_isRounded ? 8 : 0;
        if (m_renderMode == 0) drawMenuGlassEffect(painter, rect(), m_isDark, m_alpha, radius, m_hueShift, m_tintIntensity);
        else drawLegacyGlowEffect(painter, rect(), m_isDark, m_alpha, radius, 1.0f);
        QColor borderColor = m_isDark ? QColor(255, 140, 0, 120) : QColor(148, 0, 211, 120);
        borderColor = shiftHue(borderColor, m_hueShift);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(borderColor, 1));
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), radius, radius);
    }
    QDialog::paintEvent(event);
}

void AdvancedSettingsDialog::mousePressEvent(QMouseEvent *event) { QDialog::mousePressEvent(event); }
void AdvancedSettingsDialog::mouseMoveEvent(QMouseEvent *event) { QDialog::mouseMoveEvent(event); }
void AdvancedSettingsDialog::mouseReleaseEvent(QMouseEvent *event) { m_isDragging = false; QDialog::mouseReleaseEvent(event); }

void AdvancedSettingsDialog::closeEvent(QCloseEvent *event)
{
    if (m_isClosing) { event->accept(); return; }
    event->ignore();
    reject();
}

void AdvancedSettingsDialog::accept()
{
    AppConfig config = ConfigManager::loadConfig();
    config.max_retries = getRetries();
    config.timeout_ms = getTimeoutMs();
    config.hijack_from_lang = getHijackFromLang();
    config.hijack_to_lang = getHijackToLang();
    config.hijack_endpoint = getHijackEndpoint();
    config.hijack_text_getter = getHijackTextGetter();
    config.hijack_enable_imgui = getHijackEnableImGui();
    config.hijack_enable_ugui = getHijackEnableUGui();
    config.hijack_enable_ui_elements = getHijackEnableUIElements();
    config.hijack_enable_ngui = getHijackEnableNGUI();
    config.hijack_enable_text_mesh_pro = getHijackEnableTextMeshPro();
    config.hijack_enable_text_mesh = getHijackEnableTextMesh();
    config.hijack_enable_fairy_gui = getHijackEnableFairyGUI();

    if (config.hijack_from_lang.isEmpty()) config.hijack_from_lang = "ja";
    if (config.hijack_to_lang.isEmpty()) config.hijack_to_lang = "zh";
    if (config.hijack_endpoint.isEmpty()) config.hijack_endpoint = "GoogleTranslate";

    ConfigManager::saveConfig(config);

    if (m_isClosing) { QDialog::accept(); return; }
    m_isClosing = true;

    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(250);
    animation->setStartValue(windowOpacity());
    animation->setEndValue(0.0);
    animation->setEasingCurve(QEasingCurve::InCubic);
    connect(animation, &QPropertyAnimation::finished, this, [this]() { QDialog::accept(); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void AdvancedSettingsDialog::reject()
{
    if (m_isClosing) { QDialog::reject(); return; }
    m_isClosing = true;

    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(250);
    animation->setStartValue(windowOpacity());
    animation->setEndValue(0.0);
    animation->setEasingCurve(QEasingCurve::InCubic);
    connect(animation, &QPropertyAnimation::finished, this, [this]() { QDialog::reject(); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}