/**
 * ModernWindow.cpp - 防移位 & 完美对齐版本
 * ModernWindow.cpp - Anti-Shift & Perfect Alignment Version
 *
 * Developed by CAN.
 *
 * 修复内容 | FIXES:
 * 1. 修复布局移位：锁定列宽并同步 CSS 指标 | Fixed Layout Shifting: Locked column widths and synchronized CSS metrics.
 * 2. 视觉稳定性：确保标签和输入框在主题切换时不抖动 | Visual Stability: Ensured labels and inputs don't jitter during theme swap.
 * 3. 最 vexing 解析修复：为 QNetworkRequest 使用显式初始化 | Most Vexing Parse Fix: Used explicit initialization for QNetworkRequest.
 * 4. 平衡布局：控制按钮 1:1:1 比例，高度 38px | Balanced Layout: 1:1:1 ratio for control buttons with 38px height.
 * 5. 调整窗口大小：默认大小调整为 500x832 | Resized Window: Adjusted default size to 500x832.
 */

#include "ModernWindow.h"
#include "XuaConfigHijacker.h"
#include <QPainterPath>
#include <QApplication>
#include <QFileDialog>
#include <QScrollBar>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QTextStream>
#include <QPropertyAnimation>
#include <QMenu>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QTextCursor>
#include <QTextDocument>
#include <QSizeGrip>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include "ConfigManager.h"
#include "json.hpp"
#include "LogManager.h"
#include <functional>              // 必须用于 std::function | Required for std::function
#include <QLabel>                  // 用于截图覆盖层 | Used for screenshot overlay
#include <QPixmap>                 // 用于捕获屏幕 | Used for capturing screen
#include <QParallelAnimationGroup> // 用于同时执行缩放和淡出 | Used for simultaneous scaling and fading
#include <QStyle>
#include <QLinearGradient>
#include <QFontDatabase>
#include <QScreen>
#include <QGuiApplication>

#include <QSyntaxHighlighter>
#include "GlossaryManager.h"
#include "ModernUI.h"

namespace
{
QString normalizeApiBaseUrl(const QString &raw)
{
    QString normalized = raw.trimmed();
    while (normalized.endsWith('/'))
    {
        normalized.chop(1);
    }
    return normalized;
}
}

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

// Windows 11 圆角偏好设置常量（如果SDK版本较旧则手动定义）
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif
#endif

// ==========================================
// 🔥 拦截器：防止右键点击时弹出下拉历史列表
// ==========================================
class RightClickBlocker : public QObject
{
public:
    explicit RightClickBlocker(QObject *parent = nullptr) : QObject(parent) {}
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        // 拦截鼠标按下事件
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::RightButton)
            {
                return true; // 🔥 直接吞噬右键按下事件，彻底阻止下拉框展开！
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

// ==========================================
// 🌍 引用字典 (Externs) | Reference Dictionary
// ==========================================
extern const char *STR_API_ADDR[];
extern const char *STR_API_KEY[];
extern const char *STR_MODEL[];
extern const char *STR_PORT[];
extern const char *STR_THREAD[];
extern const char *STR_TEMP[];
extern const char *STR_CTX[];
extern const char *STR_SYS_PROMPT[];
extern const char *STR_PRE_PROMPT[];
extern const char *STR_GLOSSARY[];
extern const char *STR_CHK_GLOSSARY[];
extern const char *STR_LOCK_SYS[];
extern const char *STR_CLEAR_CTX[];
extern const char *STR_BTN_AUTO[];
extern const char *STR_START[];
extern const char *STR_RELOAD[];
extern const char *STR_STOP[];
extern const char *STR_TOKENS[];
extern const char *STR_FETCH[];
extern const char *STR_TEST[];
extern const char *STR_CLEAR_LOG[];
extern const char *STR_REMOVE_PATH[];
extern const char *STR_CLEAR_HISTORY[];
extern const char *TIP_TOKENS[];
extern const char *TIP_PORT[];
extern const char *TIP_THREAD[];
extern const char *TIP_TEMP[];
extern const char *TIP_CTX[];
extern const char *TIP_GLOSSARY[];
extern const char *TIP_LOCK_SYS[];
extern const char *TIP_BTN_AUTO[];
extern const char *TIP_CLEAR_CTX[];
extern const char *TIP_COMBO_MAIN[];
extern const char *LOG_TEST_START[];
extern const char *LOG_NO_KEY[];
extern const char *LOG_PARSE_ERR[];
extern const char *LOG_AUTO_TESTING[];
extern const char *LOG_MODEL_SWITCH[];
extern const char *LOG_MODEL_SAME[];
extern const char *LOG_RELOADED[];
extern const char *LOG_CFG_SAVED[];
extern const char *LOG_EXPORTED[];
extern const char *LOG_CFG_LOADED[];
extern const char *STR_BATCH_MODE[];
extern const char *TIP_BATCH_MODE[];

// API 预设列表 | API Preset List
struct LocalApiPreset
{
    const char *url;
    const char *tips[2]; // [0]=English, [1]=Chinese
};

static const LocalApiPreset MODERN_PRESETS[] = {
    {"https://api.openai.com/v1", {"OpenAI Official API", "OpenAI 官方接口"}},
    {"https://api.deepseek.com", {"DeepSeek Official API", "DeepSeek 官方接口"}},
    {"https://api.x.ai/v1", {"Grok (xAI) Official API", "Grok (xAI) 官方接口"}},
    {"https://api.siliconflow.cn/v1", {"SiliconFlow", "硅基流动 (SiliconFlow)"}},
    {"https://openrouter.ai/api/v1", {"OpenRouter Aggregator", "OpenRouter 聚合平台"}},
    {"https://generativelanguage.googleapis.com/v1beta/openai", {"Google Gemini", "Google Gemini (OpenAI 兼容端点)"}},
    {"http://localhost:11434/v1", {"Ollama Local Service", "Ollama 本地服务"}},
    {"http://localhost:1234/v1", {"LM Studio Local Service", "LM Studio 本地服务"}}};

// ==========================================
// � GlassPresetDialog: 极简流光预设添加面板
// ==========================================
class GlassPresetDialog : public QDialog
{
public:
    GlassPresetDialog(QWidget *parent, int lang, bool isDark, int alpha, bool isRounded, GlassRenderMode mode, int hueShift, int tintIntensity)
        : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog), 
          m_lang(lang), m_isDark(isDark), m_isRounded(isRounded), m_alpha(alpha), 
          m_mode(mode), m_hueShift(hueShift), m_tintIntensity(tintIntensity)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(480, 440);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(8);

        // 动态取色
        QColor accentColor = isDark ? QColor(255, 94, 0) : QColor(148, 0, 211);
        accentColor = shiftHue(accentColor, hueShift);
        QString accentHex = accentColor.name(QColor::HexRgb);
        int dynAlpha = qBound(100, 255 * tintIntensity / 100, 255);
        QColor bgAccent = accentColor; bgAccent.setAlpha(dynAlpha);
        QString bgAccentHex = bgAccent.name(QColor::HexArgb);

        // Title
        QLabel *lblTitle = new QLabel(m_lang == 1 ? "✨ 添加自定义预设" : "✨ Add Custom Preset", this);
        lblTitle->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1; margin-bottom: 6px;").arg(m_isDark ? "white" : "black"));
        mainLayout->addWidget(lblTitle);

        // Styling helpers
        QString labelStyle = QString("color: %1; font-weight: bold; font-size: 12px; margin-top: 2px; margin-bottom: 2px;").arg(m_isDark ? "#E0E0E0" : "#333333");
        QString inputStyle = m_isDark ? 
            "QLineEdit { background: rgba(0,0,0,80); border: 1px solid rgba(255,255,255,20); color: white; padding: 6px 10px; border-radius: 6px; font-size: 13px; }"
            "QLineEdit:focus { border: 1px solid " + accentHex + "; background: rgba(0,0,0,100); }" : 
            "QLineEdit { background: rgba(255,255,255,180); border: 1px solid rgba(0,0,0,20); color: black; padding: 6px 10px; border-radius: 6px; font-size: 13px; }"
            "QLineEdit:focus { border: 1px solid " + accentHex + "; background: rgba(255,255,255,220); }";

        // URL
        QLabel *lblUrl = new QLabel(m_lang == 1 ? "🔗 API 地址 (URL)  <span style='color:"+accentHex+";'>必须</span>" : "🔗 API URL  <span style='color:"+accentHex+";'>required</span>", this);
        lblUrl->setTextFormat(Qt::RichText);
        lblUrl->setStyleSheet(labelStyle);
        m_urlEdit = new QLineEdit(this);
        m_urlEdit->setStyleSheet(inputStyle);
        m_urlEdit->setMinimumHeight(32);
        m_urlEdit->setPlaceholderText("https://api.openai.com/v1");
        mainLayout->addWidget(lblUrl);
        mainLayout->addWidget(m_urlEdit);

        // Key
        QLabel *lblKey = new QLabel(m_lang == 1 ? "🔑 API 密钥 (Key)  <span style='color:"+accentHex+";'>必须</span>" : "🔑 API Key  <span style='color:"+accentHex+";'>required</span>", this);
        lblKey->setTextFormat(Qt::RichText);
        lblKey->setStyleSheet(labelStyle);
        m_keyEdit = new QLineEdit(this);
        m_keyEdit->setEchoMode(QLineEdit::Normal); // 强制明文显示
        m_keyEdit->setStyleSheet(inputStyle);
        m_keyEdit->setMinimumHeight(32);
        m_keyEdit->setPlaceholderText(m_lang == 1 ? "输入你的 API 密钥" : "Enter your API Key");
        mainLayout->addWidget(lblKey);
        mainLayout->addWidget(m_keyEdit);

        // Model
        QLabel *lblModel = new QLabel(m_lang == 1 ? "📦 模型名称 (Model)  <span style='color:"+accentHex+";'>必须</span>" : "📦 Model Name  <span style='color:"+accentHex+";'>required</span>", this);
        lblModel->setTextFormat(Qt::RichText);
        lblModel->setStyleSheet(labelStyle);
        mainLayout->addWidget(lblModel);

        QHBoxLayout *modelLayout = new QHBoxLayout();
        m_modelCombo = new SideGlassCombo(this);
        m_modelCombo->setEnv(m_isDark, m_alpha, m_isRounded);
        m_modelCombo->setRenderMode(m_mode);
        m_modelCombo->setHueShift(m_hueShift);
        m_modelCombo->setTintIntensity(m_tintIntensity);
        m_modelCombo->setEditable(true);
        m_modelCombo->lineEdit()->setStyleSheet("QLineEdit { padding: 0px 6px; margin: 0px; background: transparent; border: none; font-size: 12px; color: " + QString(m_isDark ? "white;" : "black;") + "}");
        m_modelCombo->setMinimumHeight(32);
        m_modelCombo->setStyleSheet(inputStyle); // Add borders to combo

        m_btnFetch = new QPushButton(m_lang == 1 ? "获取模型" : "Fetch", this);
        m_btnFetch->setFixedSize(80, 32);
        m_btnFetch->setStyleSheet(m_isDark ? 
            "QPushButton { background: rgba(255,255,255,10); color: white; border: 1px solid rgba(255,255,255,20); border-radius: 6px; font-size: 13px; }"
            "QPushButton:hover { background: rgba(255,255,255,30); border-color: "+accentHex+"; }" :
            "QPushButton { background: rgba(0,0,0,5); color: black; border: 1px solid rgba(0,0,0,15); border-radius: 6px; font-size: 13px; }"
            "QPushButton:hover { background: rgba(0,0,0,15); border-color: "+accentHex+"; }");
        m_btnFetch->setCursor(Qt::PointingHandCursor);

        modelLayout->setContentsMargins(0, 0, 0, 0);
        modelLayout->setSpacing(6);
        modelLayout->addWidget(m_modelCombo);
        modelLayout->addWidget(m_btnFetch);
        mainLayout->addLayout(modelLayout);

        // Status Line for Models
        QHBoxLayout *statusLayout = new QHBoxLayout();
        m_lblStatusLeft = new QLabel(this);
        m_lblStatusRight = new QLabel(this);
        
        QString statusStyle = QString("color: %1; font-size: 11px;").arg(m_isDark ? "rgba(255,255,255,80)" : "rgba(0,0,0,100)");
        m_lblStatusLeft->setStyleSheet(statusStyle);
        m_lblStatusRight->setStyleSheet(statusStyle);
        
        statusLayout->setContentsMargins(0, 2, 0, 4);
        statusLayout->addWidget(m_lblStatusLeft);
        statusLayout->addStretch(1);
        statusLayout->addWidget(m_lblStatusRight);
        mainLayout->addLayout(statusLayout);

        // Name
        QLabel *lblName = new QLabel(m_lang == 1 ? "📝 预设名称 (可选)" : "📝 Preset Name (Optional)", this);
        lblName->setStyleSheet(labelStyle);
        m_nameEdit = new QLineEdit(this);
        m_nameEdit->setStyleSheet(inputStyle);
        m_nameEdit->setMinimumHeight(32);
        m_nameEdit->setPlaceholderText(m_lang == 1 ? "例如：我的 OpenAI 兼容服务" : "e.g., My OpenAI API");
        mainLayout->addWidget(lblName);
        mainLayout->addWidget(m_nameEdit);
        
        mainLayout->addStretch(1);

        // Buttons
        QHBoxLayout *btnLayout = new QHBoxLayout();
        QPushButton *btnCancel = new QPushButton(m_lang == 1 ? "取消" : "Cancel", this);
        QPushButton *btnOk = new QPushButton(m_lang == 1 ? "💾 保存预设" : "💾 Save Preset", this);
        
        btnCancel->setMinimumHeight(32);
        btnOk->setMinimumHeight(32);
        
        QString btnHStyle = m_isDark ? 
            "QPushButton { background: rgba(255,255,255,10); color: white; border: 1px solid rgba(255,255,255,20); padding: 4px 20px; border-radius: 6px; font-size: 13px; }"
            "QPushButton:hover { background: rgba(255,255,255,30); }" :
            "QPushButton { background: rgba(0,0,0,5); color: black; border: 1px solid rgba(0,0,0,20); padding: 4px 20px; border-radius: 6px; font-size: 13px; }"
            "QPushButton:hover { background: rgba(0,0,0,15); }";
            
        // 使用带有透明度的动态主颜色按钮
        QString btnAccentStyle = QString("QPushButton { background: %1; color: white; border: none; padding: 4px 20px; border-radius: 6px; font-weight: bold; font-size: 13px; margin-left: 10px; }"
                                         "QPushButton:hover { background: %2; }").arg(bgAccentHex, accentHex);
        
        btnCancel->setStyleSheet(btnHStyle);
        btnOk->setStyleSheet(btnAccentStyle);
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnOk->setCursor(Qt::PointingHandCursor);

        btnLayout->addStretch();
        btnLayout->addWidget(btnCancel);
        btnLayout->addWidget(btnOk);
        mainLayout->addLayout(btnLayout);

        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(btnOk, &QPushButton::clicked, this, [this, dynAlpha]() {
            if (m_urlEdit->text().trimmed().isEmpty() || m_keyEdit->text().trimmed().isEmpty() || m_modelCombo->currentText().trimmed().isEmpty()) {
                GlassMessageBox::warning(this, m_lang == 1 ? "信息不完整" : "Incomplete Info",
                                 m_lang == 1 ? "API 地址、密钥和模型名称为必填项。" : "URL, Key and Model are required.",
                                 m_isDark, m_alpha, m_isRounded, m_mode, m_hueShift, m_tintIntensity);
                return;
            }
            accept();
        });

        connect(m_btnFetch, &QPushButton::clicked, this, &GlassPresetDialog::onFetchModels);
    }

    QString getUrl() const { return m_urlEdit->text().trimmed(); }
    QString getKey() const { return m_keyEdit->text().trimmed(); }
    QString getModel() const { return m_modelCombo->currentText().trimmed(); }
    QString getName() const { return m_nameEdit->text().trimmed(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        int radius = m_isRounded ? 12 : 0;
        
        if (m_mode == GlassRenderMode::Frosted) {
            drawMenuGlassEffect(painter, rect(), m_isDark, m_alpha, radius, m_hueShift, m_tintIntensity);
        } else {
            drawLegacyGlowEffect(painter, rect(), m_isDark, m_alpha, radius, 1.0f, 0.0f);
        }
    }
    
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            e->accept();
        }
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (e->buttons() & Qt::LeftButton) {
            move(e->globalPosition().toPoint() - m_dragPos);
            e->accept();
        }
    }

private:
    void onFetchModels()
    {
        QString url = m_urlEdit->text().trimmed();
        QString key = m_keyEdit->text().trimmed();
        if (url.isEmpty() || key.isEmpty()) {
            GlassMessageBox::warning(this, m_lang == 1 ? "错误" : "Error",
                             m_lang == 1 ? "请先填写 API 地址和密钥。" : "Please fill URL and Key first.",
                             m_isDark, m_alpha, m_isRounded, m_mode, m_hueShift, m_tintIntensity);
            return;
        }

        m_btnFetch->setEnabled(false);
        QTimer *spinTimer = new QTimer(m_btnFetch);
        spinTimer->setProperty("angle", 0);
        QColor arrowColor = m_isDark ? Qt::white : QColor(50, 50, 50);
        
        connect(spinTimer, &QTimer::timeout, [this, spinTimer, arrowColor]() {
            int angle = spinTimer->property("angle").toInt();
            angle = (angle + 15) % 360;
            spinTimer->setProperty("angle", angle);

            QPixmap pix(16, 16);
            pix.fill(Qt::transparent);
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            p.translate(8, 8);
            p.rotate(angle);
            p.translate(-8, -8);
            
            QPen pen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(pen);
            p.drawArc(3, 3, 10, 10, 90 * 16, -270 * 16);
            p.drawLine(3, 8, 1, 11);
            p.drawLine(3, 8, 5, 11);
            
            m_btnFetch->setIcon(QIcon(pix));
        });
        spinTimer->start(30);
        
        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        if (url.endsWith("/")) url.chop(1);
        QNetworkRequest req(QUrl(url + "/models"));
        req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
        QNetworkReply *reply = mgr->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, mgr, spinTimer]() {
            spinTimer->stop();
            spinTimer->deleteLater();
            m_btnFetch->setIcon(QIcon());
            m_btnFetch->setEnabled(true);
            
            if (reply->error() == QNetworkReply::NoError) {
                try {
                    auto doc = nlohmann::json::parse(reply->readAll().toStdString());
                    if (doc.contains("data") && doc["data"].is_array()) {
                        m_modelCombo->clear();
                        for (const auto& val : doc["data"]) {
                            if (val.contains("id") && val["id"].is_string()) {
                                m_modelCombo->addItem(QString::fromStdString(val["id"].get<std::string>()));
                            }
                        }
                        if (m_modelCombo->count() > 0) {
                            m_modelCombo->setCurrentIndex(0);
                            
                            // 更新状态文本
                            QColor accentColor = m_isDark ? QColor(255, 94, 0) : QColor(148, 0, 211);
                            accentColor = shiftHue(accentColor, m_hueShift);
                            QString accentHex = accentColor.name(QColor::HexRgb);
                            
                            m_lblStatusLeft->setText(m_lang == 1 ? "● 上次获取: <span style='color:"+accentHex+";'>刚刚</span>" : "● Last fetch: <span style='color:"+accentHex+";'>Just now</span>");
                            m_lblStatusLeft->setTextFormat(Qt::RichText);
                            m_lblStatusRight->setText(QString(m_lang == 1 ? "共 %1 个模型" : "Total %1 models").arg(m_modelCombo->count()));
                        }
                    } else {
                        m_lblStatusLeft->setText(m_lang == 1 ? "<span style='color:#FF5555;'>解析失败：数据格式不符合规范</span>" : "<span style='color:#FF5555;'>Parse Error: Invalid format</span>");
                        m_lblStatusRight->setText(m_lang == 1 ? "无模型" : "No models");
                    }
                } catch (...) {
                    m_lblStatusLeft->setText(m_lang == 1 ? "<span style='color:#FF5555;'>解析失败：非有效的 JSON 返回</span>" : "<span style='color:#FF5555;'>Parse Error: Invalid JSON</span>");
                    m_lblStatusRight->setText(m_lang == 1 ? "无模型" : "No models");
                }
            } else {
                int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                QString errorMsg;
                if (code == 401) errorMsg = m_lang == 1 ? "401 密钥失效或权限不足" : "401 Invalid Key";
                else if (code == 404) errorMsg = m_lang == 1 ? "404 接口地址不存在" : "404 Not Found";
                else if (code == 429) errorMsg = m_lang == 1 ? "429 访问受限或余额不足" : "429 Too Many Requests/No Balance";
                else if (code == 0) errorMsg = m_lang == 1 ? "网络不通，请检查地址是否拦截" : "Network Unreachable";
                else errorMsg = QString(m_lang == 1 ? "%1 发生未知网络错误" : "%1 Unknown Network Error").arg(code);
                
                m_lblStatusLeft->setText("<span style='color:#FF5555;'>● " + errorMsg + "</span>");
                m_lblStatusRight->setText(m_lang == 1 ? "获取失败" : "Fetch Failed");
            }
            reply->deleteLater();
            mgr->deleteLater();
        });
    }

    int m_lang, m_alpha, m_hueShift, m_tintIntensity;
    bool m_isDark, m_isRounded;
    GlassRenderMode m_mode;
    QPoint m_dragPos;

    QLineEdit *m_urlEdit;
    QLineEdit *m_keyEdit;
    SideGlassCombo *m_modelCombo;
    QPushButton *m_btnFetch;
    QLineEdit *m_nameEdit;
    QLabel *m_lblStatusLeft;
    QLabel *m_lblStatusRight;
};

// ==========================================
// �🚀 ModernWindow 实现 | Implementation
// ==========================================
ModernWindow::ModernWindow(TranslationServer *server, QWidget *parent)
    : QMainWindow(parent), m_server(server), m_isServerRunning(false), m_alpha(210), m_lang(1), m_isDark(true), m_paletteAdapter(nullptr)
{
    // 设置无边框窗口标志 | Set frameless window flags
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    // 启用半透明背景 | Enable translucent background
    setAttribute(Qt::WA_TranslucentBackground);

    // 🔥 CAN: 这里已经按照您的要求修改为 500x832
    // 🔥 CAN: Modified to 500x832 as per your request
    setMinimumSize(500, 600);
    resize(500, 832);

    // 🌫️ 禁用Win11原生圆角效果，防止四角出现阴影
    // Disable Win11 native rounded corners to prevent shadow at corners
#ifdef Q_OS_WIN
    QTimer::singleShot(0, this, [this]()
                       {
        HWND hwnd = (HWND)this->winId();
        if (hwnd)
        {
            // 禁用圆角
            int cornerPreference = DWMWCP_DONOTROUND;
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
            
            // 强制重新计算窗口区域
            RECT rect;
            GetWindowRect(hwnd, &rect);
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        } });
#endif

    setupUi();
    setupApiKeyMemory();
    m_lastApiBaseUrl = normalizeApiBaseUrl(apiAddressCombo->currentText());
    m_apiKeyMemoryEnabled = true;

    if (m_server)
    {
        // 连接信号槽 | Connect signals and slots
        connect(&LogManager::instance(), &LogManager::newLogAvailable, this, &ModernWindow::updateLog);
        connect(&LogManager::instance(), &LogManager::logsCleared, logArea, &QTextEdit::clear);

        m_tokenManager = new TokenManager(this);

        // 1. 服务器产生消耗 -> 告诉 TokenManager (记账)
        connect(m_server, &TranslationServer::tokenUsageReceived,
                m_tokenManager, &TokenManager::addUsage);

        // 2. TokenManager 更新账本 -> 告诉 UI (刷新显示)
        // 注意：这里我们连接到了修改了签名后的 updateToken
        connect(m_tokenManager, &TokenManager::tokensUpdated,
                this, &ModernWindow::updateToken);

        connect(m_server, &TranslationServer::serverStarted, this, [this]()
                { updatePowerButtonState(true); });
        connect(m_server, &TranslationServer::serverStopped, this, [this]()
                { updatePowerButtonState(false); });
    }

    setStyleSheet(getModernStyle(m_isDark, m_isRounded));
    // ❌ 原来的动画代码已删除，移至 showEvent
    // ❌ Original animation code removed, moved to showEvent

    this->setMouseTracking(true);
    if (centralWidget())
        centralWidget()->setMouseTracking(true);
}

ModernWindow::~ModernWindow() {}

void ModernWindow::setupApiKeyMemory()
{
    connect(apiAddressCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        handleApiBaseUrlChanged();
    });

    if (apiAddressCombo->lineEdit())
    {
        connect(apiAddressCombo->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
            handleApiBaseUrlChanged();
        });
    }

    connect(apiKeyEdit, &QLineEdit::editingFinished, this, [this]() {
        persistCurrentApiKeyMemory();
    });

    connect(modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        persistCurrentApiKeyMemory();
    });

    if (modelCombo->lineEdit())
    {
        connect(modelCombo->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
            persistCurrentApiKeyMemory();
        });
    }
}

void ModernWindow::persistCurrentApiKeyMemory()
{
    if (!m_apiKeyMemoryEnabled)
    {
        return;
    }

    const QString currentBaseUrl = normalizeApiBaseUrl(apiAddressCombo->currentText());
    if (currentBaseUrl.isEmpty())
    {
        return;
    }

    ConfigManager::saveApiKeyForBaseUrl(currentBaseUrl, apiKeyEdit->text(), "config.ini");
    ConfigManager::saveModelForBaseUrl(currentBaseUrl, modelCombo->currentText(), "config.ini");
    m_lastApiBaseUrl = currentBaseUrl;
}

void ModernWindow::handleApiBaseUrlChanged()
{
    if (!m_apiKeyMemoryEnabled)
    {
        return;
    }

    const QString newBaseUrl = normalizeApiBaseUrl(apiAddressCombo->currentText());
    const QString oldBaseUrl = normalizeApiBaseUrl(m_lastApiBaseUrl);

    if (!oldBaseUrl.isEmpty() && oldBaseUrl != newBaseUrl)
    {
        ConfigManager::saveApiKeyForBaseUrl(oldBaseUrl, apiKeyEdit->text(), "config.ini");
        ConfigManager::saveModelForBaseUrl(oldBaseUrl, modelCombo->currentText(), "config.ini");
    }

    if (!newBaseUrl.isEmpty() && oldBaseUrl != newBaseUrl)
    {
        const QString mappedApiKey = ConfigManager::loadApiKeyForBaseUrl(newBaseUrl, "config.ini");
        const QString mappedModelName = ConfigManager::loadModelForBaseUrl(newBaseUrl, "config.ini");
        QSignalBlocker blocker(apiKeyEdit);
        apiKeyEdit->setText(mappedApiKey);

        QSignalBlocker blockerModel(modelCombo);
        modelCombo->setCurrentText(mappedModelName);
    }

    m_lastApiBaseUrl = newBaseUrl;
}

void ModernWindow::closeEvent(QCloseEvent *event)
{
    // 检查是否已经是第二次触发（动画已完成）
    // Check if this is the second trigger (animation completed)
    // 我们利用动态属性来标记，不需要在头文件里改动变量
    // We use dynamic property to mark, no need to change variables in header
    if (this->property("finished_closing").toBool())
    {
        event->accept();
        return;
    }

    // 1. 拦截关闭事件 | Intercept close event
    event->ignore();

    // 2. 准备配置保存（提前保存，防止退出延迟）
    // 2. Prepare config saving (save early to prevent exit delay)
    persistCurrentApiKeyMemory();
    ConfigManager::saveConfig(getUiConfig(), "config.ini");
    // QSettings sess("config.ini", QSettings::IniFormat);
    // sess.setValue("Settings/lock_glossary_path", chkLockGlossary->isChecked());
    // sess.sync();

    if (m_server)
        m_server->stopServer();

    // 3. 创建退出动画组 (并行执行)
    // 3. Create exit animation group (parallel execution)
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    // --- 动画 A: 透明度淡出 | Animation A: Opacity Fade Out ---
    QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
    fadeAnim->setDuration(400);
    fadeAnim->setStartValue(this->windowOpacity());
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::InCubic);

    // --- 动画 B: 缩放效果 (可选，增强质感)
    // --- Animation B: Scaling Effect (Optional, enhances texture)
    // 注意：FramelessWindowHint 窗口的缩放通常比较复杂，
    // Note: Scaling for FramelessWindowHint windows is usually complex
    // 我们这里采用最稳定的淡出方式，如果你想要缩放，需要 QVariantAnimation 配合几何尺寸
    // We use the most stable fade-out here; for scaling, need QVariantAnimation with geometry
    // 为了极致的稳定性，我们主要关注透明度和位移
    // For ultimate stability, we focus on opacity and position
    group->addAnimation(fadeAnim);

    // 4. 动画结束后，标记状态并再次触发关闭
    // 4. After animation finishes, mark state and trigger close again
    connect(group, &QParallelAnimationGroup::finished, this, [this]()
            {
                this->setProperty("finished_closing", true);
                this->close(); // 再次调用 close()，由于 flag 为 true，这次会 accept
                // Call close() again, since flag is true, this time it will accept
            });
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

// 获取友好的错误消息 | Get friendly error message
QString ModernWindow::getFriendlyErrorMessage(int code, int lang)
{
    if (lang == 1) // Chinese
    {
        switch (code)
        {

        case 0:
            return "<font color=red>网络连接超时：无法连接到服务器，请检查 API 地址或网络代理。</font>";
        case 400:
            return "<font color=red>请求格式错误 (400): 请检查参数设置是否符合该模型要求。</font>";
        case 401:
            return "<font color=red>身份验证失败 (401): API Key 或是 API地址 填写错误、已过期或已被封禁。</font>";
        case 402:
            return "<font color=red>额度不足 (402): 账户余额已耗尽，请前往官网充值。</font>";
        case 403:
            return "<font color=red>拒绝访问 (403): 权限不足，或者该 Key 不支持访问所选模型。</font>";
        case 404:
            return "<font color=red>地址错误 (404): API 地址或路径错误，请检查末尾是否多/少了 /v1。</font>";
        case 429:
            return "<font color=red>频率限制/额度用尽 (429): 请求太快了，或者本月免费额度已领完。</font>";
        case 500:
            return "<font color=red>服务器错误 (500): 供应商服务器崩溃了。</font>";
        case 503:
            return "<font color=red>服务不可用 (503): 供应商正在维护中。</font>";
        case 999:
            return "<font color=red>连接超时 (10s): 服务器响应太慢，请检查网络是否稳定。</font>";
        default:
            return QString("<font color=red>网络错误 (%1): 请检查 API 地址是否有效。</font>").arg(code);
        }
    }
    else // English
    {
        switch (code)
        {
        case 0:
            return "<font color=red>Connection Timeout: Please check your network or proxy.</font>";
        case 400:
            return "<font color=red>Bad Request (400): Please check if parameters meet model requirements.</font>";
        case 401:
            return "<font color=red>Auth Failed (401): Invalid or expired API Key/Address.</font>";
        case 402:
            return "<font color=red>Insufficient Balance (402): Please top up your account.</font>";
        case 403:
            return "<font color=red>Forbidden (403): Access denied or Key lacks model permissions.</font>";
        case 429:
            return "<font color=red>Rate Limit (429): Too many requests or quota exhausted.</font>";
        case 404:
            return "<font color=red>Not Found (404): Check your API URL.</font>";
        case 500:
            return "<font color=red>Server Error (500): Provider server crashed.</font>";
        case 503:
            return "<font color=red>Service Unavailable (503): Provider is under maintenance.</font>";
        case 999:
            return "<font color=red>Connection Timeout (10s): Server response too slow.</font>";
        default:
            return QString("<font color=red>Error (%1): Check provider status.</font>").arg(code);
        }
    }
}

// 设置 UI 布局 | Setup UI Layout
void ModernWindow::setupUi()
{
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setObjectName("MainContainer");
    setCentralWidget(mainWidget);

    QVBoxLayout *rootLayout = new QVBoxLayout(mainWidget);
    // ⬇️ 压缩外边距：给内容留出更多空间
    // ⬇️ Compress margins: Leave more space for content
    rootLayout->setContentsMargins(10, 5, 10, 10);
    rootLayout->setSpacing(5); // ⬇️ 减少组件间距 | Reduce component spacing

    // --- 顶部窗口栏 | Top Window Bar ---
    QHBoxLayout *winBar = new QHBoxLayout();
    // 🔥 修复 1：设置 ObjectName 以便 CSS 针对性染色
    // 🔥 Fix 1: Set ObjectName for targeted CSS styling
    btnMin = new QPushButton("—");
    btnMin->setObjectName("WinBtnMin");
    btnMin->setFixedSize(30, 30);
    // 移除内联 setStyleSheet，交由 getModernStyle 管理
    // Remove inline setStyleSheet, managed by getModernStyle
    connect(btnMin, &QPushButton::clicked, this, &ModernWindow::showMinimized);

    btnClose = new QPushButton("✕");
    btnClose->setObjectName("WinBtnClose");
    btnClose->setFixedSize(30, 30);
    connect(btnClose, &QPushButton::clicked, this, &ModernWindow::close);

    winBar->addStretch();
    winBar->addWidget(btnMin);
    winBar->addWidget(btnClose);
    rootLayout->addLayout(winBar);

    // --- 顶部工具栏 | Top Toolbar ---
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setContentsMargins(4, 0, 4, 0);

    auto createIconBtn = [&](QChar iconChar, int width = 34)
    {
        QPushButton *btn = new QPushButton(QString(iconChar));
        btn->setFixedSize(width, 28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("TopBarBtn");
        return btn;
    };

    auto createSep = [&]()
    {
        QFrame *line = new QFrame();
        line->setFrameShape(QFrame::VLine);
        line->setFixedSize(2, 16);
        line->setObjectName("TopBarSep");
        return line;
    };

    btnLoad = createIconBtn(QChar(0xE8B7)); // Folder
    connect(btnLoad, &QPushButton::clicked, this, &ModernWindow::onLoadConfig);
    btnSave = createIconBtn(QChar(0xE74E)); // Save
    connect(btnSave, &QPushButton::clicked, this, &ModernWindow::onSaveConfig);
    btnLang = createIconBtn(QChar(0xE12B)); // Globe
    connect(btnLang, &QPushButton::clicked, this, &ModernWindow::toggleLanguage);
    btnTheme = createIconBtn(QChar(0xE793)); // Contrast/HalfMoon
    connect(btnTheme, &QPushButton::clicked, this, &ModernWindow::toggleTheme);
    btnExport = createIconBtn(QChar(0xE8A5)); // Document
    connect(btnExport, &QPushButton::clicked, this, &ModernWindow::onExportLog);
    btnShape = createIconBtn(QChar(0xE737)); // Crop/Shape
    connect(btnShape, &QPushButton::clicked, this, &ModernWindow::toggleShape);
    btnDebug = createIconBtn(QChar(0xE121)); // Clock/Speed Test (Debug Mode)
    connect(btnDebug, &QPushButton::clicked, this, &ModernWindow::toggleDebugMode);
    btnGlassStyle = createIconBtn(QChar(0xE2B3)); // Layer/Glass filter
    connect(btnGlassStyle, &QPushButton::clicked, this, &ModernWindow::toggleGlassStyle);
    btnPalette = createIconBtn(QChar(0xE2B1)); // Palette
    connect(btnPalette, &QPushButton::clicked, this, &ModernWindow::toggleHuePalette);

    // 分组添加按钮和分隔符
    topBar->addWidget(btnLoad);
    topBar->addWidget(btnSave);
    topBar->addSpacing(2);
    topBar->addWidget(createSep());
    topBar->addSpacing(2);

    topBar->addWidget(btnLang);
    topBar->addWidget(btnTheme);
    topBar->addSpacing(2);
    topBar->addWidget(createSep());
    topBar->addSpacing(2);

    topBar->addWidget(btnShape);
    topBar->addWidget(btnGlassStyle);
    topBar->addWidget(btnPalette);
    topBar->addSpacing(2);
    topBar->addWidget(createSep());
    topBar->addSpacing(2);

    topBar->addWidget(btnExport);
    topBar->addWidget(btnDebug);

    topBar->addStretch();
    // 返回按钮放在最右侧
    topBar->addWidget(createSep());
    topBar->addSpacing(2);
    btnBack = createIconBtn(QChar(0xE72B)); // Back
    connect(btnBack, &QPushButton::clicked, this, &ModernWindow::onSwitchClicked);
    topBar->addWidget(btnBack);
    
    rootLayout->addLayout(topBar);

     // --- 内容滚动区 | Content Scroll Area ---
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    // 🔥 核心修复：彻底禁用横向滚动条
    // 🔥 Core Fix: Completely disable horizontal scrollbar
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 🔥 修复悬浮提示框 (QToolTip) 在亮色模式下变黑的问题
    // 使用严格的 ID 选择器，防止 background:transparent 污染全局子控件与悬浮窗
    scroll->setObjectName("MainScrollArea");
    scroll->setStyleSheet("#MainScrollArea, #MainScrollArea > QWidget > QWidget { background:transparent; }");

    QWidget *content = new QWidget();
    content->setObjectName("ScrollContent");
    content->setStyleSheet("#ScrollContent { background:transparent; }");
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(2, 0, 2, 0); // 左右留 2px 缝隙给边框 | Leave 2px gap左右 for border
    layout->setSpacing(6);                  // 卡片间距 | Card spacing

    // 🔥 核心升级：将 QWidget 替换为 GlassCard
    // 🔥 Core Upgrade: Replace QWidget with GlassCard
    auto createCard = [&](QVBoxLayout *&vbox)
    {
        // 传入当前的明暗模式 m_isDark | Pass current dark/light mode
        GlassCard *card = new GlassCard(m_isDark);
        // 🌫️ 毛玻璃效果：将GlassCard添加到跟踪列表
        m_glassCards.append(card);
        // 无需 setObjectName("Card")，因为 GlassCard 构造函数里写了
        // No need to setObjectName("Card"), written in GlassCard constructor
        vbox = new QVBoxLayout(card);
        // 上下左右边距缩小，留出更多有效面积
        // Shrink margins to leave more effective area
        vbox->setContentsMargins(10, 6, 10, 8);
        vbox->setSpacing(4);
        return card;
    };

    // 1. API Config
    QVBoxLayout *v1;
    layout->addWidget(createCard(v1));
    QGridLayout *g1 = new QGridLayout();
    g1->setVerticalSpacing(4);
    g1->setHorizontalSpacing(6); // 锁定列间距为 6px

    // 🔥 CAN 的数学锁定法则：极限压榨前置留白
    const int INPUT_LOCK_W = 385; // 宽度继续补偿 5px，输入框更宽更霸气
    const int BTN_FETCH_W = 60;
    const int MODEL_LOCK_W = INPUT_LOCK_W - BTN_FETCH_W - 6; 

    const int LABEL_W = 65; // 🔥 极限瘦身：精准锁定到 65px (刚好容纳 Base_URL:)
    lblApi = new QLabel();
    lblApi->setFixedWidth(LABEL_W);
    lblKey = new QLabel();
    lblKey->setFixedWidth(LABEL_W);
    lblMod = new QLabel();
    lblMod->setFixedWidth(LABEL_W);

    // --- 第一行：API 地址 ---
    g1->addWidget(lblApi, 0, 0);
    apiAddressCombo = new SideGlassCombo();
    apiAddressCombo->setEnv(m_isDark, m_alpha, m_isRounded);
    apiAddressCombo->setEditable(true);
    apiAddressCombo->lineEdit()->setStyleSheet("QLineEdit { padding: 0px; margin: 0px; background: transparent; border: none; }");
    apiAddressCombo->lineEdit()->setTextMargins(3, 0, 0, 0);
    apiAddressCombo->setMinimumHeight(24);
    apiAddressCombo->setMinimumWidth(INPUT_LOCK_W); // 🔒 锁死总宽度

    int presetCount = sizeof(MODERN_PRESETS) / sizeof(LocalApiPreset);
    for (int k = 0; k < presetCount; ++k)
        apiAddressCombo->addItem(MODERN_PRESETS[k].url);

    // 加载自定义 API URLs
    AppConfig startupCfg = ConfigManager::loadConfig();
    for (const QString &customUrl : startupCfg.custom_api_urls)
    {
        if (!customUrl.isEmpty() && apiAddressCombo->findText(customUrl) == -1)
        {
            apiAddressCombo->addItem(customUrl);
            const QString presetName = ConfigManager::loadPresetNameForBaseUrl(customUrl, "config.ini");
            apiAddressCombo->setItemData(apiAddressCombo->count() - 1,
                presetName.isEmpty() ? (m_lang == 1 ? "自定义 API 地址" : "Custom API URL") : presetName,
                Qt::ToolTipRole);
        }
    }
    
    // 添加 "+" 选项
    apiAddressCombo->addItem("+");

    apiAddressCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(apiAddressCombo, &QComboBox::customContextMenuRequested, this, &ModernWindow::onApiComboContextMenu);

    connect(apiAddressCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (apiAddressCombo->itemText(index) == "+") {
            apiAddressCombo->blockSignals(true);
            apiAddressCombo->setCurrentIndex(0);
            apiAddressCombo->blockSignals(false);

            GlassPresetDialog dlg(this, m_lang, m_isDark, m_alpha, m_isRounded, m_glassRenderMode, m_hueShift, m_tintIntensity);
            if (dlg.exec() != QDialog::Accepted) return;

            QString newUrl = dlg.getUrl();
            QString newKey = dlg.getKey();
            QString newModel = dlg.getModel();
            QString newName = dlg.getName();

            AppConfig cfg = ConfigManager::loadConfig();
            if (!cfg.custom_api_urls.contains(newUrl)) {
                cfg.custom_api_urls.append(newUrl);
            }
            ConfigManager::saveConfig(cfg);
            ConfigManager::saveApiKeyForBaseUrl(newUrl, newKey, "config.ini");
            ConfigManager::saveModelForBaseUrl(newUrl, newModel, "config.ini");
            if (newName.isEmpty()) {
                ConfigManager::removePresetNameForBaseUrl(newUrl, "config.ini");
            } else {
                ConfigManager::savePresetNameForBaseUrl(newUrl, newName, "config.ini");
            }

            int targetIndex = apiAddressCombo->findText(newUrl);
            if (targetIndex == -1) {
                int plusIndex = apiAddressCombo->findText("+");
                if (plusIndex == -1) plusIndex = apiAddressCombo->count();
                apiAddressCombo->insertItem(plusIndex, newUrl);
                targetIndex = plusIndex;
            }

            apiAddressCombo->setItemData(targetIndex,
                newName.isEmpty() ? (m_lang == 1 ? "自定义 API 地址" : "Custom API URL") : newName,
                Qt::ToolTipRole);
            
            apiAddressCombo->setCurrentIndex(targetIndex);
            apiAddressCombo->setCurrentText(newUrl);
            apiKeyEdit->setText(newKey);
            modelCombo->setCurrentText(newModel);
        }
    });

    g1->addWidget(apiAddressCombo, 0, 1, 1, 2); // 跨第 1 和第 2 列

    // --- 第二行：API 密钥 ---
    g1->addWidget(lblKey, 1, 0);
    apiKeyEdit = new QLineEdit();
    apiKeyEdit->setMinimumHeight(24);
    apiKeyEdit->setTextMargins(3, 0, 0, 0);
    apiKeyEdit->setMinimumWidth(INPUT_LOCK_W); // 🔒 锁死总宽度
    g1->addWidget(apiKeyEdit, 1, 1, 1, 2);     // 跨第 1 和第 2 列

    // --- 第三行：模型名称与获取按钮 ---
    g1->addWidget(lblMod, 2, 0);
    modelCombo = new SideGlassCombo();
    modelCombo->setEnv(m_isDark, m_alpha, m_isRounded);
    modelCombo->setEditable(true);
    modelCombo->lineEdit()->setStyleSheet("QLineEdit { padding: 0px; margin: 0px; background: transparent; border: none; }");
    modelCombo->lineEdit()->setTextMargins(3, 0, 0, 0);
    modelCombo->setMinimumHeight(24);
    modelCombo->setMinimumWidth(MODEL_LOCK_W); // 🔒 使用精确计算的 274px
    g1->addWidget(modelCombo, 2, 1);           // 仅占第 1 列

    btnFetch = new QPushButton("获取");
    btnFetch->setObjectName("SmallFuncBtn");
    btnFetch->setFixedSize(BTN_FETCH_W, 24); // 🔒 固定 60px
    connect(btnFetch, &QPushButton::clicked, this, &ModernWindow::onFetchModels);
    g1->addWidget(btnFetch, 2, 2); // 占第 2 列

    v1->addLayout(g1);

    // 2. Parameters
    QVBoxLayout *v2;
    layout->addWidget(createCard(v2));
    QGridLayout *g2 = new QGridLayout();
    g2->setVerticalSpacing(4);
    portEdit = new QLineEdit();
    threadSpin = new QSpinBox();
    tempSpin = new QDoubleSpinBox();
    contextSpin = new QSpinBox();

    // 🔥 修复：彻底隐藏自带的上下调节按钮，保持与 UI 极简质感完全一致
    threadSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    tempSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    contextSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);

    portEdit->setAlignment(Qt::AlignCenter);
    threadSpin->setAlignment(Qt::AlignCenter);
    tempSpin->setAlignment(Qt::AlignCenter);
    contextSpin->setAlignment(Qt::AlignCenter);

    threadSpin->setRange(1, 1000);
    tempSpin->setRange(0, 2);
    contextSpin->setRange(0, 20);
    // 高度统统压缩到 24 | Compress all heights to 24
    portEdit->setFixedHeight(24);
    threadSpin->setFixedHeight(24);
    tempSpin->setFixedHeight(24);
    contextSpin->setFixedHeight(24);

    g2->addWidget(lblPrt = new QLabel(), 0, 0);
    g2->addWidget(portEdit, 0, 1);
    g2->addWidget(lblThd = new QLabel(), 0, 2);
    g2->addWidget(threadSpin, 0, 3);
    g2->addWidget(lblTmp = new QLabel(), 1, 0);
    g2->addWidget(tempSpin, 1, 1);
    g2->addWidget(lblCtx = new QLabel(), 1, 2);

    QHBoxLayout *ctxH = new QHBoxLayout();
    ctxH->addWidget(contextSpin, 1);
    btnClearCtx = new QPushButton();
    btnClearCtx->setObjectName("SmallFuncBtn");
    btnClearCtx->setFixedSize(45, 24);
    connect(btnClearCtx, &QPushButton::clicked, this, &ModernWindow::onClearContext);
    ctxH->addWidget(btnClearCtx);
    g2->addLayout(ctxH, 1, 3);
    v2->addLayout(g2);

    // 3. Prompts
    QVBoxLayout *v3;
    layout->addWidget(createCard(v3));
    QHBoxLayout *h3 = new QHBoxLayout();
    lblSys = new QLabel();
    h3->addWidget(lblSys);
    h3->addStretch();

    // 4 新增：打包开关
    chkBatch = new QCheckBox();

    // 🌟 新增：绑定右键菜单策略
    chkBatch->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(chkBatch, &QCheckBox::customContextMenuRequested, this, &ModernWindow::onBatchContextMenu);

    h3->addWidget(chkBatch);
    h3->addSpacing(10); // 加点间距

    // 5 新增：处理富文本开关
    chkHandleRichText = new QCheckBox();
    connect(chkHandleRichText, &QCheckBox::toggled, this, [this](bool checked)
            {
        m_isHandleRichText = checked;
        if (!chkBatch->isChecked()) return; // 🔥 在多行模式未启用时，屏蔽相关日志与服务器动态更新

        QString msg = (m_lang == 1) ? 
            QString("📝 文本处理: %1").arg(checked ? "已开启" : "已关闭") : 
            QString("📝 HandleRichText: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);
        if (m_isServerRunning && m_server) m_server->updateConfig(getUiConfig()); });
    h3->addWidget(chkHandleRichText);
    h3->addSpacing(10);

    // 6 新增：提取换行开关
    chkExtractNewline = new QCheckBox();
    connect(chkExtractNewline, &QCheckBox::toggled, this, [this](bool checked)
            {
        m_isExtractNewline = checked;
        if (!chkBatch->isChecked()) return; // 🔥 在多行模式未启用时，屏蔽相关日志与服务器动态更新

        QString msg = (m_lang == 1) ? 
            QString("↩️ 保留换行: %1").arg(checked ? "已开启" : "已关闭") : 
            QString("↩️ Keep \\n: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);
        if (m_isServerRunning && m_server) m_server->updateConfig(getUiConfig()); });
    h3->addWidget(chkExtractNewline);
    h3->addSpacing(10);

    // 🔥 初始化子功能状态
    if (chkHandleRichText)
    {
        chkHandleRichText->setEnabled(chkBatch->isChecked());
        QGraphicsOpacityEffect *op1 = new QGraphicsOpacityEffect(chkHandleRichText);
        op1->setOpacity(chkBatch->isChecked() ? 1.0 : 0.4);
        chkHandleRichText->setGraphicsEffect(op1);
    }
    if (chkExtractNewline)
    {
        chkExtractNewline->setEnabled(chkBatch->isChecked());
        QGraphicsOpacityEffect *op2 = new QGraphicsOpacityEffect(chkExtractNewline);
        op2->setOpacity(chkBatch->isChecked() ? 1.0 : 0.4);
        chkExtractNewline->setGraphicsEffect(op2);
    }

    // 🔥修复：连接状态改变时立即更新服务器配置，保持实时同步
    connect(chkBatch, &QCheckBox::toggled, this, [this](bool checked)
            {
        // 🔥 将 HandleRichText 和 ExtractNewline 作为打包模式的子功能
        if (chkHandleRichText) {
            chkHandleRichText->setEnabled(checked);
            QGraphicsOpacityEffect *op1 = new QGraphicsOpacityEffect(chkHandleRichText);
            op1->setOpacity(checked ? 1.0 : 0.4);
            chkHandleRichText->setGraphicsEffect(op1);
        }
        if (chkExtractNewline) {
            chkExtractNewline->setEnabled(checked);
            QGraphicsOpacityEffect *op2 = new QGraphicsOpacityEffect(chkExtractNewline);
            op2->setOpacity(checked ? 1.0 : 0.4);
            chkExtractNewline->setGraphicsEffect(op2);
        }

        QString msg = (m_lang == 1) ?
            QString("📦 多行模式: %1").arg(checked ? "已开启" : "关闭") :
            QString("📦 Batch Mode: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);

        if (m_isServerRunning && m_server) {
            m_server->updateConfig(getUiConfig());
        } });

    chkLockSysPrompt = new QCheckBox();
    h3->addWidget(chkLockSysPrompt);
    v3->addLayout(h3);

    systemPromptEdit = new QTextEdit();
    systemPromptEdit->setMinimumHeight(80); // 压缩高度 | Compress height

    v3->addWidget(systemPromptEdit);
    v3->addWidget(lblPre = new QLabel());
    prePromptEdit = new QLineEdit();
    prePromptEdit->setFixedHeight(24);
    v3->addWidget(prePromptEdit);

    // 4. Glossary
    QVBoxLayout *v4;
    layout->addWidget(createCard(v4));
    QHBoxLayout *h4 = new QHBoxLayout();
    lblGlo = new QLabel();
    h4->addWidget(lblGlo);
    h4->addSpacing(10);
    chkLockGlossary = new QCheckBox();
    h4->addWidget(chkLockGlossary);
    h4->addStretch();
    // --- 新增：编辑术语表按钮 ---
    btnEditGlossary = new QPushButton();
    btnEditGlossary->setObjectName("SmallFuncBtn");
    btnEditGlossary->setFixedSize(110, 24);
    connect(btnEditGlossary, &QPushButton::clicked, this, &ModernWindow::onEditGlossaryClicked);
    h4->addWidget(btnEditGlossary);

    // --- 原有：自动翻译按钮 ---
    btnEditAuto = new QPushButton();
    btnEditAuto->setObjectName("SmallFuncBtn");
    btnEditAuto->setFixedSize(64, 24);
    connect(btnEditAuto, &QPushButton::clicked, this, &ModernWindow::onOpenAutoTranslations);
    h4->addWidget(btnEditAuto);

    v4->addLayout(h4);

    QHBoxLayout *gloRow = new QHBoxLayout();
    chkGlossary = new QCheckBox();
    gloRow->addWidget(chkGlossary);
    glossaryCombo = new SideGlassCombo();
    glossaryCombo->setEnv(m_isDark, m_alpha, m_isRounded);
    glossaryCombo->setEditable(true);
    glossaryCombo->setFixedHeight(24);
    glossaryCombo->setMinimumWidth(330); // 锁死宽度微调 | Lock width tweak

    // ==========================================
    // 🔥 CAN 绝杀：修复右键同时弹出下拉列表的问题
    // ==========================================

    // 1. 安装拦截器，吞噬右键按下，阻止历史记录弹出
    glossaryCombo->installEventFilter(new RightClickBlocker(glossaryCombo));

    // 2. 将右键菜单策略同时应用给 ComboBox 和其内部的 LineEdit
    glossaryCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    glossaryCombo->lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);

    // 3. 绑定信号：无论点在边框还是文字上，都完美唤出我们的清理菜单！
    connect(glossaryCombo, &QComboBox::customContextMenuRequested, this, &ModernWindow::onGlossaryContextMenu);
    connect(glossaryCombo->lineEdit(), &QLineEdit::customContextMenuRequested, this, &ModernWindow::onGlossaryContextMenu);

    connect(glossaryCombo, &QComboBox::activated, this, &ModernWindow::onGlossaryChanged);
    gloRow->addWidget(glossaryCombo, 1);

    btnSelectGlossary = new QPushButton("...");
    btnSelectGlossary->setObjectName("SmallFuncBtn");
    btnSelectGlossary->setFixedSize(36, 24);
    connect(btnSelectGlossary, &QPushButton::clicked, this, &ModernWindow::onSelectGlossary);
    btnSelectGlossary->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(btnSelectGlossary, &QPushButton::customContextMenuRequested, this, &ModernWindow::onGlossaryContextMenu);
    gloRow->addWidget(btnSelectGlossary);
    gloRow->addStretch();
    v4->addLayout(gloRow);

    scroll->setWidget(content);
    scroll->setMinimumHeight(466);
    rootLayout->addWidget(scroll, 1);

    // --- 底部控制区 | Bottom Control Area ---
    QHBoxLayout *mainCtrl = new QHBoxLayout();
    mainCtrl->setSpacing(8);
    mainCtrl->setContentsMargins(2, 0, 2, 0);
    const int CTRL_BTN_H = 34; // 稍微压扁 | Slightly compressed

    btnTest = new QPushButton();
    btnTest->setFixedHeight(CTRL_BTN_H);
    connect(btnTest, &QPushButton::clicked, this, &ModernWindow::onTestConfig);

    btnStop = new QPushButton();
    btnStop->setFixedHeight(CTRL_BTN_H);

    connect(btnStop, &QPushButton::clicked, this, &ModernWindow::onStopClicked);

    btnPower = new QPushButton();
    btnPower->setFixedHeight(CTRL_BTN_H);
    connect(btnPower, &QPushButton::clicked, this, &ModernWindow::onPowerClicked);

    mainCtrl->addWidget(btnTest, 1);
    mainCtrl->addWidget(btnStop, 1);
    mainCtrl->addWidget(btnPower, 1);
    rootLayout->addLayout(mainCtrl);

    lblTokens = new QLabel("TOKENS: 0");
    lblTokens->setObjectName("LblTokens"); // 🔥 赋予专属 ID，交由全局明暗主题 CSS 自动接管
    lblTokens->setAlignment(Qt::AlignCenter);
    rootLayout->addWidget(lblTokens);

    logArea = new QTextEdit();
    logArea->setObjectName("LogArea");
    logArea->setReadOnly(true);
    logArea->setMinimumHeight(150); // 压缩 Log 区域 | Compress Log area
    logArea->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(logArea, &QTextEdit::customContextMenuRequested, this, &ModernWindow::onLogContextMenu);
    logArea->document()->setDocumentMargin(0);

    // 🔥 升级为 GlassCard
    GlassCard *logCard = new GlassCard(m_isDark);
    m_glassCards.append(logCard);
    QVBoxLayout *logCardLayout = new QVBoxLayout(logCard);
    logCardLayout->setContentsMargins(6, 6, 6, 6); 
    logCardLayout->addWidget(logArea);

    QHBoxLayout *logWrapperLayout = new QHBoxLayout();
    logWrapperLayout->setContentsMargins(2, 0, 2, 0); // 强行让盒子左右各缩进 2px
    logWrapperLayout->addWidget(logCard);             // 把发光卡片塞进盒子里
    rootLayout->addLayout(logWrapperLayout, 4);       // 把盒子放进主窗口

    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(50, 255);
    m_opacitySlider->setValue(m_alpha);
    m_opacitySlider->setFixedHeight(14);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &ModernWindow::onOpacityChange);

    // 🔥 CAN 像素级精修：同样新建一个盒子来包裹滑块
    QHBoxLayout *sliderWrapperLayout = new QHBoxLayout();
    sliderWrapperLayout->setContentsMargins(2, 0, 2, 0); // 滑块也左右各缩进 2px
    sliderWrapperLayout->addWidget(m_opacitySlider);
    rootLayout->addLayout(sliderWrapperLayout);
    // 初始化对齐 | Initialize alignment
    connect(glossaryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int)
            { QTimer::singleShot(0, [this]()
                                 { 
            if(glossaryCombo->lineEdit()) glossaryCombo->lineEdit()->setCursorPosition(0); }); });
    connect(apiAddressCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int)
            { QTimer::singleShot(0, [this]()
                                 { 
            if(apiAddressCombo->lineEdit()) apiAddressCombo->lineEdit()->setCursorPosition(0); }); });
}

// 更新 UI 文本 (多语言) | Update UI Text (Multi-language)
void ModernWindow::updateUIText()
{
    int i = m_lang;
    QString currentApi = apiAddressCombo->currentText();
    QSignalBlocker blocker(apiAddressCombo);

    lblApi->setText(STR_API_ADDR[i]);
    lblKey->setText(STR_API_KEY[i]);
    lblMod->setText(STR_MODEL[i]);
    lblPrt->setText(STR_PORT[i]);
    lblThd->setText(STR_THREAD[i]);
    lblTmp->setText(STR_TEMP[i]);
    lblCtx->setText(STR_CTX[i]);
    lblSys->setText(STR_SYS_PROMPT[i]);
    lblPre->setText(STR_PRE_PROMPT[i]);
    lblGlo->setText(STR_GLOSSARY[i]);
    chkGlossary->setText(STR_CHK_GLOSSARY[i]);
    chkLockSysPrompt->setText(STR_LOCK_SYS[i]);
    chkLockGlossary->setText(STR_LOCK_SYS[i]);
    btnClearCtx->setText(STR_CLEAR_CTX[i]);
    btnEditAuto->setText(STR_BTN_AUTO[i]);

    btnDebug->setToolTip(i == 1 ? "开启后，日志将显示底层端口类型和毫秒级耗时" : "Enable to show endpoint types and ms latency in logs");
    btnEditGlossary->setText(i == 1 ? "术语表编辑" : "Edit Terms");
    btnShape->setToolTip(i == 1 ? "切换窗口形状 (圆角/直角)" : "Toggle Window Shape (Rounded/Sharp)");
    btnEditGlossary->setToolTip(i == 1 ? "侧边滑出流光面板，极速编辑当前选择的术语表" : "Slide out glass panel to edit current glossary");

    // 🎨 调色盘按钮
    if (btnPalette) btnPalette->setToolTip(i == 1 ? "全局色调与浓度设定" : "Global Hue & Tint Settings");

    // 🎨 毛玻璃风格切换按钮
    bool isLegacy = (m_glassRenderMode == GlassRenderMode::Legacy);
    btnGlassStyle->setToolTip(isLegacy ? (i == 1 ? "当前：发光特效（点击切换为毛玻璃）" : "Current: Glow Effect (Click to switch to Frosted)") 
                                       : (i == 1 ? "当前：毛玻璃特效（点击切换为发光）" : "Current: Frosted Effect (Click to switch to Glow)"));

    chkHandleRichText->setText(i == 1 ? "文本处理" : "HRText");
    chkHandleRichText->setToolTip(i == 1 ? "【打包模式子功能】保留游戏原文中的富文本。\n只要先开启了打包模式才可生效。\n默认：关闭" : "[Batch Sub-feature] Preserves rich text code in game strings.\nRequires Batch Mode to be enabled first.\nDefault: OFF");

    chkExtractNewline->setText(i == 1 ? "保留换行" : "PLB");
    chkExtractNewline->setToolTip(i == 1 ? "【打包模式子功能】保留游戏原文中的换行符发送给LLM。\n只要先开启了打包模式才可生效。\n默认：开启" : "[Batch Sub-feature] Preserves newlines and sends to LLM.\nRequires Batch Mode to be enabled first.\nDefault: ON");
    btnTest->setText(STR_TEST[i]);
    btnStop->setText(STR_STOP[i]);
    btnFetch->setText(STR_FETCH[i]);

    // 悬浮提示全量补全 | Full tooltip completion
    btnLoad->setToolTip(i == 1 ? "读取配置文件 (.ini)" : "Load Configuration (.ini)");
    btnSave->setToolTip(i == 1 ? "保存当前配置到文件" : "Save Current Configuration");
    btnLang->setToolTip(i == 1 ? "切换语言 (中文/English)" : "Toggle Language (CN/EN)");
    btnTheme->setToolTip(i == 1 ? "切换流光背景主题" : "Toggle Glass Theme Style");
    btnExport->setToolTip(i == 1 ? "导出当前运行日志" : "Export Runtime Logs");
    btnBack->setToolTip(i == 1 ? "返回经典窗口模式" : "Switch to Classic View");
    portEdit->setToolTip(TIP_PORT[i]);
    lblPrt->setToolTip(TIP_PORT[i]);
    threadSpin->setToolTip(TIP_THREAD[i]);
    lblThd->setToolTip(TIP_THREAD[i]);
    tempSpin->setToolTip(TIP_TEMP[i]);
    lblTmp->setToolTip(TIP_TEMP[i]);
    contextSpin->setToolTip(TIP_CTX[i]);
    lblCtx->setToolTip(TIP_CTX[i]);
    chkLockSysPrompt->setToolTip(TIP_LOCK_SYS[i]);
    glossaryCombo->setToolTip(TIP_GLOSSARY[i]);
    chkGlossary->setToolTip(TIP_GLOSSARY[i]);
    btnEditAuto->setToolTip(TIP_BTN_AUTO[i]);
    btnClearCtx->setToolTip(TIP_CLEAR_CTX[i]);
    chkLockGlossary->setToolTip(TIP_LOCK_SYS[i]);
    chkBatch->setText(STR_BATCH_MODE[i]);
    chkBatch->setToolTip(TIP_BATCH_MODE[i]);

    chkExtractNewline->setText(i == 1 ? "保留换行" : "PLB");
    chkExtractNewline->setToolTip(i == 1 ? "开启后，将保留并提取游戏原文中的换行符。\n(自动将 ini 中的 IgnoreWhitespaceInDialogue/NGUI 设为 false)\n默认：关闭" : "If enabled, newlines will be extracted and preserved (sets IgnoreWhitespace... to false).\nDefault: OFF");

    QString baseTip = (i == 1) ? "选择术语表文件 (.txt)" : "Select glossary files (.txt)";
    QString extraTip = (i == 1)
                           ? "\n💡 提示：\n• 左键点击：浏览文件\n• 右键点击：打开清理菜单 (移除路径/清空历史)"
                           : "\n💡 Tip:\n• Left Click: Browse files\n• Right Click: Open cleanup menu";
    btnSelectGlossary->setToolTip(baseTip + extraTip);
    apiAddressCombo->setToolTip(TIP_COMBO_MAIN[i]);

    int presetSize = sizeof(MODERN_PRESETS) / sizeof(MODERN_PRESETS[0]); // 🔥 动态获取数组长度
    for (int k = 0; k < apiAddressCombo->count(); ++k)
    {
        QString urlStr = apiAddressCombo->itemText(k);
        if (urlStr == "+") {
            apiAddressCombo->setItemData(k, i == 1 ? "添加自定义 API 地址" : "Add custom API URL", Qt::ToolTipRole);
            continue;
        }

        bool isPreset = false;
        for (int p = 0; p < presetSize; ++p) // 👈 使用动态长度
        {
            if (urlStr == MODERN_PRESETS[p].url)
            {
                apiAddressCombo->setItemData(k, MODERN_PRESETS[p].tips[i], Qt::ToolTipRole);
                isPreset = true;
                break;
            }
        }
        if (!isPreset) {
            const QString presetName = ConfigManager::loadPresetNameForBaseUrl(urlStr, "config.ini");
            apiAddressCombo->setItemData(k,
                presetName.isEmpty() ? (i == 1 ? "自定义 API 地址" : "Custom API URL") : presetName,
                Qt::ToolTipRole);
        }
    }
    apiAddressCombo->setCurrentText(currentApi);
    updatePowerButtonState(m_isServerRunning);

    long long t = lblTokens->property("current_total").toLongLong();
    long long p = lblTokens->property("current_p").toLongLong();
    long long c = lblTokens->property("current_c").toLongLong();
    updateToken(t, p, c);
}

// 更新 Token 显示 | Update Token Display
void ModernWindow::updateToken(long long total, long long p, long long c)
{
    // 🔥 CAN FIX: 让标签使用动态属性自己记住当前的数值，解决语言切换滞后问题！
    lblTokens->setProperty("current_total", total);
    lblTokens->setProperty("current_p", p);
    lblTokens->setProperty("current_c", c);

    lblTokens->setText(QString("%1 %2").arg(STR_TOKENS[m_lang]).arg(total));

    QString pL = (m_lang == 1) ? "输入总计 (Total Prompt):" : "Total Input:";
    QString cL = (m_lang == 1) ? "输出总计 (Total Completion):" : "Total Output:";

    // Tooltip 显示的也是累计值，清晰明了
    QString fullTip = QString("<b>%1</b><br><br>%2 %3<br>%4 %5")
                          .arg(TIP_TOKENS[m_lang])
                          .arg(pL)
                          .arg(p)
                          .arg(cL)
                          .arg(c);
    lblTokens->setToolTip(fullTip);
}

// 获取模型列表 | Fetch Model List
void ModernWindow::onFetchModels()
{
    QString urlBase = apiAddressCombo->currentText();
    if (urlBase.isEmpty())
        return;
    if (urlBase.endsWith("/"))
        urlBase.chop(1);

    m_server->injectLog(m_lang == 1 ? "🔍 正在获取模型列表..." : "🔍 Fetching models...");

    // ==========================================
    // 🌀 CAN 极简美学：纯白/暗灰 几何顺时针旋转箭头
    // ==========================================
    btnFetch->setEnabled(false);
    btnFetch->setText(""); // 按钮比较窄，清空文字，只展示纯粹的旋转图标

    QTimer *spinTimer = new QTimer(btnFetch);
    spinTimer->setProperty("angle", 0);

    // 智能适配明暗模式，防止亮色模式下白色箭头看不见
    QColor arrowColor = m_isDark ? Qt::white : QColor(50, 50, 50);

    connect(spinTimer, &QTimer::timeout, [this, spinTimer, arrowColor]()
            {
        int angle = spinTimer->property("angle").toInt();
        angle = (angle + 15) % 360; // 每次旋转 15 度
        spinTimer->setProperty("angle", angle);

        QPixmap pix(16, 16);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        
        // 核心：将坐标系平移到中心，旋转后再移回来
        p.translate(8, 8);
        p.rotate(angle);
        p.translate(-8, -8);
        
        QPen pen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        
        // 画 270 度的完美圆弧 (从 12点钟 顺时针画到 9点钟)
        p.drawArc(3, 3, 10, 10, 90 * 16, -270 * 16);
        // 在 9点钟 位置画箭头的两个倒刺
        p.drawLine(3, 8, 1, 11);
        p.drawLine(3, 8, 5, 11);
        
        btnFetch->setIcon(QIcon(pix)); });
    spinTimer->start(30); // 约 33 FPS，极其丝滑
    // ==========================================

    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    QNetworkRequest req;
    req.setUrl(QUrl(urlBase + "/models"));
    QString key = apiKeyEdit->text().split(',')[0].trimmed();
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    req.setTransferTimeout(10000);

    QNetworkReply *rep = mgr->get(req);

    // 🔥 注意：将 spinTimer 传入 Lambda 表达式
    connect(rep, &QNetworkReply::finished, [this, rep, mgr, spinTimer]()
            {
        // ==========================================
        // 🌀 请求结束：销毁几何引擎，卸载图标，恢复文字
        // ==========================================
        spinTimer->stop();
        spinTimer->deleteLater(); 
        btnFetch->setIcon(QIcon()); // 清除图标
        btnFetch->setText(STR_FETCH[m_lang]); 
        btnFetch->setEnabled(true);
        // ==========================================

        if(rep->error() == QNetworkReply::NoError) {
            try {
                auto j = nlohmann::json::parse(rep->readAll().toStdString()); 
                modelCombo->clear();
                int count = 0;
                for(const auto& it : j["data"]) {
                    modelCombo->addItem(QString::fromStdString(it["id"]));
                    count++;}
                m_server->injectLog(m_lang == 1 ? QString("✅ 获取成功！共发现 %1 个可用模型。").arg(count) 
                                                 : QString("✅ Success! Found %1 available models.").arg(count));
                modelCombo->setFocus();
            } catch(...) { 
                m_server->injectLog("❌ " + QString(LOG_PARSE_ERR[m_lang])); 
            }
        } else { 
            m_server->injectLog("❌ " + getFriendlyErrorMessage(rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), m_lang)); 
        }
        rep->deleteLater(); 
        mgr->deleteLater(); });
}

// 测试配置 (网络请求) | Test Configuration (Network Request)
void ModernWindow::onTestConfig()
{
    m_server->injectLog(LOG_TEST_START[m_lang]);
    QStringList keys = apiKeyEdit->text().split(',', Qt::SkipEmptyParts);
    if (keys.isEmpty())
    {
        m_server->injectLog(LOG_NO_KEY[m_lang]);
        return;
    }

    // ==========================================
    // 🌀 CAN 极简美学：纯白/暗灰 几何顺时针旋转箭头
    // ==========================================
    btnTest->setEnabled(false);
    // 按钮较宽，保留文字并在前面加空格拉开与图标的距离
    btnTest->setText(m_lang == 1 ? " 测试中..." : " Testing...");

    QTimer *spinTimer = new QTimer(btnTest);
    spinTimer->setProperty("angle", 0);

    QColor arrowColor = m_isDark ? Qt::white : QColor(50, 50, 50);

    connect(spinTimer, &QTimer::timeout, [this, spinTimer, arrowColor]()
            {
        int angle = spinTimer->property("angle").toInt();
        angle = (angle + 12) % 360; // 测试按钮可以转得稍微稳重一点(12度)
        spinTimer->setProperty("angle", angle);

        QPixmap pix(16, 16);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(8, 8);
        p.rotate(angle);
        p.translate(-8, -8);
        
        QPen pen(arrowColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.drawArc(3, 3, 10, 10, 90 * 16, -270 * 16);
        p.drawLine(3, 8, 1, 11);
        p.drawLine(3, 8, 5, 11);
        
        btnTest->setIcon(QIcon(pix)); });
    spinTimer->start(30);
    // ==========================================

    QString urlBase = apiAddressCombo->currentText();
    if (urlBase.endsWith("/"))
        urlBase.chop(1);
    urlBase += "/chat/completions";

    QString modelStr = modelCombo->currentText();
    auto fnd = std::make_shared<int>(0);
    auto scs = std::make_shared<int>(0);
    int ttl = keys.size();

    for (int i = 0; i < ttl; ++i)
    {
        QString key = keys[i].trimmed();
        QString msk = (key.length() > 8) ? ("..." + key.right(8)) : key;
        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        QNetworkRequest req;
        req.setUrl(QUrl(urlBase));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
        req.setTransferTimeout(10000);

        nlohmann::json j;
        j["model"] = modelStr.toStdString();
        j["messages"] = nlohmann::json::array({{{"role", "user"}, {"content", "hi"}}});
        j["max_tokens"] = 5;

        QNetworkReply *rep = mgr->post(req, QByteArray::fromStdString(j.dump()));

        // 🔥 注意：将 spinTimer 传入 Lambda 表达式
        connect(rep, &QNetworkReply::finished, [this, rep, mgr, msk, i, fnd, scs, ttl, spinTimer]()
                {
            (*fnd)++; if(rep->error() == QNetworkReply::NoError) (*scs)++;
            int cd = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(); 
            if(rep->error() == QNetworkReply::TimeoutError) cd = 999;
            
            QString icn = (rep->error() == QNetworkReply::NoError) ? "✅" : "❌";
            QString st = (rep->error() == QNetworkReply::NoError) ? (m_lang == 1 ? "通过" : "PASS") : getFriendlyErrorMessage(cd, m_lang);
            m_server->injectLog(QString("%1 Key-%2 (%3): %4").arg(icn).arg(i+1).arg(msk).arg(st));
            
            if(*fnd == ttl) {
                // ==========================================
                // 🌀 请求全部结束：销毁定时器，卸载图标
                // ==========================================
                spinTimer->stop();         
                spinTimer->deleteLater();  
                btnTest->setIcon(QIcon()); // 清除图标
                
                btnTest->setEnabled(true); 
                btnTest->setText(STR_TEST[m_lang]);
                // ==========================================

                m_server->injectLog("----------------------------------");
                
                QString successColor = m_isDark ? "#f2be45" : "#9C27B0";
                QString summary = QString(m_lang == 1 
                    ? "<font color='#4CAF50'>📊 测试结束！</font> <font color='%1'>%2: %3</font>, <font color='#F44336'>%4: %5</font>" 
                    : "<font color='#4CAF50'>📊 Finished!</font> <font color='%1'>%2: %3</font>, <font color='#F44336'>%4: %5</font>")
                    .arg(successColor) // %1 颜色
                    .arg(m_lang == 1 ? "成功" : "Success") // %2 标签文字
                    .arg(*scs) // %3 成功数量
                    .arg(m_lang == 1 ? "失败" : "Failed") // %4 标签文字
                    .arg(ttl - *scs); // %5 失败数量
                m_server->injectLog("<b>" + summary + "</b>");
            }
            rep->deleteLater(); 
            mgr->deleteLater(); });
    }
}

// 从配置加载 UI | Load UI from Config
void ModernWindow::loadConfigToUi()
{
    if (!m_server)
        return;

    m_apiKeyMemoryEnabled = false;

    // 1. 抢先从文件加载配置
    AppConfig cfg = ConfigManager::loadConfig();

    // 🔥🔥🔥 CAN 的神级时序修复：必须在所有动作之前，确立语言和主题规则！
    m_lang = cfg.language;

    if (m_isDark != cfg.is_dark || m_isRounded != cfg.is_rounded)
    {
        m_isDark = cfg.is_dark;
        m_isRounded = cfg.is_rounded;
        m_storedIsRounded = cfg.is_rounded; // 同步保护中间值

        // A. 重新生成并覆盖全局 CSS
        setStyleSheet(getModernStyle(m_isDark, m_isRounded));
        // B. 强制刷新主窗口样式缓存
        style()->unpolish(this);
        style()->polish(this);
        // C. 暴力强制刷新所有子控件
        QList<QWidget *> widgets = this->findChildren<QWidget *>();
        for (QWidget *w : widgets)
        {
            style()->unpolish(w);
            style()->polish(w);
        }
        // D. 强制通知所有发光卡片
        QList<GlassCard *> cards = this->findChildren<GlassCard *>();
        for (GlassCard *card : cards)
        {
            card->setTheme(m_isDark);
            card->setRounded(m_isRounded);
        }
    }

    // 2. 同步日志历史 (此时 onLogMessage 拦截器已经知道当前环境了，完美发力！)
    logArea->clear();
    QStringList logs = LogManager::instance().getHistory();
    for (const QString &msg : logs)
    {
        updateLog(msg);
    }

    // 3. 恢复锁定状态 (优先恢复，防止后续逻辑受阻)
    chkLockSysPrompt->setChecked(cfg.lock_system_prompt);
    chkLockGlossary->setChecked(cfg.lock_glossary);

    // 🌟 清理已经被删除的自定义预设
    for (int i = apiAddressCombo->count() - 1; i >= 0; --i)
    {
        QString itemText = apiAddressCombo->itemText(i);
        if (itemText == "+") continue;

        bool isBuiltIn = false;
        int presetSize = sizeof(MODERN_PRESETS) / sizeof(LocalApiPreset);
        for (int p = 0; p < presetSize; ++p) {
            if (MODERN_PRESETS[p].url == itemText) {
                isBuiltIn = true;
                break;
            }
        }
        if (!isBuiltIn && !cfg.custom_api_urls.contains(itemText)) {
            apiAddressCombo->removeItem(i);
        }
    }

    // 🌟 加载最新的自定义 API URLs 到下拉框
    for (const QString &customUrl : cfg.custom_api_urls)
    {
        if (!customUrl.isEmpty() && apiAddressCombo->findText(customUrl) == -1)
        {
            // 插入到 "+" 之前
            int insIndex = apiAddressCombo->count() > 0 ? apiAddressCombo->count() - 1 : 0;
            apiAddressCombo->insertItem(insIndex, customUrl);
            QString presetName = ConfigManager::loadPresetNameForBaseUrl(customUrl, "config.ini");
            apiAddressCombo->setItemData(insIndex, 
                presetName.isEmpty() ? (m_lang == 1 ? "自定义 API 地址" : "Custom API URL") : presetName, Qt::ToolTipRole);
        }
    }

    // 4. 基础 UI 填充
    apiAddressCombo->setCurrentText(cfg.api_address);
    apiKeyEdit->setText(cfg.api_key);
    modelCombo->setCurrentText(cfg.model_name);
    portEdit->setText(QString::number(cfg.port));
    threadSpin->setValue(cfg.max_threads);
    tempSpin->setValue(cfg.temperature);
    contextSpin->setValue(cfg.context_num);

    // 🔥 CAN 抢修：将被遗漏的前置文本填充代码补回！
    prePromptEdit->setText(cfg.pre_prompt);

    // --- 确保多行模式开关使用正确的语言打印日志 ---
    chkBatch->blockSignals(true); // 戴上耳罩，禁止发射 toggled 信号
    chkBatch->setChecked(cfg.enable_batch);
    chkBatch->blockSignals(false); // 摘下耳罩，恢复用户手动点击的响应
    chkGlossary->setChecked(cfg.enable_glossary);

    // --- 同样的逻辑应用到文本处理开关，确保日志语言正确 ---
    m_isHandleRichText = cfg.handle_rich_text;
    chkHandleRichText->blockSignals(true);
    chkHandleRichText->setChecked(m_isHandleRichText);
    chkHandleRichText->blockSignals(false);

    m_isExtractNewline = cfg.extract_newline;
    chkExtractNewline->blockSignals(true);
    chkExtractNewline->setChecked(m_isExtractNewline);
    chkExtractNewline->blockSignals(false);

    // 🔥 CAN 补充：手动更新禁用效果与遮罩 (因为被 blockedSignals 拦截了)
    if (chkHandleRichText)
    {
        chkHandleRichText->setEnabled(cfg.enable_batch);
        QGraphicsOpacityEffect *op1 = new QGraphicsOpacityEffect(chkHandleRichText);
        op1->setOpacity(cfg.enable_batch ? 1.0 : 0.4);
        chkHandleRichText->setGraphicsEffect(op1);
    }
    if (chkExtractNewline)
    {
        chkExtractNewline->setEnabled(cfg.enable_batch);
        QGraphicsOpacityEffect *op2 = new QGraphicsOpacityEffect(chkExtractNewline);
        op2->setOpacity(cfg.enable_batch ? 1.0 : 0.4);
        chkExtractNewline->setGraphicsEffect(op2);
    }

    m_isDebugMode = cfg.enable_debug_mode;
    btnDebug->setProperty("debugOn", m_isDebugMode);
    btnDebug->style()->unpolish(btnDebug);
    btnDebug->style()->polish(btnDebug);

    // 5. 系统提示词逻辑
    if (!cfg.lock_system_prompt || systemPromptEdit->toPlainText().isEmpty())
    {
        systemPromptEdit->setText(cfg.system_prompt);
    }

    // 6. 术语表逻辑 (适配版)
    if (!chkLockGlossary->isChecked())
    {
        glossaryCombo->clear();
        if (!cfg.glossary_history.isEmpty())
            glossaryCombo->addItems(cfg.glossary_history);
        if (!cfg.glossary_path.isEmpty())
            glossaryCombo->setCurrentText(cfg.glossary_path);
    }

    if (glossaryCombo->lineEdit())
        glossaryCombo->lineEdit()->setCursorPosition(0);
    QTimer::singleShot(50, [this]()
                       {
        if(apiAddressCombo->lineEdit()) apiAddressCombo->lineEdit()->setCursorPosition(0);
        if(glossaryCombo->lineEdit()) glossaryCombo->lineEdit()->setCursorPosition(0);
        apiKeyEdit->setCursorPosition(0);
        prePromptEdit->setCursorPosition(0); });

    // --- 7. 恢复透明度并触发重绘 ---
    m_storedModernOpacity = cfg.modern_opacity;
    m_alpha = cfg.modern_opacity;
    if (m_opacitySlider)
    {
        QSignalBlocker blocker(m_opacitySlider);
        m_opacitySlider->setValue(m_alpha);
    }

    // --- 8. 恢复毛玻璃渲染参数 ---
    m_glassRenderMode = (cfg.glass_render_mode == 1) ? GlassRenderMode::Legacy : GlassRenderMode::Frosted;
    m_hueShift = qBound(0, cfg.hue_shift, 360);
    m_tintIntensity = qBound(0, cfg.tint_intensity, 200);

    updateUIText();
    updatePowerButtonState(m_server->isRunning());
    updateComboEnv();
    update(); // 终极重绘

    // 9. 立即同步到 Server！
    if (m_server)
    {
        m_server->updateConfig(cfg);
    }

    m_lastApiBaseUrl = normalizeApiBaseUrl(apiAddressCombo->currentText());
    m_apiKeyMemoryEnabled = true;
}

// 获取 UI 配置 | Get UI Config
AppConfig ModernWindow::getUiConfig()
{
    AppConfig cfg;
    AppConfig savedCfg = ConfigManager::loadConfig(); // 先加载已有配置以继承不需要修改的值
    cfg.custom_api_urls = savedCfg.custom_api_urls;

    cfg.api_address = apiAddressCombo->currentText();
    cfg.api_key = apiKeyEdit->text();
    
    // 检查并保存自定义 API 地址
    if (!cfg.api_address.isEmpty() && cfg.api_address != "+") {
        bool isPreset = false;
        int presetSize = sizeof(MODERN_PRESETS) / sizeof(LocalApiPreset);
        for (int p = 0; p < presetSize; ++p) {
            if (cfg.api_address == MODERN_PRESETS[p].url) {
                isPreset = true;
                break;
            }
        }
        if (!isPreset && !cfg.custom_api_urls.contains(cfg.api_address)) {
            cfg.custom_api_urls.append(cfg.api_address);
        }
    }

    cfg.model_name = modelCombo->currentText();
    cfg.port = portEdit->text().toInt();
    cfg.max_threads = threadSpin->value();
    cfg.temperature = tempSpin->value();
    cfg.context_num = contextSpin->value();
    cfg.system_prompt = systemPromptEdit->toPlainText();
    cfg.pre_prompt = prePromptEdit->text();
    cfg.enable_glossary = chkGlossary->isChecked();
    cfg.glossary_path = glossaryCombo->currentText();
    QStringList h;
    for (int i = 0; i < glossaryCombo->count(); ++i)
        h << glossaryCombo->itemText(i);
    cfg.glossary_history = h;
    cfg.language = m_lang;

    cfg.modern_opacity = m_storedModernOpacity;
    cfg.enable_debug_mode = m_isDebugMode;

    cfg.lock_system_prompt = chkLockSysPrompt->isChecked();
    cfg.lock_glossary = chkLockGlossary->isChecked();

    // --- 🔥 新增：收集当前 UI 状态 | New: Collect current UI state ---
    cfg.is_dark = m_isDark; // 记录当前明暗 | Record current dark/light
    cfg.ui_mode = 1;        // 强制标记：我是流光模式 | Force mark: I am Modern Mode
    // --- 🔥 重点：将透明度设置 | Key: Set opacity ---
    cfg.modern_opacity = m_alpha;
    cfg.is_rounded = m_isRounded;

    cfg.glass_render_mode = (m_glassRenderMode == GlassRenderMode::Legacy) ? 1 : 0;
    cfg.hue_shift = m_hueShift;
    cfg.tint_intensity = m_tintIntensity;

    cfg.is_rounded = m_storedIsRounded;
    cfg.is_from_modern = true;
    cfg.enable_batch = chkBatch->isChecked();
    cfg.handle_rich_text = chkHandleRichText->isChecked();
    cfg.extract_newline = chkExtractNewline->isChecked();

    return cfg;
}

// 电源按钮点击 (启动/重载) | Power Button Click (Start/Reload)
void ModernWindow::onPowerClicked()
{
    if (!m_server)
        return;
    AppConfig nCfg = getUiConfig();
    if (m_isServerRunning)
    {
        AppConfig oCfg = m_server->getConfig();
        if (oCfg.model_name != nCfg.model_name)
            m_server->injectLog(QString(LOG_MODEL_SWITCH[m_lang]).arg(oCfg.model_name).arg(nCfg.model_name));
        else
            m_server->injectLog(QString(LOG_MODEL_SAME[m_lang]).arg(nCfg.model_name));
        m_server->updateConfig(nCfg);
        m_server->injectLog(LOG_RELOADED[m_lang]);
        m_server->injectLog(LOG_AUTO_TESTING[m_lang]);
        onTestConfig();
    }
    else
    {
        m_server->updateConfig(nCfg);
        m_server->startServer();
    }
}

// 停止服务器 | Stop Server
void ModernWindow::onStopClicked()
{
    if (m_server)
        m_server->stopServer();
}

// 更新电源按钮状态 (重构版：极致主题融合的空灵玻璃风格) | Update Power Button State (Refactored: Ethereal Theme-Glass Style)
void ModernWindow::updatePowerButtonState(bool running)
{
    m_isServerRunning = running;

    // 状态逻辑控制
    btnStop->setEnabled(running);
    portEdit->setEnabled(!running);
    threadSpin->setEnabled(!running);

    // 统一圆角参数 (根据神级正则设置调整)
    int r = m_isRounded ? 6 : 0;

    // 动态对比度参数计算
    // 仅在毛玻璃模式 (Frosted) 下对抗流光淹没，Legacy 模式保持原始纯粹空灵感
    bool isFrosted = (m_glassRenderMode == GlassRenderMode::Frosted);
    float tintFactor = isFrosted ? (qBound(0, m_tintIntensity, 200) / 100.0f) : 0.0f;
    
    // 恢复极致空灵的基底透明度，并对亮色模式下的边框/背景进行补偿以增强可见性
    int bgAlphaBase = isFrosted ? (m_isDark ? 15 + int(15 * tintFactor) : 20 + int(20 * tintFactor)) : 15;
    int bgAlphaHover = isFrosted ? (m_isDark ? 40 + int(20 * tintFactor) : 50 + int(20 * tintFactor)) : 40;
    int bgAlphaPress = isFrosted ? (m_isDark ? 60 + int(20 * tintFactor) : 70 + int(20 * tintFactor)) : 60;
    // 亮色模式下大幅增加边框透明度以解决边缘消失的问题
    int borderAlphaBase = isFrosted ? (m_isDark ? 50 + int(20 * tintFactor) : 90 + int(40 * tintFactor)) : (m_isDark ? 45 : 50);

    auto rgbaStr = [](const QColor &c, int a) {
        return QString("rgba(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(a);
    };

    // 独立取色器：彻底修复非毛玻璃下三巨头被流光（HueShift）污染的问题
    auto getDynamicColor = [this, isFrosted](const QColor &baseColor) {
        return isFrosted ? shiftHue(baseColor, this->m_hueShift) : baseColor;
    };

    // 智能获取高对比度文本颜色 (仅在毛玻璃模式下动态防沉没)
    auto getTextColor = [isFrosted](const QColor &c, bool dark) {
        if (!isFrosted) return c.name(); // 非毛玻璃模式保持原色，不干预
        
        QColor tc = c;
        int h, s, l, a;
        tc.getHsl(&h, &s, &l, &a);
        if (dark) {
            // 暗黑模式下，如果是过暗的冷色调(Hue 208等深紫深蓝)，必须大幅拔高亮度(L=200+)穿透暗色背景
            tc.setHsl(h, qMax(0, s - 10), qMax(l, 200), a); 
        } else {
            // 亮色模式下，必须狠狠压低亮度(L=60-)以抵御白底高光的吞噬
            tc.setHsl(h, qMin(255, s + 30), qMin(l, 55), a);
        }
        return tc.name();
    };

    // ============================================================
    // 1. 测试按钮 (BtnTest): 樱花粉色系 -> 动态色相保持对比度
    // ============================================================
    QColor cTest = getDynamicColor(m_isDark ? QColor(255, 105, 180) : QColor(255, 20, 147));
    QString testStyle = QString(
        "QPushButton { background: %2; border: 1px solid %3; border-radius: %1px; color: %6; font-weight: bold; }"
        "QPushButton:hover { background: %4; border: 1px solid %6; color: #FFFFFF; }"
        "QPushButton:pressed { background: %5; }"
    ).arg(r)
     .arg(rgbaStr(cTest, bgAlphaBase))
     .arg(rgbaStr(cTest, borderAlphaBase))
     .arg(rgbaStr(cTest, bgAlphaHover))
     .arg(rgbaStr(cTest, bgAlphaPress))
     .arg(getTextColor(cTest, m_isDark));
    
    btnTest->setStyleSheet(testStyle);

    // ============================================================
    // 2. 停止按钮 (BtnStop): 霓虹玫瑰红 -> 动态色相保持对比度
    // ============================================================
    QColor cStop = getDynamicColor(m_isDark ? QColor(255, 51, 102) : QColor(220, 20, 60));
    QString stopStyle = QString(
        "QPushButton { background: %2; border: 1px solid %3; color: %6; font-weight: bold; border-radius: %1px; }"
        "QPushButton:hover { background: %4; border: 1px solid %6; color: #FFFFFF; }"
        "QPushButton:pressed { background: %5; }"
        "QPushButton:disabled { background: transparent; border: 1px solid rgba(128, 128, 128, 25); color: rgba(128, 128, 128, 80); }"
    ).arg(r)
     .arg(rgbaStr(cStop, bgAlphaBase))
     .arg(rgbaStr(cStop, borderAlphaBase))
     .arg(rgbaStr(cStop, bgAlphaHover))
     .arg(rgbaStr(cStop, bgAlphaPress))
     .arg(getTextColor(cStop, m_isDark));
    
    btnStop->setStyleSheet(stopStyle);

    // ============================================================
    // 3. 电源按钮 (BtnPower): 动态呼吸与主题深度融合 -> 动态色相保持对比度
    // ============================================================
    // 清理旧动画
    QVariantAnimation *oldAnim = btnPower->findChild<QVariantAnimation *>("powerAnim");
    if (oldAnim)
    {
        oldAnim->stop();
        oldAnim->deleteLater();
    }

    if (running)
    {
        // >>> 运行中状态 (Reload): 呼吸脉冲 <<<
        btnPower->setText(STR_RELOAD[m_lang]);

        QVariantAnimation *pulseAnim = new QVariantAnimation(btnPower);
        pulseAnim->setObjectName("powerAnim");
        pulseAnim->setDuration(2500);       // 延长至2.5秒，呼吸感更深邃优雅
        pulseAnim->setStartValue(borderAlphaBase);       // 边框最低透明度
        pulseAnim->setKeyValueAt(0.5, 220); // 边框最高透明度 (峰值极亮)
        pulseAnim->setEndValue(borderAlphaBase);
        pulseAnim->setLoopCount(-1);
        pulseAnim->setEasingCurve(QEasingCurve::InOutSine);

        connect(pulseAnim, &QVariantAnimation::valueChanged, [this, r, bgAlphaHover, bgAlphaBase, getDynamicColor, getTextColor](const QVariant &value)
                {
            int alpha = value.toInt(); 
            // 动态判断当前主题，获取基础取色并进行独立判断处理
            QColor cPowerRun = getDynamicColor(this->m_isDark ? QColor(0, 229, 255) : QColor(255, 140, 0));
            
            QString rgbaBg = QString("rgba(%1, %2, %3, %4)").arg(cPowerRun.red()).arg(cPowerRun.green()).arg(cPowerRun.blue()).arg(bgAlphaBase);
            QString rgbaBorder = QString("rgba(%1, %2, %3, %4)").arg(cPowerRun.red()).arg(cPowerRun.green()).arg(cPowerRun.blue()).arg(alpha);
            QString rgbaHover = QString("rgba(%1, %2, %3, %4)").arg(cPowerRun.red()).arg(cPowerRun.green()).arg(cPowerRun.blue()).arg(bgAlphaHover);

            QString qss = QString(
                "QPushButton { background: %1; border: 1px solid %2; border-radius: %3px; color: %5; font-weight: bold; }"
                "QPushButton:hover { background: %4; color: #FFF; border: 1px solid %5; }"
            ).arg(rgbaBg).arg(rgbaBorder).arg(r).arg(rgbaHover).arg(getTextColor(cPowerRun, this->m_isDark));
            
            btnPower->setStyleSheet(qss); 
        });
        pulseAnim->start();
    }
    else
    {
        // >>> 停止状态 (Start): 呼应核心主题色 <<<
        btnPower->setText(STR_START[m_lang]);
        
        QColor cPowerStop = getDynamicColor(m_isDark ? QColor(255, 140, 0) : QColor(148, 0, 211));
        QString startStyle = QString(
            "QPushButton { background: %2; border: 1px solid %3; border-radius: %1px; color: %6; font-weight: bold; }"
            "QPushButton:hover { background: %4; border: 1px solid %6; color: #FFFFFF; }"
            "QPushButton:pressed { background: %5; }"
        ).arg(r)
         .arg(rgbaStr(cPowerStop, bgAlphaBase))
         .arg(rgbaStr(cPowerStop, borderAlphaBase))
         .arg(rgbaStr(cPowerStop, bgAlphaHover))
         .arg(rgbaStr(cPowerStop, bgAlphaPress))
         .arg(getTextColor(cPowerStop, m_isDark));

        btnPower->setStyleSheet(startStyle);
    }
}

// 更新日志 | Update Log
void ModernWindow::updateLog(QString msg)
{
    if (!logArea)
        return;

    // 🔥 CAN 拦截器升级版：同时涵盖"测速模式"、"多行模式"与"文本处理"的跨语言清洗！
    if (m_lang == 0) // 0 代表英文模式
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
        if (msg.contains("提取换行"))
        {
            msg.replace("已开启", "ON");
            msg.replace("已关闭", "OFF");
            msg.replace("提取换行: ", "Extract Newlines: ");
            msg.replace("提取换行：", "Extract Newlines: ");
            msg.replace("提取换行", "Extract Newlines");
        }
        if (msg.contains("文本处理"))
        {
            msg.replace("已开启", "ON");
            msg.replace("已关闭", "OFF");
            msg.replace("文本处理: ", "HandleRichText: ");
            msg.replace("文本处理：", "HandleRichText: ");
            msg.replace("文本处理", "HandleRichText");
        }
    }
    else if (m_lang == 1) // 1 代表中文模式
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
        if (msg.contains("HandleRichText"))
        {
            msg.replace("ON", "已开启");
            msg.replace("OFF", "已关闭");
            msg.replace("HandleRichText: ", "文本处理: ");
            msg.replace("HandleRichText", "文本处理");
        }
        if (msg.contains("Extract Newlines"))
        {
            msg.replace("ON", "已开启");
            msg.replace("OFF", "已关闭");
            msg.replace("Extract Newlines: ", "提取换行: ");
            msg.replace("Extract Newlines", "提取换行");
        }
    }

    // 像经典模式一样直接 append，利用 Qt 富文本引擎渲染 HTML
    logArea->append(msg);

    // 限制行数，防止长时间运行导致内存溢出
    if (logArea->document()->blockCount() > 2000)
    {
        QTextCursor c(logArea->document());
        c.movePosition(QTextCursor::Start);
        for (int i = 0; i < 100; ++i)
            c.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        c.removeSelectedText();
    }

    /*
    QScrollBar *sb = logArea->verticalScrollBar();
    if (sb) {
        sb->setValue(sb->maximum());
    }
    */
}

// 选择术语表 | Select Glossary
void ModernWindow::onSelectGlossary()
{
    QString fn = QFileDialog::getOpenFileName(this, "Select", "", "_Substitutions (*.txt);;All (*)");
    if (!fn.isEmpty())
        addToGlossaryHistory(fn);

    // 如果服务器在运行，执行“即时生效”逻辑
    if (m_isServerRunning && m_server)
    {
        AppConfig cfg = getUiConfig();
        m_server->updateConfig(cfg);
        m_server->injectLog(m_lang == 0 ? "✅ New glossary applied." : "✅ 新术语表已应用。");
    }
}

// 术语表变更 | Glossary Changed
void ModernWindow::onGlossaryChanged()
{
    // 获取当前 UI 状态 | Get current UI state
    AppConfig currentCfg = getUiConfig();
    // 如果服务器正在运行，立即同步配置到 Server 实例
    // If server is running, sync config to Server instance immediately
    if (m_isServerRunning && m_server)
    {
        m_server->updateConfig(currentCfg);
        // 获取路径字符串，如果是空的显示提示
        // Get path string, show prompt if empty
        QString path = currentCfg.glossary_path;
        if (path.isEmpty())
            path = (m_lang == 1 ? "未选择" : "None");
        // 打印详细反馈日志 | Print detailed feedback log
        QString msg = (m_lang == 1)
                          ? QString("🔄 术语表已切换至：%1").arg(path)
                          : QString("🔄 Glossary switched to: %1").arg(path);
        m_server->injectLog(msg);
    }
}

// 加载配置 | Load Config
void ModernWindow::onLoadConfig()
{
    QString fn = QFileDialog::getOpenFileName(this, "Load", "", "*.ini");
    if (!fn.isEmpty())
    {
        AppConfig c = ConfigManager::loadConfig(fn);
        m_apiKeyMemoryEnabled = false;
        apiAddressCombo->setCurrentText(c.api_address);
        apiKeyEdit->setText(c.api_key);
        modelCombo->setCurrentText(c.model_name);
        portEdit->setText(QString::number(c.port));
        threadSpin->setValue(c.max_threads);
        tempSpin->setValue(c.temperature);
        contextSpin->setValue(c.context_num);
        prePromptEdit->setText(c.pre_prompt);
        chkGlossary->setChecked(c.enable_glossary);
        if (!chkLockSysPrompt->isChecked())
            systemPromptEdit->setText(c.system_prompt);
        if (!chkLockGlossary->isChecked())
        {
            glossaryCombo->clear();
            glossaryCombo->addItems(c.glossary_history);
            glossaryCombo->setCurrentText(c.glossary_path);
        }
        // 1. 多行模式 (CheckBox)：Qt 原生机制，仅在状态【发生改变】时触发 toggled 信号并打印日志
        chkBatch->setChecked(c.enable_batch);

        // 2. 测速模式 (Button + Bool)：手动模拟 Qt 的"仅改变时触发"逻辑
        bool oldDebugState = m_isDebugMode;  // 记录旧状态
        m_isDebugMode = c.enable_debug_mode; // 更新新状态

        // 刷新按钮视觉样式
        btnDebug->setProperty("debugOn", m_isDebugMode);
        btnDebug->style()->unpolish(btnDebug);
        btnDebug->style()->polish(btnDebug);
        // 🔥🔥🔥 [FIX] 核心修复：仅当状态【确实改变】时，才打印日志！ 🔥🔥🔥
        // 这样就和 chkBatch 的行为保持了 100% 的对称性：
        // - 如果两个都是从【关->关】，都不会打印，只有"配置已加载"。
        // - 如果两个都是从【关->开】，都会打印。
        if (oldDebugState != m_isDebugMode)
        {
            QString debugMsg = (m_lang == 1) ? QString("🛠️ 测速模式: %1").arg(m_isDebugMode ? "已开启" : "已关闭") : QString("🛠️ Speed Test Mode: %1").arg(m_isDebugMode ? "ON" : "OFF");
            m_server->injectLog(debugMsg);
        }

        m_lastApiBaseUrl = normalizeApiBaseUrl(apiAddressCombo->currentText());
        m_apiKeyMemoryEnabled = true;
        // 3. 打印配置加载完成日志
        m_server->injectLog(QString(LOG_CFG_LOADED[m_lang]) + fn);
    }
}

// 保存配置 | Save Config
void ModernWindow::onSaveConfig()
{
    persistCurrentApiKeyMemory();
    QString fn = QFileDialog::getSaveFileName(this, "Save", "config.ini", "*.ini");
    if (!fn.isEmpty())
    {
        ConfigManager::saveConfig(getUiConfig(), fn);
        m_server->injectLog(QString(LOG_CFG_SAVED[m_lang]) + fn);
    }
}

// 导出日志 | Export Log
void ModernWindow::onExportLog()
{
    QFile f("run_log.txt");
    if (f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream(&f) << logArea->toPlainText();
        m_server->injectLog(LOG_EXPORTED[m_lang]);
    }
}

// 打开自动翻译文件 | Open Auto Translation File
void ModernWindow::onOpenAutoTranslations()
{
    QString p = glossaryCombo->currentText();

    // 🌟 修复：拦截空路径或无效占位符，弹出优雅的玻璃提示框，彻底消除“沉默的失败”
    if (p.isEmpty() || p == "未选择" || p == "None")
    {
        GlassMessageBox::warning(this,
                                 (m_lang == 1 ? "⚠️ 提示" : "⚠️ Notice"),
                                 (m_lang == 1 ? "请先选择一个有效的术语表路径！\n程序需要基于该路径来定位自动翻译记录文件。"
                                              : "Please select a valid glossary path first!\nThe program needs it to locate the auto-translation file."),
                                 m_isDark, m_alpha, m_isRounded, m_glassRenderMode, m_hueShift, m_tintIntensity);
        return;
    }

    QString t = QFileInfo(p).absolutePath() + "/_AutoGeneratedTranslations.txt";
    if (!QFileInfo::exists(t))
    {
        GlassMessageBox::warning(this,
                                 (m_lang == 1 ? "⚠️ 文件未找到" : "⚠️ File Not Found"),
                                 (m_lang == 1 ? "未找到 _AutoGeneratedTranslations.txt。\n请确认游戏是否已经运行并生成了翻译。"
                                              : "Could not find _AutoGeneratedTranslations.txt."),
                                 m_isDark, m_alpha, m_isRounded, m_glassRenderMode, m_hueShift, m_tintIntensity);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(t));
}

// 术语表右键菜单 | Glossary Context Menu
void ModernWindow::onGlossaryContextMenu(const QPoint &pos)
{
    QMenu m(this);

    // 🌟 注入流光级动态透明样式
    m.setAttribute(Qt::WA_TranslucentBackground);
    m.setWindowFlags(m.windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    int r = m_isRounded ? 6 : 0;
    QString menuStyle = m_isDark ? QString("QMenu { background-color: rgba(30, 30, 35, %1); color: #EAEAEA; border: 1px solid rgba(255,255,255,30); border-radius: %2px; padding: 4px; margin: 0px; }"
                                           "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                                           "QMenu::item:selected { background-color: rgba(255, 140, 0, 180); color: white; }"
                                           "QMenu::item:disabled { color: rgba(128,128,128,150); }")
                                       .arg(m_alpha)
                                       .arg(r)
                                 : QString("QMenu { background-color: rgba(245, 250, 255, %1); color: #222222; border: 1px solid rgba(0,0,0,30); border-radius: %2px; padding: 4px; margin: 0px; }"
                                           "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                                           "QMenu::item:selected { background-color: rgba(148, 0, 211, 180); color: white; }"
                                           "QMenu::item:disabled { color: rgba(128,128,128,180); }")
                                       .arg(m_alpha)
                                       .arg(r);
    m.setStyleSheet(menuStyle);

    QAction *rm = m.addAction(STR_REMOVE_PATH[m_lang]);
    QAction *cl = m.addAction(STR_CLEAR_HISTORY[m_lang]);

    // 🔥 如果当前下拉框是空的，直接禁用删除动作
    if (glossaryCombo->currentText().isEmpty())
    {
        rm->setEnabled(false);
    }

    // 智能判断弹出位置
    QWidget *senderWidget = qobject_cast<QWidget *>(sender());
    QPoint globalPos;
    if (senderWidget)
    {
        globalPos = senderWidget->mapToGlobal(pos);
        globalPos.ry() += 5;
    }
    else
    {
        globalPos = QCursor::pos();
    }

    QAction *s = m.exec(globalPos);
    if (s == rm)
    {
        int index = glossaryCombo->currentIndex();
        if (index != -1)
        {
            glossaryCombo->removeItem(index);
            if (m_server)
                m_server->injectLog(m_lang == 1 ? "🧽 已从历史记录中移除该路径。" : "🧽 Removed current path from history.");
        }
        if (glossaryCombo->count() == 0 || index == -1)
            glossaryCombo->setEditText("");
    }
    else if (s == cl)
    {
        glossaryCombo->clear();
        glossaryCombo->setEditText("");
        if (m_server)
            m_server->injectLog(m_lang == 1 ? "🗑️ 术语表历史记录已清空。" : "🗑️ Glossary history cleared.");
    }
}

void ModernWindow::onApiComboContextMenu(const QPoint &pos)
{
    // 只在点击下拉框自身（而不是LineEdit）时响应
    QMenu menu(this);
    
    // 🌟 注入流光级动态透明样式
    menu.setAttribute(Qt::WA_TranslucentBackground);
    menu.setWindowFlags(menu.windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    int r = m_isRounded ? 6 : 0;
    QString menuStyle = m_isDark ? QString("QMenu { background-color: rgba(30, 30, 35, %1); color: #EAEAEA; border: 1px solid rgba(255,255,255,30); border-radius: %2px; padding: 4px; margin: 0px; }"
                                           "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                                           "QMenu::item:selected { background-color: rgba(255, 140, 0, 180); color: white; }"
                                           "QMenu::item:disabled { color: rgba(128,128,128,150); }")
                                       .arg(m_alpha)
                                       .arg(r)
                                 : QString("QMenu { background-color: rgba(245, 250, 255, %1); color: #222222; border: 1px solid rgba(0,0,0,30); border-radius: %2px; padding: 4px; margin: 0px; }"
                                           "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                                           "QMenu::item:selected { background-color: rgba(148, 0, 211, 180); color: white; }"
                                           "QMenu::item:disabled { color: rgba(128,128,128,180); }")
                                       .arg(m_alpha)
                                       .arg(r);
    menu.setStyleSheet(menuStyle);

    QAction *actDelete = menu.addAction(m_lang == 1 ? "🗑️ 删除此预设" : "🗑️ Delete Preset");
    
    QAction *sel = menu.exec(apiAddressCombo->mapToGlobal(pos));
    if (sel == actDelete) {
        QString currentUrl = apiAddressCombo->currentText();
        
        // 禁止删除加号以及内置预设
        bool isBuiltIn = false;
        int presetSize = sizeof(MODERN_PRESETS) / sizeof(LocalApiPreset);
        for (int p = 0; p < presetSize; ++p) {
            if (currentUrl == MODERN_PRESETS[p].url) {
                isBuiltIn = true;
                break;
            }
        }
        
        if (currentUrl == "+" || isBuiltIn) {
            GlassMessageBox::warning(this, m_lang == 1 ? "无法删除" : "Cannot Delete",
                               m_lang == 1 ? "内置预设与默认选项无法删除。" : "Built-in presets and default options cannot be deleted.",
                               m_isDark, m_alpha, m_isRounded, m_glassRenderMode, m_hueShift, m_tintIntensity);
            return;
        }

        int reply = QMessageBox::question(this, m_lang == 1 ? "确认删除" : "Confirm Deletion",
                                        (m_lang == 1 ? "确定要删除以下自定义 API 预设吗？\n" : "Are you sure you want to delete this custom API preset?\n") + currentUrl,
                                        QMessageBox::Yes | QMessageBox::No);
                                        
        if (reply == QMessageBox::Yes) {
            AppConfig cfg = ConfigManager::loadConfig();
            const QString normalizedCurrentUrl = normalizeApiBaseUrl(currentUrl);
            const QString fallbackUrl = QString(MODERN_PRESETS[0].url);

            cfg.custom_api_urls.removeAll(currentUrl);

            // 若删除的是当前配置中的URL，先回退到默认预设，避免后续保存把已删预设重新写回配置。
            if (normalizeApiBaseUrl(cfg.api_address) == normalizedCurrentUrl)
            {
                cfg.api_address = fallbackUrl;
                cfg.api_key = ConfigManager::loadApiKeyForBaseUrl(fallbackUrl, "config.ini");
                cfg.model_name = ConfigManager::loadModelForBaseUrl(fallbackUrl, "config.ini");
            }

            ConfigManager::saveConfig(cfg);

            // 同步删除与该URL关联的Key和Model映射，防止配置污染。
            ConfigManager::removeApiKeyForBaseUrl(currentUrl, "config.ini");
            ConfigManager::removeModelForBaseUrl(currentUrl, "config.ini");
            ConfigManager::removePresetNameForBaseUrl(currentUrl, "config.ini");
            
            // 从UI中移除
            int idx = apiAddressCombo->findText(currentUrl);
            if (idx != -1) {
                apiAddressCombo->removeItem(idx);
                apiAddressCombo->setCurrentIndex(0);
            }
        }
    }
}

// 日志右键菜单 | Log Context Menu
void ModernWindow::onLogContextMenu(const QPoint &pos)
{
    QMenu *m = logArea->createStandardContextMenu();

    // 🌟 注入流光级动态透明样式
    m->setAttribute(Qt::WA_TranslucentBackground);
    m->setWindowFlags(m->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    int r = m_isRounded ? 6 : 0;
    QString menuStyle = m_isDark ? QString("QMenu { background-color: rgba(30, 30, 35, %1); color: #EAEAEA; border: 1px solid rgba(255,255,255,30); border-radius: %2px; padding: 4px; margin: 0px; }"
                                           "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                                           "QMenu::item:selected { background-color: rgba(255, 140, 0, 180); color: white; }"
                                           "QMenu::item:disabled { color: rgba(128,128,128,150); }")
                                       .arg(m_alpha)
                                       .arg(r)
                                 : QString("QMenu { background-color: rgba(245, 250, 255, %1); color: #222222; border: 1px solid rgba(0,0,0,30); border-radius: %2px; padding: 4px; margin: 0px; }"
                                           "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                                           "QMenu::item:selected { background-color: rgba(148, 0, 211, 180); color: white; }"
                                           "QMenu::item:disabled { color: rgba(128,128,128,180); }")
                                       .arg(m_alpha)
                                       .arg(r);
    m->setStyleSheet(menuStyle);

    m->addSeparator();

    QAction *cl = m->addAction(STR_CLEAR_LOG[m_lang]);
    connect(cl, &QAction::triggered, []()
            { LogManager::instance().clear(); });

    m->exec(logArea->mapToGlobal(pos));
    delete m;
}

// 清除上下文 | Clear Context
void ModernWindow::onClearContext()
{
    if (m_server)
        m_server->clearAllContexts();
}

// 添加到术语表历史 | Add to Glossary History
void ModernWindow::addToGlossaryHistory(const QString &p)
{
    if (p.isEmpty())
        return;
    QStringList l;
    for (int i = 0; i < glossaryCombo->count(); ++i)
        l << glossaryCombo->itemText(i);
    l.removeAll(p);
    l.insert(0, p);
    while (l.size() > 10)
        l.removeLast();
    glossaryCombo->clear();
    glossaryCombo->addItems(l);
    glossaryCombo->setCurrentIndex(0);
    if (glossaryCombo->lineEdit())
        glossaryCombo->lineEdit()->setCursorPosition(0);
}

// 自定义绘制事件 (背景渐变/光晕) | Custom Paint Event (Background Gradient/Glow)
void ModernWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRect rect = this->rect();
    int r = m_isRounded ? 20 : 0;

    // 🎨 根据渲染模式选择背景效果
    if (m_glassRenderMode == GlassRenderMode::Frosted)
    {
        // 🌫️ 新版：原生毛玻璃渐变效果
        // 计算透明度因子
        float a = m_alpha / 255.0f;

        // 0. 有色毛玻璃的“深色底盘” (注入基础颜色调色)
        // 使用 m_tintIntensity (0~100) 混合无色和有色底盘
        float tint = m_tintIntensity / 100.0f;
        QColor colorlessDark(25, 25, 30, m_alpha);
        QColor tintedDark(20, 15, 30, m_alpha);
        QColor colorlessLight(245, 245, 250, m_alpha);
        QColor tintedLight(245, 240, 250, m_alpha);

        QColor frostedBase;
        if (m_isDark) {
            frostedBase.setRed(colorlessDark.red() + (tintedDark.red() - colorlessDark.red()) * tint);
            frostedBase.setGreen(colorlessDark.green() + (tintedDark.green() - colorlessDark.green()) * tint);
            frostedBase.setBlue(colorlessDark.blue() + (tintedDark.blue() - colorlessDark.blue()) * tint);
            frostedBase.setAlpha(m_alpha);
        } else {
            frostedBase.setRed(colorlessLight.red() + (tintedLight.red() - colorlessLight.red()) * tint);
            frostedBase.setGreen(colorlessLight.green() + (tintedLight.green() - colorlessLight.green()) * tint);
            frostedBase.setBlue(colorlessLight.blue() + (tintedLight.blue() - colorlessLight.blue()) * tint);
            frostedBase.setAlpha(m_alpha);
        }
        
        p.setBrush(frostedBase);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect, r, r);

        // 1. 左侧大面积青蓝/孔雀绿光晕 (Tinted Cyan Glow)
        QRadialGradient cyanGlow(rect.width() * 0.1, rect.height() * 0.4, rect.width() * 0.8);
        if (m_isDark) {
            cyanGlow.setColorAt(0.0, shiftHue(QColor(0, 160, 210, 80 * a * tint), m_hueShift));
            cyanGlow.setColorAt(1.0, QColor(0, 160, 210, 0));
        } else {
            cyanGlow.setColorAt(0.0, shiftHue(QColor(0, 200, 255, 110 * a * tint), m_hueShift));
            cyanGlow.setColorAt(1.0, QColor(0, 200, 255, 0));
        }
        p.setBrush(cyanGlow);
        p.drawRoundedRect(rect, r, r);

        // 2. 右侧大面积橙色/琥珀色光晕 (Tinted Orange Glow)
        QRadialGradient orangeGlow(rect.width() * 0.8, rect.height() * 0.2, rect.width() * 0.7);
        if (m_isDark) {
            orangeGlow.setColorAt(0.0, shiftHue(QColor(210, 95, 10, 75 * a * tint), m_hueShift));
            orangeGlow.setColorAt(1.0, QColor(210, 95, 10, 0));
        } else {
            orangeGlow.setColorAt(0.0, shiftHue(QColor(255, 150, 50, 90 * a * tint), m_hueShift));
            orangeGlow.setColorAt(1.0, QColor(255, 150, 50, 0));
        }
        p.setBrush(orangeGlow);
        p.drawRoundedRect(rect, r, r);

        // 3. 底部大面积紫红/洋红光晕 (Tinted Purple Glow)
        QRadialGradient purpleGlow(rect.width() * 0.5, rect.height() * 0.9, rect.width() * 0.8);
        if (m_isDark) {
            purpleGlow.setColorAt(0.0, shiftHue(QColor(130, 10, 150, 85 * a * tint), m_hueShift));
            purpleGlow.setColorAt(1.0, QColor(130, 10, 150, 0));
        } else {
            purpleGlow.setColorAt(0.0, shiftHue(QColor(220, 100, 240, 100 * a * tint), m_hueShift));
            purpleGlow.setColorAt(1.0, QColor(220, 100, 240, 0));
        }
        p.setBrush(purpleGlow);
        p.drawRoundedRect(rect, r, r);

        // 3.5 全局表面统一光泽 (对角线略微提亮，维持玻璃的高光质感)
        if (m_alpha > 40)
        {
            QLinearGradient diagGlint(rect.topLeft(), rect.bottomRight());
            if (m_isDark)
            {
                diagGlint.setColorAt(0.0, QColor(255, 255, 255, 25 * a));
                diagGlint.setColorAt(0.2, QColor(255, 255, 255, 0));
                diagGlint.setColorAt(0.8, QColor(255, 255, 255, 0));
                diagGlint.setColorAt(1.0, QColor(255, 255, 255, 15 * a));
            }
            else
            {
                diagGlint.setColorAt(0.0, QColor(255, 255, 255, 120 * a));
                diagGlint.setColorAt(0.3, QColor(255, 255, 255, 0));
                diagGlint.setColorAt(0.7, QColor(255, 255, 255, 0));
                diagGlint.setColorAt(1.0, QColor(255, 255, 255, 50 * a));
            }
            p.setBrush(diagGlint);
            p.drawRoundedRect(rect, r, r);
        }

        // 4. 内发光描边 (Rim Light)
        if (m_alpha > 50)
        {
            QPainterPath path;
            path.addRoundedRect(rect, r, r);
            QPainterPath innerPath;
            innerPath.addRoundedRect(rect.adjusted(1, 1, -1, -1), r - 1, r - 1);
            path = path.subtracted(innerPath);

            QLinearGradient rimGrad(rect.topLeft(), rect.bottomRight());
            if (m_isDark)
            {
                rimGrad.setColorAt(0.0, QColor(255, 255, 255, 60 * a));
                rimGrad.setColorAt(0.3, QColor(255, 255, 255, 5 * a));
                rimGrad.setColorAt(0.7, QColor(255, 255, 255, 0));
                rimGrad.setColorAt(1.0, QColor(255, 255, 255, 20 * a));
            }
            else
            {
                rimGrad.setColorAt(0.0, QColor(255, 255, 255, 200 * a));
                rimGrad.setColorAt(0.3, QColor(255, 255, 255, 50 * a));
                rimGrad.setColorAt(0.7, QColor(255, 255, 255, 0));
                rimGrad.setColorAt(1.0, QColor(255, 255, 255, 80 * a));
            }
            p.fillPath(path, rimGrad);
        }

        // 5. 极细的外边框
        QColor borderColor = m_isDark ? QColor(0, 0, 0, 100 * a) : QColor(0, 0, 0, 25 * a);
        p.setPen(QPen(borderColor, 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(rect, r, r);
    }
    else
    {
        // ✨ 旧版：伪毛玻璃发光效果
        // 彻底同步背景色，消除色差（统一为深邃黑底 QColor(30, 30, 35)）
        if (m_isDark)
        {
            p.setBrush(QColor(30, 30, 35, qMin(255, m_alpha)));
        }
        else
        {
            p.setBrush(QColor(255, 255, 255, qMin(255, m_alpha)));
        }
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect, r, r);

        // 旧版内部发光效果
        int glowLayers = 5;
        QColor glowColor = m_isDark ? QColor(255, 140, 0) : QColor(100, 40, 20);
        int startAlpha = m_isDark ? 130 : 120;

        p.setBrush(Qt::NoBrush);
        for (int i = 0; i < glowLayers; ++i)
        {
            QColor col = glowColor;
            col.setAlpha(startAlpha * (glowLayers - i) / glowLayers);
            p.setPen(QPen(col, 1));
            p.drawRoundedRect(rect.adjusted(i, i, -i, -i), r - i, r - i);
        }

        // 外边框
        QPen borderPen(m_isDark ? QColor(255, 180, 100, 80) : QColor(148, 0, 211, 100), 1);
        p.setPen(borderPen);
        p.drawRoundedRect(rect, r, r);
    }
}

void ModernWindow::updateCursorShape(const QPoint &pos)
{
    // 如果正在拖拽移动窗口，或者按下了鼠标左键，则不改变光标
    if (m_isDragging || (QGuiApplication::mouseButtons() & Qt::LeftButton))
        return;

    const int border = 5; // 边缘触发判定厚度 (与 nativeEvent 中的 RESIZE_BORDER 保持一致，避免误触按钮)
    int edge = 0;

    if (pos.x() <= border)
        edge |= 1; // 左边缘
    if (pos.x() >= width() - border)
        edge |= 2; // 右边缘
    if (pos.y() <= border)
        edge |= 4; // 上边缘
    if (pos.y() >= height() - border)
        edge |= 8; // 下边缘

    m_resizeEdge = edge;

    // 完美映射八向拉伸光标
    if (edge == 1 || edge == 2)
        setCursor(Qt::SizeHorCursor); // 左右 ↔
    else if (edge == 4 || edge == 8)
        setCursor(Qt::SizeVerCursor); // 上下 ↕
    else if (edge == 5 || edge == 10)
        setCursor(Qt::SizeFDiagCursor); // 左上/右下 ⤡
    else if (edge == 6 || edge == 9)
        setCursor(Qt::SizeBDiagCursor); // 右上/左下 ⤢
    else
        setCursor(Qt::ArrowCursor); // 恢复正常
}

void ModernWindow::leaveEvent(QEvent *event)
{
    // 鼠标离开窗口边缘时，确保光标恢复正常
    if (!m_isDragging && m_resizeEdge == 0)
        setCursor(Qt::ArrowCursor);
    QMainWindow::leaveEvent(event);
}

void ModernWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        updateCursorShape(e->pos()); // 按下瞬间再次确认边缘状态

        if (m_resizeEdge != 0)
        {
            // ✨ 进入拉伸模式：锁定初始几何信息
            m_dragStartGeom = geometry();
            m_dragStartGlobalPos = e->globalPosition().toPoint();
            m_isDragging = false; // 绝对禁止移动冲突
        }
        else if (e->pos().y() <= 60)
        {
            // 进入移动模式：仅限顶部 60px 拖拽
            m_isDragging = true;
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
        }
    }
}

void ModernWindow::mouseMoveEvent(QMouseEvent *e)
{
    // 1. 悬停探测 (未按鼠标左键时，灵敏切换光标)
    if (!(e->buttons() & Qt::LeftButton))
    {
        updateCursorShape(e->pos());
        return;
    }

    // 2. ✨ 拉伸窗口逻辑 (八向精准计算)
    if (m_resizeEdge != 0)
    {
        QRect g = m_dragStartGeom;
        QPoint diff = e->globalPosition().toPoint() - m_dragStartGlobalPos;

        const int minW = 500; // 最小宽度保护
        const int minH = 600; // 最小高度保护

        // 🌟 CAN 核心：获取当前屏幕的可用区域（自动刨除底部的任务栏和任何侧边停靠栏）
        QRect screenRect = this->screen()->availableGeometry();

        if (m_resizeEdge & 1)
        { // 拖动左边
            int newX = g.left() + diff.x();
            if (newX < screenRect.left())
                newX = screenRect.left(); // 撞墙停止
            if (g.right() - newX >= minW)
                g.setLeft(newX);
        }
        if (m_resizeEdge & 2)
        { // 拖动右边
            int newR = g.right() + diff.x();
            if (newR > screenRect.right())
                newR = screenRect.right(); // 撞墙停止
            if (newR - g.left() >= minW)
                g.setRight(newR);
        }
        if (m_resizeEdge & 4)
        { // 拖动上边
            int newY = g.top() + diff.y();
            if (newY < screenRect.top())
                newY = screenRect.top(); // 撞墙停止
            if (g.bottom() - newY >= minH)
                g.setTop(newY);
        }
        if (m_resizeEdge & 8)
        { // 拖动下边
            int newB = g.bottom() + diff.y();
            if (newB > screenRect.bottom())
                newB = screenRect.bottom(); // 🌟 碰到任务栏即刻停止！
            if (newB - g.top() >= minH)
                g.setBottom(newB);
        }

        setGeometry(g); // 实时应用新尺寸
        return;
    }

    // 3. 移动窗口逻辑
    if (m_isDragging)
    {
        move(e->globalPosition().toPoint() - m_dragPos);
    }
}

void ModernWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        m_isDragging = false;
        m_resizeEdge = 0;
        updateCursorShape(e->pos()); // 松开时更新当前光标
    }
    QMainWindow::mouseReleaseEvent(e);
}

// 显示事件 (入场动画) | Show Event (Entry Animation)
// ==========================================
// 💎 修正后的 showEvent：移除位移推出，改为原地纯净淡入
// ==========================================
void ModernWindow::showEvent(QShowEvent *event)
{

#ifdef Q_OS_WIN

    QTimer::singleShot(10, this, [this]()
                       {
        HWND hwnd = (HWND)this->winId();
        
        // 1. 赋予系统级可调节边框与标题栏属性 (激活 Aero Snap 与原生阻尼)
        DWORD style = GetWindowLong(hwnd, GWL_STYLE);
        SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_MAXIMIZEBOX);
        
        // 2. 禁用Win11原生圆角效果，防止四角出现阴影
        int cornerPreference = DWMWCP_DONOTROUND;
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
        
        // 3. 核心：强迫 Windows 立即重新计算窗口非客户区！
        // SWP_FRAMECHANGED 会立刻触发你写的 WM_NCCALCSIZE，让 DWM 彻底丢弃圆角缓存！
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, 
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); });
#endif

    // 0. 保护未保存的当前状态（透明度、文本），并防止与操作系统的恢复动画起冲突！
    if (event->spontaneous())
    {
        QMainWindow::showEvent(event);
        return;
    }

    // 1. 基础逻辑：加载配置与同步 Server
    loadConfigToUi();

    if (m_server)
    {
        AppConfig liveCfg = m_server->getConfig();
        if (!liveCfg.glossary_path.isEmpty())
        {
            if (glossaryCombo->findText(liveCfg.glossary_path) == -1)
                glossaryCombo->addItem(liveCfg.glossary_path);
            glossaryCombo->setCurrentText(liveCfg.glossary_path);
        }
        chkGlossary->setChecked(liveCfg.enable_glossary);
        chkLockGlossary->setChecked(liveCfg.lock_glossary);
        chkLockSysPrompt->setChecked(liveCfg.lock_system_prompt);
        if (m_lang != liveCfg.language)
        {
            m_lang = liveCfg.language;
            updateUIText();
        }
        if (glossaryCombo->lineEdit())
            glossaryCombo->lineEdit()->setCursorPosition(0);
    }

    // ============================================================
    // 🚀 CAN FIX: 彻底移除 Y 轴的 40px 偏移滑动，仅保留透明度淡入
    // 这样当 main.cpp 播放闪光掩护时，流光窗口就在原地浮现，稳如泰山！
    // ============================================================

    if (this->property("bypass_fade").toBool())
    {
        this->setWindowOpacity(1.0);
        this->setProperty("bypass_fade", false); // 用完即焚
    }
    else
    {

        this->setWindowOpacity(0.0); // 初始设为全透明

        // 仅保留淡入动画 (时长 400ms，完美配合 main.cpp 的闪烁掩护)
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
        fadeAnim->setDuration(400);
        fadeAnim->setStartValue(0.0);
        fadeAnim->setEndValue(1.0);
        fadeAnim->setEasingCurve(QEasingCurve::OutQuad);
        fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QMainWindow::showEvent(event);
}

// 透明度滑块变更 | Opacity Slider Changed
void ModernWindow::onOpacityChange(int v)
{
    m_alpha = v;
    m_storedModernOpacity = v; // 🔥 让保护值与滑块实时同步

    // 🔥 完美简化：直接调用我们在 .h 写好的批量更新函数，清爽至极！
    updateComboEnv();

    // 🌫️ 毛玻璃效果：更新所有GlassCard的透明度
    for (GlassCard *card : m_glassCards)
    {
        if (card)
        {
            card->setAlpha(m_alpha);
        }
    }

    // 🔥 将透明度同步传达给打开的抽屉
    if (m_glossaryDrawer)
    {
        static_cast<GlossaryDrawer *>(m_glossaryDrawer.data())->setAlpha(m_alpha);
    }

    update(); // 重绘主窗口 | Repaint main window
    
    // 🎨 调色板实时刷新
    if (m_palettePopup) {
        m_palettePopup->update();
    }
}

// 切换语言 | Toggle Language
void ModernWindow::toggleLanguage()
{
    // 使用 smoothSwitch 包裹语言切换逻辑
    // Wrap language switch logic with smoothSwitch
    smoothSwitch([this]()
                 {
                     m_lang = (m_lang == 0) ? 1 : 0;
                     // 更新界面所有文本 | Update all UI text
                     updateUIText();
                     // 同步配置到服务器（如果在运行）
                     // Sync config to server (if running)
                     if (m_server)
                     {
                         AppConfig c = m_server->getConfig();
                         c.language = m_lang;
                         m_server->updateConfig(c);
                     }

                     if (m_glossaryDrawer)
                     {
                         static_cast<GlossaryDrawer *>(m_glossaryDrawer.data())->updateLanguage(m_lang);
                     } });
}

// 切换回经典模式 | Switch back to Classic Mode
void ModernWindow::onSwitchClicked()
{
    // 🔥 修复：在切换回经典模式前，强制销毁悬浮的术语表编辑抽屉
    // 这里我们直接调用 close() 而不是 animateClose()，让它瞬间消失，配合主窗口的淡出更加利落！
    if (m_glossaryDrawer)
    {
        m_glossaryDrawer->close();
    }

    if (m_server)
        m_server->updateConfig(getUiConfig());

    emit requestClassicMode();
}

// 🌊 专属局部类：全息扫描覆盖层 (Holographic Scanner Wave)
// ==========================================
class RippleOverlay : public QWidget
{
public:
    QPixmap m_pixmap;
    QPoint m_center;
    float m_radius = 0;
    bool m_isDarkTarget = true; // 用于决定光波的色调

    RippleOverlay(const QPixmap &pix, const QPoint &center, QWidget *parent = nullptr)
        : QWidget(parent), m_pixmap(pix), m_center(center)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        // 1. 物理剪裁旧界面
        QPainterPath path;
        path.addRect(rect());
        path.addEllipse(QPointF(m_center), m_radius, m_radius);
        p.setClipPath(path);

        // 🔥 关键修正 1：防止新 UI 从半透明背景“透底”混色！
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawPixmap(0, 0, m_pixmap);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver); // 恢复正常模式

        p.setClipping(false);

        // ==========================================
        // 🌟 核心视觉升级：全息能量洗刷波
        // ==========================================
        if (m_radius > 0)
        {
            float waveFront = m_radius + 5.0f;
            float waveTail = std::max(0.0f, m_radius - 70.0f);
            if (waveFront <= 0)
                return;

            QRadialGradient grad(m_center, waveFront);
            float cutStop = m_radius / waveFront;
            float tailStop = waveTail / waveFront;

            QColor coreColor = m_isDarkTarget ? QColor(255, 120, 0, 255) : QColor(148, 0, 211, 220);
            QColor midColor = m_isDarkTarget ? QColor(255, 120, 0, 90) : QColor(148, 0, 211, 90);

            grad.setColorAt(1.0, Qt::transparent);
            grad.setColorAt(qMin(1.0f, cutStop + 0.015f), coreColor);
            grad.setColorAt(cutStop, coreColor);

            if (tailStop > 0 && tailStop < cutStop)
            {
                grad.setColorAt(std::max(tailStop, cutStop - 0.15f), midColor);
                grad.setColorAt(tailStop, Qt::transparent);
            }
            grad.setColorAt(0.0, Qt::transparent);

            p.setBrush(grad);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(m_center), waveFront, waveFront);
        }
    }
};

// ==========================================
// 🎨 平滑切换动画 (一触即发·扫描洗刷 | Holographic Reveal)
// ==========================================
void ModernWindow::smoothSwitch(std::function<void()> changeLogic)
{
    // 0. 防连点保护，防止多重波纹排队错乱
    if (this->property("is_switching").toBool())
        return;
    this->setProperty("is_switching", true);

    // 1. 获取震中 (全局坐标)
    QPoint globalPos = QCursor::pos();

    // 计算相对于主窗口的震中
    QPoint epicenter = this->mapFromGlobal(globalPos);

    // 2. 截取旧境并盖上蒙版
    RippleOverlay *overlay = new RippleOverlay(this->grab(), epicenter, this);
    overlay->setGeometry(this->rect());
    // 防止被新下发的样式表污染边框
    overlay->setStyleSheet("background: transparent; border: none;");
    overlay->show();
    overlay->raise();

    RippleOverlay *drawerOverlay = nullptr;
    if (m_glossaryDrawer)
    {
        // 关键点：对于副窗口，震中坐标必须映射到副窗口的坐标系中
        // 即使点击点在主窗口，映射后的 drawerCenter 可能是 (-500, 100)，这正是我们要的“波源在外部”的效果
        QPoint drawerCenter = m_glossaryDrawer->mapFromGlobal(globalPos);
        drawerOverlay = new RippleOverlay(m_glossaryDrawer->grab(), drawerCenter, m_glossaryDrawer.data());
        drawerOverlay->setGeometry(m_glossaryDrawer->rect());
        drawerOverlay->setStyleSheet("background: transparent; border: none;");
        drawerOverlay->show();
        drawerOverlay->raise();
    }

    // ============================================================
    // 🚨 核心排队修正 2：强制优先绘制旧画面！
    // ============================================================
    overlay->repaint();
    if (drawerOverlay)
        drawerOverlay->repaint();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // ============================================================
    // 🚨 核心排队修正 3：将“耗时的 UI 替换”挂起到下一帧
    // ============================================================
    QTimer::singleShot(15, this, [this, overlay, drawerOverlay, globalPos, changeLogic]()
                       {
        // 3. 瞬间执行极其耗时的切换逻辑
        changeLogic();

        // 4. 将新的主题状态注入光波覆盖层
        overlay->m_isDarkTarget = this->m_isDark;
        if (drawerOverlay) drawerOverlay->m_isDarkTarget = this->m_isDark;

        // 强行让底层主窗口将新 UI 画入显存
        this->repaint();

        // ============================================================
        // 🔥 CAN FIX: 全局覆盖半径计算逻辑
        // 以前只计算了主窗口，现在我们需要计算“震源”到“所有窗口最远角落”的距离
        // ============================================================
        auto calculateMaxDistance = [](QWidget* w, QPoint globalP) -> double {
            if (!w) return 0.0;
            QPoint localP = w->mapFromGlobal(globalP);
            int width = w->width();
            int height = w->height();
            
            // 计算点击点到矩形四个角的欧几里得距离，取最大值
            double d1 = std::hypot(localP.x(), localP.y());                   // 左上
            double d2 = std::hypot(width - localP.x(), localP.y());           // 右上
            double d3 = std::hypot(width - localP.x(), height - localP.y());  // 右下
            double d4 = std::hypot(localP.x(), height - localP.y());          // 左下
            
            return std::max({d1, d2, d3, d4});
        };

        // 计算主窗口所需半径
        double radiusMain = calculateMaxDistance(this, globalPos);
        
        // 计算抽屉窗口所需半径 (如果存在)
        double radiusDrawer = 0.0;
        if (m_glossaryDrawer) {
            radiusDrawer = calculateMaxDistance(m_glossaryDrawer.data(), globalPos);
        }

        // 🔥 最终半径取两者之大，并额外加 50px 缓冲，确保波纹彻底扫出屏幕
        double finalMaxRadius = std::max(radiusMain, radiusDrawer) + 50.0;

        // 5. 驱动「全息扫描」引擎
        QVariantAnimation *anim = new QVariantAnimation(this);
        // 半径变大了，为了保持视觉速度一致，如果距离特别远，稍微增加一点点时长
        int duration = (finalMaxRadius > 1000) ? 650 : 550; 
        
        anim->setDuration(duration); 
        anim->setStartValue(0.0f);
        anim->setEndValue((float)finalMaxRadius);
        
        anim->setEasingCurve(QEasingCurve::InOutCubic); 

        connect(anim, &QVariantAnimation::valueChanged, [overlay, drawerOverlay ](const QVariant& val){
            float r = val.toFloat();
            overlay->m_radius = r;
            overlay->update();
            // 两个 Overlay 共享同一个半径 r
            // 因为它们的 m_center 都是基于同一个 globalPos 算出来的相对坐标
            // 所以视觉上波纹会无缝连接，像是一个巨大的圆环穿过两个分离的窗口
            if(drawerOverlay) {
                drawerOverlay->m_radius = r;
                drawerOverlay->update();
            }
        });

        connect(anim, &QVariantAnimation::finished, [this, overlay, drawerOverlay, anim](){
            overlay->deleteLater();
            if(drawerOverlay) drawerOverlay->deleteLater();
            anim->deleteLater();
            
            this->setProperty("is_switching", false); 
        });

        anim->start(); });
}

void ModernWindow::onEditGlossaryClicked()
{
    // 1. 如果抽屉已经处于打开状态，则触发关闭
    if (m_glossaryDrawer)
    {
        static_cast<GlossaryDrawer *>(m_glossaryDrawer.data())->animateClose();
        return;
    }

    // 2. 获取当前术语表路径
    QString path = glossaryCombo->currentText();
    if (path.isEmpty() || path == "未选择" || path == "None")
    {
        GlassMessageBox::warning(this,
                                 m_lang == 1 ? "⚠️ 提示" : "⚠️ Notice",
                                 m_lang == 1 ? "请先选择一个术语表！" : "Please select a glossary first!",
                                 m_isDark, m_alpha, m_isRounded, m_glassRenderMode, m_hueShift, m_tintIntensity);
        return;
    }

    if (!QFileInfo::exists(path))
    {
        GlassMessageBox::warning(this,
                                 m_lang == 1 ? "❌ 错误" : "❌ Error",
                                 m_lang == 1 ? "该术语表文件不存在！" : "Glossary file does not exist!",
                                 m_isDark, m_alpha, m_isRounded, m_glassRenderMode, m_hueShift, m_tintIntensity);
        return;
    }

    // 3. 实例化抽屉面板 (🔥 重点：补齐了 m_isRounded 参数！)
    GlossaryDrawer *drawer = new GlossaryDrawer(this, path, m_isDark, m_alpha, m_isRounded, m_lang, m_server);
    drawer->setRenderMode(m_glassRenderMode); // 🎨 同步传入渲染模式
    drawer->setHueShift(m_hueShift);
    drawer->setTintIntensity(m_tintIntensity);
    m_glossaryDrawer = drawer;
    m_glossaryDrawer->show();
}

// 切换主题 | Toggle Theme
void ModernWindow::toggleTheme()
{
    smoothSwitch([this]()
                 {
        m_isDark = !m_isDark;
        // 1. 设置 CSS (补齐 m_isRounded)
        setStyleSheet(getModernStyle(m_isDark, m_isRounded));
        
        style()->unpolish(this);
        style()->polish(this);
        
        QList<QWidget*> widgets = this->findChildren<QWidget*>();
        for (QWidget* w : widgets) {
            style()->unpolish(w);
            style()->polish(w);
        }
        
        QList<GlassCard*> cards = this->findChildren<GlassCard*>();
        for (GlassCard* card : cards) {
            card->setTheme(m_isDark);
        }
        
        updateComboEnv();
        
        if (m_glossaryDrawer) {
            static_cast<GlossaryDrawer*>(m_glossaryDrawer.data())->updateEnv(m_isDark, m_alpha, m_isRounded);
        }
        
        update(); 
        
        // 🔥 CAN 绝杀：强制重置光标，瞬间抹杀 Qt 视口偏移 Bug！
        resetAllCursors(); 
        updatePowerButtonState(m_isServerRunning); });
}

// 切换形状 | Toggle Shape
void ModernWindow::toggleShape()
{
    smoothSwitch([this]()
                 {
        m_isRounded = !m_isRounded;
        m_storedIsRounded = m_isRounded; 
        
        setStyleSheet(getModernStyle(m_isDark, m_isRounded));
        style()->unpolish(this); style()->polish(this);
        
        QList<QWidget*> widgets = this->findChildren<QWidget*>();
        for (QWidget* w : widgets) { style()->unpolish(w); style()->polish(w); }
        
        QList<GlassCard*> cards = this->findChildren<GlassCard*>();
        for (GlassCard* card : cards) card->setRounded(m_isRounded);
        
        updateComboEnv();
        
        if (m_glossaryDrawer) {
            static_cast<GlossaryDrawer*>(m_glossaryDrawer.data())->updateEnv(m_isDark, m_alpha, m_isRounded);
        }
        
        update();
        
        // 🎨 调色板实时刷新
        if (m_palettePopup) {
            m_palettePopup->update();
        }
        
        // 🔥 CAN 绝杀：强制重置光标，瞬间抹杀 Qt 视口偏移 Bug！
        resetAllCursors(); 
        updatePowerButtonState(m_isServerRunning); });
}

// 切换测速模式 | Toggle Debug Mode
void ModernWindow::toggleDebugMode()
{
    m_isDebugMode = !m_isDebugMode;

    btnDebug->setProperty("debugOn", m_isDebugMode);
    btnDebug->style()->unpolish(btnDebug);
    btnDebug->style()->polish(btnDebug);

    updateUIText();

    // 👇👇👇 加上这段修复代码：打印状态日志 👇👇👇
    QString msg = (m_lang == 1) ? QString("🛠️ 测速模式: %1").arg(m_isDebugMode ? "已开启" : "已关闭") : QString("🛠️ Speed Test Mode: %1").arg(m_isDebugMode ? "ON" : "OFF");
    LogManager::instance().addLog(msg);

    if (m_server && m_isServerRunning)
    {
        m_server->updateConfig(getUiConfig());
    }
}

// 🎨 切换毛玻璃渲染模式 | Toggle Glass Render Mode
void ModernWindow::toggleGlassStyle()
{
    smoothSwitch([this]()
                 {
        // 切换渲染模式
        if (m_glassRenderMode == GlassRenderMode::Frosted)
        {
            m_glassRenderMode = GlassRenderMode::Legacy;
        }
        else
        {
            m_glassRenderMode = GlassRenderMode::Frosted;
        }

        // 根据模式控制调色板按钮的出现与退出动画
        if (btnPalette) {
            bool targetVisible = (m_glassRenderMode == GlassRenderMode::Frosted);
            
            // 确保取消最小尺寸锁定，允许宽度动画
            btnPalette->setMinimumWidth(0); 

            QPropertyAnimation *widthAnim = new QPropertyAnimation(btnPalette, "maximumWidth");
            widthAnim->setDuration(450); 
            widthAnim->setEasingCurve(QEasingCurve::InOutCubic);

            QGraphicsOpacityEffect *eff = qobject_cast<QGraphicsOpacityEffect*>(btnPalette->graphicsEffect());
            if (!eff) {
                eff = new QGraphicsOpacityEffect(btnPalette);
                btnPalette->setGraphicsEffect(eff);
            }
            
            QPropertyAnimation *fadeAnim = new QPropertyAnimation(eff, "opacity");
            fadeAnim->setDuration(350);

            if (targetVisible) {
                btnPalette->setVisible(true);
                widthAnim->setStartValue(btnPalette->width());
                widthAnim->setEndValue(38);
                fadeAnim->setStartValue(eff->opacity());
                fadeAnim->setEndValue(1.0);
            } else {
                widthAnim->setStartValue(btnPalette->width());
                widthAnim->setEndValue(0);
                fadeAnim->setStartValue(eff->opacity());
                fadeAnim->setEndValue(0.0);
                
                // 动画结束后隐藏防止占用点击事件
                connect(widthAnim, &QPropertyAnimation::finished, btnPalette, [this]() {
                    if (m_glassRenderMode != GlassRenderMode::Frosted && btnPalette) {
                        btnPalette->setVisible(false);
                    }
                });
            }
            
            widthAnim->start(QAbstractAnimation::DeleteWhenStopped);
            fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
        }
        
        // 若切换到旧版发光模式，则关闭可能已打开的调色板
        if (m_glassRenderMode == GlassRenderMode::Legacy && m_palettePopup) {
            m_palettePopup->close();
        } else if (m_palettePopup) {
            // 若切换到 Frosted，立即刷新调色板样式
            m_palettePopup->update();
        }
        
        // 分发给所有 GlassCard
        for (GlassCard *card : m_glassCards) {
            if (card) {
                card->setRenderMode(m_glassRenderMode);
            }
        }
        
        // 同步给所有下拉菜单
        if (apiAddressCombo) apiAddressCombo->setRenderMode(m_glassRenderMode);
        if (modelCombo) modelCombo->setRenderMode(m_glassRenderMode);
        if (glossaryCombo) glossaryCombo->setRenderMode(m_glassRenderMode);
        
        // 更新界面的提示文本 (同步鼠标悬浮的字典提示)
        updateUIText();

        // 如果悬浮窗开启也需要更新
        if (m_glossaryDrawer) {
            static_cast<GlossaryDrawer*>(m_glossaryDrawer.data())->setRenderMode(m_glassRenderMode);
        }
        
        // 强制重绘主窗口
        update(); });
}

// 🎨 全局色相偏移量改变
void ModernWindow::onHueChanged(int hueOffset)
{
    m_hueShift = hueOffset;
    updateComboEnv();
    updatePowerButtonState(m_isServerRunning);
    update();
    // 🎨 调色板实时刷新
    if (m_palettePopup) {
        m_palettePopup->update();
    }
}

// 🎨 更新调色板浓度 (Tint Intensity)
void ModernWindow::onTintIntensityChanged(int intensity)
{
    m_tintIntensity = intensity;
    updateComboEnv();
    updatePowerButtonState(m_isServerRunning);
    update();
    // 🎨 调色板实时刷新
    if (m_palettePopup) {
        m_palettePopup->update();
    }
}

// 🎨 调色板实时同步适配器（持有主窗口指针，动态读取参数）
class PaletteUpdateAdapter : public QObject {
public:
    PaletteUpdateAdapter(QWidget *popup, ModernWindow *mainWin, QObject *parent = nullptr)
        : QObject(parent), m_popup(popup), m_mainWin(mainWin)
    {
        if (mainWin) {
            // 连接信号，确保窗口销毁时清空指针
            connect(mainWin, &ModernWindow::destroyed, this, [this]() {
                m_mainWin = nullptr;
            });
        }
    }

    void startOpenAnimation()
    {
        if (!m_popup) {
            return;
        }

        m_isClosing = false;

        const QRect finalRect = m_popup->geometry();
        const QSize startSize(
            qMax(220, int(finalRect.width() * 0.92)),
            qMax(120, int(finalRect.height() * 0.90)));

        QRect startRect(QPoint(0, 0), startSize);
        startRect.moveCenter(finalRect.center() + QPoint(0, 6));

        m_popup->setWindowOpacity(0.0);
        m_popup->setGeometry(startRect);

        auto *group = new QParallelAnimationGroup(m_popup);

        auto *geometryAnim = new QPropertyAnimation(m_popup, "geometry", group);
        geometryAnim->setDuration(180);
        geometryAnim->setStartValue(startRect);
        geometryAnim->setEndValue(finalRect);
        geometryAnim->setEasingCurve(QEasingCurve::OutCubic);

        auto *opacityAnim = new QPropertyAnimation(m_popup, "windowOpacity", group);
        opacityAnim->setDuration(140);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutQuad);

        connect(group, &QParallelAnimationGroup::finished, m_popup, [this]() {
            if (m_popup) {
                m_popup->setWindowOpacity(1.0);
            }
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void startCloseAnimation()
    {
        if (!m_popup || m_isClosing) {
            return;
        }

        m_isClosing = true;

        const QRect startRect = m_popup->geometry();
        const QSize endSize(
            qMax(220, int(startRect.width() * 0.90)),
            qMax(120, int(startRect.height() * 0.88)));

        QRect endRect(QPoint(0, 0), endSize);
        endRect.moveCenter(startRect.center() + QPoint(0, 5));

        auto *group = new QParallelAnimationGroup(m_popup);

        auto *geometryAnim = new QPropertyAnimation(m_popup, "geometry", group);
        geometryAnim->setDuration(130);
        geometryAnim->setStartValue(startRect);
        geometryAnim->setEndValue(endRect);
        geometryAnim->setEasingCurve(QEasingCurve::InCubic);

        auto *opacityAnim = new QPropertyAnimation(m_popup, "windowOpacity", group);
        opacityAnim->setDuration(110);
        opacityAnim->setStartValue(1.0);
        opacityAnim->setEndValue(0.0);
        opacityAnim->setEasingCurve(QEasingCurve::InQuad);

        connect(group, &QParallelAnimationGroup::finished, m_popup, [this]() {
            if (m_popup) {
                m_popup->close();
            }
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    bool eventFilter(QObject *obj, QEvent *ev) override {
        if (ev->type() == QEvent::Paint && m_mainWin && m_popup) {
            QWidget *w = static_cast<QWidget*>(obj);
            QPainter p(w);
            p.setRenderHint(QPainter::Antialiasing);
            QRect r = w->rect();

            // 🎨 通过 getter 方法直接读取主窗口的最新参数
            bool isDark = m_mainWin->getIsDark();
            int alpha = m_mainWin->getAlpha();
            bool isRounded = m_mainWin->getIsRounded();
            int hueShift = m_mainWin->getHueShift();
            int tintIntensity = m_mainWin->getTintIntensity();
            GlassRenderMode mode = m_mainWin->getGlassRenderMode();
            
            int rds = isRounded ? 12 : 0;

            // 🌫️ 使用与主窗相同的渲染参数
            if (mode == GlassRenderMode::Frosted) {
                drawMenuGlassEffect(p, r, isDark, alpha, rds, hueShift, tintIntensity);
            } else {
                drawLegacyGlowEffect(p, r, isDark, alpha, rds, 1.0f);
            }

            // 💎 增强边框质感：使用彩虹渐变描边，实时响应 hueShift
            QLinearGradient borderGrad(r.topLeft(), r.bottomRight());
            if (isDark) {
                // 深色模式：使用 hueShift 调整色彩
                QColor c1 = shiftHue(QColor(255, 140, 0, 200), hueShift);     // 橙色主调
                QColor c2 = shiftHue(QColor(100, 200, 255, 180), hueShift);   // 青色辅调
                QColor c3 = shiftHue(QColor(255, 100, 200, 200), hueShift);   // 粉紫色
                borderGrad.setColorAt(0.0, c1);
                borderGrad.setColorAt(0.5, c2);
                borderGrad.setColorAt(1.0, c3);
            } else {
                // 亮色模式
                QColor c1 = shiftHue(QColor(147, 0, 211, 240), hueShift);     // 紫色主调
                QColor c2 = shiftHue(QColor(100, 150, 255, 200), hueShift);   // 蓝色辅调
                QColor c3 = shiftHue(QColor(200, 80, 180, 240), hueShift);    // 粉色
                borderGrad.setColorAt(0.0, c1);
                borderGrad.setColorAt(0.5, c2);
                borderGrad.setColorAt(1.0, c3);
            }
            p.setPen(QPen(borderGrad, 2.5));
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
            
            // 💡 额外：顶部光晕
            if (alpha > 100) {
                QLinearGradient topGlow(r.topLeft(), r.topLeft() + QPoint(0, 20));
                topGlow.setColorAt(0.0, QColor(255, 255, 255, 60));
                topGlow.setColorAt(1.0, QColor(255, 255, 255, 0));
                p.setBrush(topGlow);
                p.setPen(Qt::NoPen);
                p.drawPath(createTopGlowPath(r, rds));
            }
            
            return true;
        }
        if (ev->type() == QEvent::WindowDeactivate && m_popup) {
            startCloseAnimation();
            return true;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    // 创建顶部高光路径
    static QPainterPath createTopGlowPath(const QRect &rect, int radius) {
        QPainterPath path;
        int h = 20;
        path.moveTo(rect.left() + radius, rect.top());
        path.lineTo(rect.right() - radius, rect.top());
        path.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + radius);
        path.lineTo(rect.right(), rect.top() + h);
        path.lineTo(rect.left(), rect.top() + h);
        path.lineTo(rect.left(), rect.top() + radius);
        path.quadTo(rect.left(), rect.top(), rect.left() + radius, rect.top());
        path.closeSubpath();
        return path;
    }

    QPointer<QWidget> m_popup;
    ModernWindow *m_mainWin;
    bool m_isClosing = false;
};

// 🎨 展开全局调色盘悬浮窗（支持实时渲染）
void ModernWindow::toggleHuePalette()
{
    if (m_palettePopup) {
        if (m_paletteAdapter) {
            m_paletteAdapter->startCloseAnimation();
        } else {
            m_palettePopup->close();
        }
        return;
    }

    QWidget *popup = new QWidget(this, Qt::Tool | Qt::FramelessWindowHint);
    popup->setAttribute(Qt::WA_TranslucentBackground);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setFixedSize(284, 148); // 继续压缩像素尺寸

    QVBoxLayout *layout = new QVBoxLayout(popup);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(6);

    // 核心 UI 内容：标题与滑动条（支持中/英双语）
    QString titleText = (m_lang == 1) ? QStringLiteral("🌈 全局色调设定") : QStringLiteral("🌈 Global Tint Settings");
    QLabel *title = new QLabel(titleText);
    title->setStyleSheet(QString("font-size: 12px; font-weight: bold; color: %1;")
        .arg(m_isDark ? "#FFA500" : "#9400D3"));
    layout->addWidget(title, 0, Qt::AlignHCenter);

    // --- 色相选择滑块 ---
    QString hueLabel = (m_lang == 1) ? QStringLiteral("🌈 色相偏移") : QStringLiteral("🌈 Hue Shift");
    QLabel *lblHue = new QLabel(hueLabel);
    lblHue->setStyleSheet(QString("font-size: 11px; color: %1;").arg(m_isDark ? "#CCCCCC" : "#555555"));
    layout->addWidget(lblHue);

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 360);
    slider->setValue(m_hueShift);
    
    // 根据全局渲染形状调整滑块外观
    int grooveRadius = m_isRounded ? 5 : 0;
    int handleRadius = m_isRounded ? 6 : 0;
    int handleWidth  = m_isRounded ? 13 : 10;
    
    // 炫酷的彩虹渐变滑块背景
    QString sliderStyle = QString(R"(
        QSlider {
            min-height: 16px;
            padding: 0px;
        }
        QSlider::groove:horizontal {
            border: none;
            height: 8px;
            margin: 0px;
            border-radius: %1px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #ff0000, stop:0.16 #ffff00, stop:0.33 #00ff00,
                stop:0.5 #00ffff, stop:0.66 #0000ff, stop:0.83 #ff00ff, stop:1 #ff0000);
            border: 1px solid rgba(255,255,255,100);
        }
        QSlider::sub-page:horizontal,
        QSlider::add-page:horizontal {
            background: transparent;
            margin: 0px;
        }
        QSlider::handle:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #e0e0e0);
            border: 2px solid #333;
            width: %2px;
            margin: -4px -1px;
            border-radius: %3px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }
    )").arg(grooveRadius).arg(handleWidth).arg(handleRadius);
    slider->setStyleSheet(sliderStyle);
    
    connect(slider, &QSlider::valueChanged, this, &ModernWindow::onHueChanged);
    layout->addWidget(slider);

    // --- 流光浓度滑块 ---
    QString tintLabel = (m_lang == 1) ? QStringLiteral("✨ 流光浓度") : QStringLiteral("✨ Tint Intensity");
    QLabel *lblTint = new QLabel(tintLabel);
    lblTint->setStyleSheet(QString("font-size: 11px; color: %1;").arg(m_isDark ? "#CCCCCC" : "#555555"));
    layout->addWidget(lblTint);

    QSlider *tintSlider = new QSlider(Qt::Horizontal);
    tintSlider->setRange(0, 200);
    tintSlider->setValue(m_tintIntensity);

    QString tintStyle = QString(R"(
        QSlider {
            min-height: 16px;
            padding: 0px;
        }
        QSlider::groove:horizontal {
            border: none;
            height: 8px;
            margin: 0px;
            border-radius: %1px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(100, 100, 100, 180), stop:0.5 rgba(200, 100, 255, 220), stop:1 rgba(255, 100, 200, 200));
            border: 1px solid rgba(255,255,255,80);
        }
        QSlider::sub-page:horizontal,
        QSlider::add-page:horizontal {
            background: transparent;
            margin: 0px;
        }
        QSlider::handle:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #e0e0e0);
            border: 2px solid #333;
            width: %2px;
            margin: -4px -1px;
            border-radius: %3px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }
    )").arg(grooveRadius).arg(handleWidth).arg(handleRadius);
    tintSlider->setStyleSheet(tintStyle);

    connect(tintSlider, &QSlider::valueChanged, this, &ModernWindow::onTintIntensityChanged);
    layout->addWidget(tintSlider);

    // 滑块提示（可访问性/快速查看）
    slider->setToolTip((m_lang == 1) ? QStringLiteral("色相偏移") : QStringLiteral("Hue Shift"));
    tintSlider->setToolTip((m_lang == 1) ? QStringLiteral("流光浓度") : QStringLiteral("Tint Intensity"));

    m_palettePopup = popup;

    // 位置：在按钮下方无缝弹出
    if (btnPalette) {
        QPoint globalPos = btnPalette->mapToGlobal(QPoint(0, btnPalette->height()));
        popup->move(globalPos.x() - popup->width() / 2 + btnPalette->width() / 2, globalPos.y() + 8);
    } else {
        popup->move(this->mapToGlobal(QPoint(this->width() / 2 - popup->width() / 2, 100)));
    }

    // 💎 安装实时同步适配器
    m_paletteAdapter = new PaletteUpdateAdapter(popup, this, popup);
    popup->installEventFilter(m_paletteAdapter);

    connect(popup, &QObject::destroyed, this, [this]() {
        m_palettePopup = nullptr;
        m_paletteAdapter = nullptr;
    });
    
    popup->show();
    if (m_paletteAdapter) {
        m_paletteAdapter->startOpenAnimation();
    }
}

void ModernWindow::onBatchContextMenu(const QPoint &pos)
{
    QMenu menu(this);

    // 🌟 注入流光级动态透明样式
    menu.setAttribute(Qt::WA_TranslucentBackground);
    menu.setWindowFlags(menu.windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    int r = m_isRounded ? 6 : 0;
    QString menuStyle = m_isDark ? QString("QMenu { background-color: rgba(30, 30, 35, %1); color: #EAEAEA; border: 1px solid rgba(255,255,255,30); border-radius: %2px; padding: 4px; margin: 0px; }"
                                           "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                                           "QMenu::item:selected { background-color: rgba(255, 140, 0, 180); color: white; }"
                                           "QMenu::item:disabled { color: rgba(128,128,128,150); }")
                                       .arg(m_alpha)
                                       .arg(r)
                                 : QString("QMenu { background-color: rgba(245, 250, 255, %1); color: #222222; border: 1px solid rgba(0,0,0,30); border-radius: %2px; padding: 4px; margin: 0px; }"
                                           "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                                           "QMenu::item:selected { background-color: rgba(148, 0, 211, 180); color: white; }"
                                           "QMenu::item:disabled { color: rgba(128,128,128,180); }")
                                       .arg(m_alpha)
                                       .arg(r);
    menu.setStyleSheet(menuStyle);

    QString actionText = (m_lang == 1) ? "🔄 强制还原备份 (恢复初始 .xua_bak)"
                                       : "🔄 Hard Restore Backup (.xua_bak)";
    QAction *restoreAction = menu.addAction(actionText);

    QAction *selectedAction = menu.exec(chkBatch->mapToGlobal(pos));
    if (selectedAction == restoreAction)
    {
        QString path = glossaryCombo->currentText();
        int res = XuaConfigHijacker::hardRestoreFromBackup(path);

        if (res == 0)
        {
            LogManager::instance().addLog((m_lang == 1) ? "✅ [多行模式] 强制还原成功！已恢复初始备份。"
                                                        : "✅ [Batch Mode] Hard restore success! Back to original .xua_bak");
        }
        else if (res == 1)
        {
            LogManager::instance().addLog((m_lang == 1) ? "❌ 找不到 Config.ini 路径 (请检查术语表路径是否正确)"
                                                        : "❌ Config.ini path not found.");
        }
        else if (res == 2)
        {
            LogManager::instance().addLog((m_lang == 1) ? "⚠️ 未找到备份文件 (.xua_bak)，说明您的配置尚未被劫持修改过。"
                                                        : "⚠️ Backup file (.xua_bak) not found. Config was not hijacked.");
        }
        else
        {
            LogManager::instance().addLog((m_lang == 1) ? "❌ 文件还原失败，请检查文件是否被游戏占用。"
                                                        : "❌ File restore failed, check if file is locked.");
        }
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool ModernWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool ModernWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG")
    {
        MSG *msg = static_cast<MSG *>(message);

        // 1. 拦截客户区计算：抹杀丑陋的原生标题栏，将您的流光玻璃铺满整个窗口！
        if (msg->message == WM_NCCALCSIZE)
        {
            if (msg->wParam == TRUE)
            {
                *result = 0; // 核心魔法：返回 0 告诉 Win11 不要画边框
                return true;
            }
        }

        // 2. 接管命中测试：呼唤 Win11 系统的原生边界阻尼和分屏
        if (msg->message == WM_NCHITTEST)
        {
            // 1. 全屏或最大化状态拦截
            if (this->isMaximized() || this->isFullScreen())
            {
                *result = HTCLIENT;
                return true;
            }
            // ====================================================================
            // 🔥 CAN 的降维打击：彻底抛弃 Qt 坐标，使用 Windows 绝对物理坐标系
            // ====================================================================

            // 从 lParam 中极其精准地提取鼠标在屏幕上的【绝对物理坐标】
            int x = GET_X_LPARAM(msg->lParam);
            int y = GET_Y_LPARAM(msg->lParam);
            // 获取当前窗口在屏幕上的【绝对物理包围盒】
            HWND hwnd = (HWND)this->winId();
            RECT winRect;
            GetWindowRect(hwnd, &winRect);
            // 区域参数定义 (这些数值现在代表的是真实的屏幕物理像素，无比精准)
            const int RESIZE_BORDER = 5;       // 极薄的 5 像素拉伸热区，绝不吞噬按钮
            const int CAPTION_HEIGHT = 40;     // 顶部拖拽区高度
            const int BUTTON_AREA_WIDTH = 140; // 右上角按钮防务区宽度 (根据你的 UI 微调)
            // 精确的物理边界碰撞检测 (只比较绝对坐标，没有任何相对转换的误差！)
            bool isLeft = (x >= winRect.left) && (x < winRect.left + RESIZE_BORDER);
            bool isRight = (x < winRect.right) && (x >= winRect.right - RESIZE_BORDER);
            bool isTop = (y >= winRect.top) && (y < winRect.top + RESIZE_BORDER);
            bool isBottom = (y < winRect.bottom) && (y >= winRect.bottom - RESIZE_BORDER);
            // --- 优先级 1：四角拉伸 (精准打击) ---
            if (isLeft && isTop)
            {
                *result = HTTOPLEFT;
                return true;
            }
            if (isRight && isTop)
            {
                *result = HTTOPRIGHT;
                return true;
            }
            if (isLeft && isBottom)
            {
                *result = HTBOTTOMLEFT;
                return true;
            }
            if (isRight && isBottom)
            {
                *result = HTBOTTOMRIGHT;
                return true;
            }
            // --- 优先级 2：四边拉伸 ---
            if (isLeft)
            {
                *result = HTLEFT;
                return true;
            }
            if (isRight)
            {
                *result = HTRIGHT;
                return true;
            }
            if (isTop)
            {
                *result = HTTOP;
                return true;
            }
            if (isBottom)
            {
                *result = HTBOTTOM;
                return true;
            }
            // --- 优先级 3：顶部标题栏拖拽区，以及神圣不可侵犯的右上角按钮区 ---
            if (y >= winRect.top && y < winRect.top + CAPTION_HEIGHT)
            {
                // 如果鼠标 X 坐标进入了右上角的按钮保护领地，强行归还给 Qt 处理！
                if (x >= winRect.right - BUTTON_AREA_WIDTH)
                {
                    *result = HTCLIENT;
                    return true;
                }

                // 否则，安全的顶部区域就是纯粹的拖拽区
                *result = HTCAPTION;
                return true;
            }
            // --- 优先级 4：其他所有区域全部交还客户区 ---
            *result = HTCLIENT;
            return true;
        }
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}