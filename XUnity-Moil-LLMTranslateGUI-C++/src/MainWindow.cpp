/**
 * MainWindow.cpp - Moil's XUnity LLM Translator GUI Implementation
 * MainWindow.cpp - Moil的XUnity大模型翻译器GUI实现
 *
 * Final Perfect Version by CAN.
 * Base: MainWindow.txt (Preferred UI/UX)
 * Core Logic: 1.txt (Robust Hot Reload & Layout Fixes)
 * Updated: Logic Fixed - User Lock Override (Session Persistence)
 * UI Tweaked: Glossary Path Stretch, Evolve & Lock Migrated (By CAN)
 */

#include "MainWindow.h"
#include "json.hpp"
#include "LogManager.h"
#include "XuaConfigHijacker.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QCloseEvent>
#include <QStyleFactory>
#include <QPixmap>
#include <QMenu>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QListView>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QMouseEvent>
#include <QTimer>

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QSignalBlocker>

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

// ==========================================
// 🎨 术语表语法高亮器 (实时渲染 原文=译文)
// ==========================================
class GlossaryHighlighter : public QSyntaxHighlighter
{
public:
    explicit GlossaryHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {}

    void setTheme(bool isDark)
    {
        if (isDark)
        {
            // 🌙 暗色主题
            fmtOriginal.setForeground(QColor("#CD7F32"));    // 🌟 经典古铜色 (Bronze)
            fmtOriginal.setFontWeight(QFont::Bold);          // 原文加粗，增加辨识度
            fmtEqual.setForeground(QColor("#3bfd00"));       // 暗灰色 (弱化等号的视觉占比)
            fmtTranslation.setForeground(QColor("#fd0000")); // 黄金色 (译文，完美搭配你的悬浮金)
            fmtComment.setForeground(QColor("#6A9955"));     // 护眼绿 (应对用户写 // 注释的情况)
        }
        else
        {
            // ☀️ 亮色主题
            fmtOriginal.setForeground(QColor("#fc0000")); // 深马鞍棕 (亮色下的古铜平替)
            fmtOriginal.setFontWeight(QFont::Bold);
            fmtEqual.setForeground(QColor("#ff0000fd"));
            fmtTranslation.setForeground(QColor("#ff00d4")); // 微软科技蓝
            fmtComment.setForeground(QColor("#008000"));
        }
        rehighlight(); // 立即刷新整个文档的高亮
    }

protected:
    void highlightBlock(const QString &text) override
    {
        // 1. 如果是注释 (以 // 开头)，整行变绿
        if (text.trimmed().startsWith("//"))
        {
            setFormat(0, text.length(), fmtComment);
            return;
        }

        // 2. 查找第一个等号的位置 (XUnity的规则是首个等号分割)
        int equalPos = text.indexOf('=');
        if (equalPos != -1)
        {
            // 🌟 原文 (从位置 0 开始，长度为 equalPos)
            setFormat(0, equalPos, fmtOriginal);

            // 🌟 等号本身 (位置 equalPos，长度为 1)
            setFormat(equalPos, 1, fmtEqual);

            // 🌟 译文 (从等号后一位开始，直到行尾)
            setFormat(equalPos + 1, text.length() - equalPos - 1, fmtTranslation);
        }
        // 如果没有等号，则不应用任何格式，保持原有的纯净文字颜色
    }

private:
    QTextCharFormat fmtOriginal;
    QTextCharFormat fmtEqual;
    QTextCharFormat fmtTranslation;
    QTextCharFormat fmtComment;
};

// ==========================================
// 🌍 多语言字典定义 (UI Text)
// ==========================================
const char *STR_BATCH_MODE[] = {"Batch", "多行模式"};
const char *TIP_BATCH_MODE[] = {
    "[Advanced] If checked, Config.ini will be hijacked to support batch translation (auto-backup).\n[Rec] OFF for Visual Novels, ON for RPG/UI-heavy games.\n[Disclaimer] Enabling may increase translation latency. \n[Tip]Startup flow: Start service first, then launch game (ensure ini is modified).\n Right-click to restore original ini.",
    "【慎用】开启后，启动服务时将自动修改游戏 Config.ini 以支持多行并发（会自动备份）。\n【建议】纯视觉小说游戏请关闭此项；UI 密集型游戏请开启。\n【声明】开启多行翻译后翻译延时可能会增加。\n【提示】开启后，启动流程为：先启动服务，再运行游戏（确保 ini 已被修改）。\n 右键可恢复ini原内容。"};

const char *STR_HANDLE_RICH_TEXT[] = {"HRText", "文本处理"};
const char *TIP_HANDLE_RICH_TEXT[] = {
    "[Batch Sub-feature] Preserves rich text code in game strings.\nRequires Batch Mode to be enabled first.\nDefault: OFF",
    "【打包模式子功能】保留游戏原文中的富文本。\n只有先开启了打包模式才可生效。\n默认：关闭"};

const char *STR_EXTRACT_NEWLINE[] = {"PLB", "保留换行"};
const char *TIP_EXTRACT_NEWLINE[] = {
    "[Batch Sub-feature] Preserves newlines and sends to LLM.\nRequires Batch Mode to be enabled first.\nDefault: ON",
    "【打包模式子功能】保留游戏原文中的换行符发送给LLM。\n只有先开启了打包模式才可生效。\n默认：开启"};

const char *STR_DEBUG_MODE[] = {"STM", "测速模式"};
const char *TIP_DEBUG_MODE[] = {"Enable to show endpoint types and ms latency in logs", "开启后，日志将显示底层端口类型和毫秒级耗时"};

// Window titles / 窗口标题
const char *STR_TITLE[] = {"Moil's XUnity LLM Translator", "Moil的XUnity大模型翻译GUI"};

// Configuration section titles / 配置部分标题
const char *STR_API_CFG[] = {"API Configuration", "API 配置"};
const char *STR_LOG_AREA[] = {"Runtime Logs", "运行日志"};

// API configuration related text / API配置相关文本
const char *STR_API_ADDR[] = {"Base_URL:", "API 地址:"};
const char *STR_API_KEY[] = {"API Key:", "API 密钥:"};
const char *STR_MODEL[] = {"Model:", "模型名称:"};
const char *STR_FETCH[] = {"Fetch", "获取"};

// Server parameter text / 服务器参数文本
const char *STR_PORT[] = {"Port:", "端口:"};
const char *STR_THREAD[] = {"Thr:", "线程:"};
const char *STR_TEMP[] = {"Tmp:", "温度:"};
const char *STR_CTX[] = {"CX:", "上下文:"};

// Prompt related text / 提示词相关文本
const char *STR_SYS_PROMPT[] = {"Sys-Prompt:", "系统提示:"};
const char *STR_PRE_PROMPT[] = {"Pre-Prompt:", "前置文本:"};

// 🌟 术语表编辑器与玻璃模式相关字典
const char *STR_BTN_MODERN[] = {"✨ Modern", "✨ 玻璃模式"};
const char *TIP_BTN_MODERN[] = {"Switch to Glass/Fluid UI", "切换至极简透明玻璃界面"};

const char *STR_BTN_EDIT_GLOS[] = {"📝 Edit Terms", "📝 编辑术语"};
const char *TIP_BTN_EDIT_GLOS[] = {"Open editor to modify glossary", "打开悬浮窗实时编辑当前术语表"};

const char *STR_GLOS_TITLE[] = {"📝 Glossary Editor", "📝 术语表编辑"};
const char *STR_GLOS_SAVE[] = {"💾 Save & Apply", "💾 保存并应用"};
const char *STR_GLOS_CLOSE[] = {"Close", "关闭"};
const char *STR_GLOS_WARN_TITLE[] = {"Warning", "警告"};
const char *STR_GLOS_WARN_MSG[] = {"Please select a glossary first!", "请先选择一个术语表！"};
const char *LOG_GLOS_SAVED[] = {"✅ Glossary updated and applied!", "✅ 术语表内容已更新并生效！"};

// Control button text / 控制按钮文本
const char *STR_START[] = {"Start Service", "启动服务"};
const char *STR_RELOAD[] = {"Hot Reload", "热重载配置"};
const char *STR_STOP[] = {"Stop Service", "停止服务"};
const char *STR_HUD[] = {"HUD Mode", "HUD 模式"};
const char *STR_TEST[] = {"Test Config", "测试配置"};
const char *STR_LOAD[] = {"Load Config", "读取配置"};
const char *STR_SAVE[] = {"Save Config", "保存配置"};
const char *STR_EXPORT[] = {"Export Log", "导出日志"};

// Theme and language text / 主题和语言文本
const char *STR_THEME_LIGHT[] = {"Light Mode", "切换亮色"};
const char *STR_THEME_DARK[] = {"Dark Mode", "切换暗色"};
const char *STR_LANG_BTN[] = {"中文", "English"};

// Glossary related text / 术语表相关文本
const char *STR_GLOSSARY[] = {"Glossary", "术语表"};
const char *STR_CHK_GLOSSARY[] = {"SE", "自进化"};
const char *STR_CLEAR_LOG[] = {"Clear Log", "清空日志"};
const char *STR_REMOVE_PATH[] = {"Remove Current Path", "移除当前路径"};
const char *STR_CLEAR_HISTORY[] = {"Clear All History", "清空历史记录"};

// Token statistics text / Token统计文本
const char *STR_TOKENS[] = {"Tokens:", "消耗:"};
const char *TIP_TOKENS[] = {"Total Usage (Prompt + Completion)", "本次运行总消耗 (输入+输出)"};

// Context clearing related text / 上下文清除相关文本
const char *STR_CLEAR_CTX[] = {"Clr", "清空"};
const char *TIP_CLEAR_CTX[] = {"Clear Context", "清除历史对话记忆"};

// New button text / 新按钮文本
const char *STR_BTN_AUTO[] = {"Edit", "编辑"};
const char *TIP_BTN_AUTO[] = {
    "Open _AutoGeneratedTranslations.txt in same folder",
    "打开同目录下的 _AutoGeneratedTranslations.txt (自动翻译结果)"};

//
const char *STR_LOCK_SYS[] = {"Lock", "锁定"};
const char *TIP_LOCK_SYS[] = {
    "Lock System Prompt to prevent overwriting when loading config",
    "锁定系统提示词，防止在读取配置时被覆盖"};

// ==========================================
// 📝 日志消息定义 (Logs - Merged for Completeness)
// ==========================================

const char *LOG_TEST_START[] = {"📡 === Testing API Keys ===", "📡 === 开始测试所有 API Key ==="};
const char *LOG_NO_KEY[] = {"❌ No API Key", "❌ 未找到 API Key"};
const char *LOG_PASS[] = {"Pass", "测试通过"};
const char *LOG_FAIL[] = {"Fail", "失败"};
const char *LOG_FETCH_SUCCESS[] = {"Fetch Models Success", "模型列表获取成功"};
const char *LOG_FETCH_FAIL[] = {"Fetch Failed: ", "获取失败: "};
const char *LOG_PARSE_ERR[] = {"Parse Error", "解析错误"};
const char *LOG_CFG_SAVED[] = {"⚙️ Config Saved: ", "⚙️ 配置已保存: "};
const char *LOG_CFG_LOADED[] = {"📙 Config Loaded: ", "📙 配置已加载: "};
const char *LOG_EXPORTED[] = {"✒️ Log Exported to run_log.txt", "✒️ 日志已导出到 run_log.txt"};

// 🔥 Enhanced Hot Reload Logs (From 1.txt) / 增强的热重载日志（来自1.txt）
const char *LOG_RELOADED[] = {"⚡ Config Hot Reloaded!", "⚡ 配置已热重载生效！"};
const char *LOG_MODEL_SWITCH[] = {"🔄 Model Switch: [%1] -> [%2]", "🔄 模型切换: [%1] -> [%2]"};
const char *LOG_MODEL_SAME[] = {"⚡ Reloaded (Model: %1)", "⚡ 热重载成功 (模型: %1)"};
const char *LOG_AUTO_TESTING[] = {"🛠️ Auto-testing new config...", "🛠️ 正在自动测试新配置..."};

// ==========================================
// 💡 详细工具提示 (Detailed Tooltips)
// ==========================================

const char *TIP_PORT[] = {
    "Local Listening Port (Restart required to change)\nEnsure XUnity Endpoint is set to http://localhost:port",
    "本地监听端口 (修改需重启服务)\n请确保 XUnity 配置文件 Endpoint 设置为 http://localhost:端口号"};

const char *TIP_THREAD[] = {
    "Concurrent Threads\nThe effective range is 64 to 256\nThe default value is 100.",
    "并发线程数\n有效值：64 ~ 256\n默认为100"};

const char *TIP_TEMP[] = {
    "Sampling Temperature\n0.0-0.3: Strict\n0.7-1.0: Standard\n>1.0: Creative/Random",
    "采样温度 (Temperature)\n0.0-0.3: 严谨\n0.7-1.0: 标准\n>1.0: 随机/创造性"};

const char *TIP_CTX[] = {
    "Context Memory\nNumber of history turns to carry.\nNote: More context consumes more tokens.",
    "上下文记忆 (Context)\n携带的历史对话轮数。\n注意：上下文越多，消耗 Token 越多。"};

const char *TIP_GLOSSARY[] = {
    "Requires [SE] to be enabled first.\nSelect XUnity's _Substitutions.txt.\nLLM will reference and append to it.",
    "本功能需要先行勾选[自进化]才可正常工作\n选择 XUnity 的 _Substitutions.txt 文件。\nLLM 将自动参考并补充该文件。"};

const char *TIP_COMBO_MAIN[] = {
    "Enter API Address or select from list.\nMust support /v1/chat/completions format.",
    "在此输入 API 地址，或从下拉列表中选择主流服务商。\n所有地址必须兼容 OpenAI 接口格式 (/v1/chat/completions)。"};

// ==========================================
// ⚙️ API Presets
// ==========================================

struct ApiPresetDef
{
    const char *url;     // API URL / API地址
    const char *tips[2]; // Tooltip text in both languages / 双语工具提示文本
};

const ApiPresetDef PRESETS_DATA[] = {
    {"https://api.openai.com/v1", {"OpenAI Official API", "OpenAI 官方接口"}},
    {"https://api.deepseek.com", {"DeepSeek Official API", "DeepSeek 官方接口"}},
    {"https://api.x.ai/v1", {"Grok (xAI) Official API", "Grok (xAI) 官方接口"}},
    {"https://api.siliconflow.cn/v1", {"SiliconFlow", "硅基流动 (SiliconFlow)"}},
    {"https://openrouter.ai/api/v1", {"OpenRouter Aggregator", "OpenRouter 聚合平台"}},
    {"https://generativelanguage.googleapis.com/v1beta/openai", {"Google Gemini", "Google Gemini (OpenAI 兼容端点)"}},
    {"http://localhost:11434/v1", {"Ollama Local Service", "Ollama 本地服务"}},
    {"http://localhost:1234/v1", {"LM Studio Local Service", "LM Studio 本地服务"}}};

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

// ==========================================
// ✨ 经典模式流光预设窗口 (Classic Preset Dialog with Modern Styling)
// ==========================================
class ClassicPresetDialog : public QDialog
{
public:
    ClassicPresetDialog(QWidget *parent, int lang, bool isDark)
        : QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint), m_lang(lang), m_isDark(isDark)
    {
        setFixedSize(480, 440);
        setWindowTitle(m_lang == 1 ? "添加自定义预设" : "Add Custom Preset");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(8);

        // 统一的主体色
        QString accentHex = isDark ? "#FF5E00" : "#9400D3"; // 默认色彩 (Orange/Purple)
        QString bgAccentHex = isDark ? "rgba(255, 94, 0, 150)" : "rgba(148, 0, 211, 150)";

        // Title
        QLabel *lblTitle = new QLabel(m_lang == 1 ? "✨ 添加自定义预设" : "✨ Add Custom Preset", this);
        lblTitle->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1; margin-bottom: 6px;").arg(m_isDark ? "white" : "black"));
        mainLayout->addWidget(lblTitle);

        // Styling helpers
        QString labelStyle = QString("color: %1; font-weight: bold; font-size: 12px; margin-top: 2px; margin-bottom: 2px;").arg(m_isDark ? "#E0E0E0" : "#333333");
        QString inputStyle = m_isDark ? 
            "QLineEdit, QComboBox { background: rgba(0,0,0,80); border: 1px solid rgba(255,255,255,20); color: white; padding: 6px 10px; border-radius: 6px; font-size: 13px; }"
            "QLineEdit:focus, QComboBox:focus { border: 1px solid " + accentHex + "; background: rgba(0,0,0,100); }" : 
            "QLineEdit, QComboBox { background: rgba(255,255,255,180); border: 1px solid rgba(0,0,0,20); color: black; padding: 6px 10px; border-radius: 6px; font-size: 13px; }"
            "QLineEdit:focus, QComboBox:focus { border: 1px solid " + accentHex + "; background: rgba(255,255,255,220); }";

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
        m_modelCombo = new QComboBox(this);
        m_modelCombo->setEditable(true);
        m_modelCombo->setMinimumHeight(32);
        m_modelCombo->setStyleSheet(inputStyle);

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
        connect(btnOk, &QPushButton::clicked, this, [this]() {
            if (m_urlEdit->text().trimmed().isEmpty() || m_keyEdit->text().trimmed().isEmpty() || m_modelCombo->currentText().trimmed().isEmpty()) {
                QMessageBox::warning(this, m_lang == 1 ? "信息不完整" : "Incomplete Info",
                                 m_lang == 1 ? "API 地址、密钥和模型名称为必填项。" : "URL, Key and Model are required.");
                return;
            }
            accept();
        });

        connect(m_btnFetch, &QPushButton::clicked, this, &ClassicPresetDialog::onFetchModels);
    }

    QString getUrl() const { return m_urlEdit->text().trimmed(); }
    QString getKey() const { return m_keyEdit->text().trimmed(); }
    QString getModel() const { return m_modelCombo->currentText().trimmed(); }
    QString getName() const { return m_nameEdit->text().trimmed(); }

private:
    void onFetchModels()
    {
        QString url = m_urlEdit->text().trimmed();
        QString key = m_keyEdit->text().trimmed();

        if (url.isEmpty()) {
            QMessageBox::warning(this, m_lang == 1 ? "错误" : "Error", 
                               m_lang == 1 ? "请先填写 API 地址。" : "Please fill URL first.");
            return;
        }

        if (url.endsWith("/")) url.chop(1);

        // 使用 LoadingOverlay 动画覆盖整个获取按钮
        LoadingOverlay *fetchLoadingOverlay = new LoadingOverlay(m_btnFetch);
        fetchLoadingOverlay->setGeometry(m_btnFetch->rect());
        fetchLoadingOverlay->raise();
        fetchLoadingOverlay->start();
        
        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        QNetworkRequest req(QUrl(url + "/models"));
        req.setTransferTimeout(8000);
        if (!key.isEmpty()) {
            req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
        }

        QNetworkReply *reply = mgr->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, mgr, fetchLoadingOverlay]() {
            if (fetchLoadingOverlay) {
                fetchLoadingOverlay->stop();
                fetchLoadingOverlay->deleteLater();
            }

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
                            
                            // 更新状态文本统一格式
                            QString accentHex = m_isDark ? "#FF5E00" : "#9400D3";
                            m_lblStatusLeft->setText(m_lang == 1 ? "● 上次获取: <span style='color:"+accentHex+";'>刚刚</span>" : "● Last fetch: <span style='color:"+accentHex+";'>Just now</span>");
                            m_lblStatusLeft->setTextFormat(Qt::RichText);
                            m_lblStatusRight->setText(QString(m_lang == 1 ? "共 %1 个模型" : "Total %1 models").arg(m_modelCombo->count()));
                            m_modelCombo->setFocus();
                        }
                    } else {
                        m_lblStatusLeft->setText(m_lang == 1 ? "<span style='color:#FF5555;'>解析失败：数据格式不符合规范</span>" : "<span style='color:#FF5555;'>Parse Error: Invalid format</span>");
                        m_lblStatusRight->setText(m_lang == 1 ? "无模型" : "No models");
                    }
                } catch(...) {
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

    int m_lang;
    bool m_isDark;

    QLineEdit *m_urlEdit;
    QLineEdit *m_keyEdit;
    QComboBox *m_modelCombo;
    QLineEdit *m_nameEdit;
    QPushButton *m_btnFetch;
    
    QLabel *m_lblStatusLeft;
    QLabel *m_lblStatusRight;
};

// ==========================================
// 🚀 Implementation
// ==========================================

// --- MainWindow.cpp ---

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // 1. 初始化成员变量
    m_isClosing = false;
    m_isDarkTheme = true; // 默认暂设为 true
    m_currentLang = 1;
    m_isServerRunning = false;

    setMinimumSize(500, 600);
    resize(500, 800);

    // 2. 创建核心组件
    m_tokenManager = new TokenManager(this);
    server = new TranslationServer(this);
    m_hudWindow = new HudWindow(nullptr);

    // 3. 设置 UI
    setupUi();
    setupApiKeyMemory();

    // 4. 连接信号槽 (保持不变...)
    connect(&LogManager::instance(), &LogManager::newLogAvailable, this, &MainWindow::onLogMessage);
    connect(&LogManager::instance(), &LogManager::logsCleared, logArea, &QTextEdit::clear);
    connect(server, &TranslationServer::tokenUsageReceived, m_tokenManager, &TokenManager::addUsage);
    connect(m_tokenManager, &TokenManager::tokensUpdated, this, &MainWindow::updateTokenDisplay);
    connect(m_hudWindow, &HudWindow::requestRestore, this, &MainWindow::restoreFromHud);
    connect(m_tokenManager, &TokenManager::tokensUpdated, [this](long long t, long long, long long)
            {
        if(m_hudWindow) m_hudWindow->updateTokens(t); });
    connect(server, &TranslationServer::serverStarted, this, [this]()
            { toggleControls(true); });
    connect(server, &TranslationServer::serverStopped, this, [this]()
            { toggleControls(false); });

    connect(server, &TranslationServer::workStarted, this, &MainWindow::onServerWorkStarted);
    connect(server, &TranslationServer::workFinished, this, &MainWindow::onServerWorkFinished);

    // 5. 加载配置
    // loadConfigToUi 会读取配置文件，并将正确的颜色设置赋值给 m_isDarkTheme
    loadConfigToUi();

    m_lastApiBaseUrl = normalizeApiBaseUrl(apiAddressCombo->currentText());
    m_apiKeyMemoryEnabled = true;

    updateUIText();

    // --- 🔥 核心修复：不要强制 true，而是使用读取到的 m_isDarkTheme ---
    // 之前是：applyTheme(true);  <-- 这就是罪魁祸首
    applyTheme(m_isDarkTheme);

    // 6. 淡入动画 (保持不变)
    setWindowOpacity(0.0);
    fadeAnim = new QPropertyAnimation(this, "windowOpacity");
    fadeAnim->setDuration(500);
    fadeAnim->setStartValue(0.0);
    fadeAnim->setEndValue(1.0);
    fadeAnim->start();
}

void MainWindow::setupApiKeyMemory()
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

void MainWindow::persistCurrentApiKeyMemory()
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

void MainWindow::handleApiBaseUrlChanged()
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

MainWindow::~MainWindow()
{
    // Clean up resources / 清理资源
    if (m_hudWindow)
    {
        m_hudWindow->close();
        delete m_hudWindow;
    }
    server->stopServer();
}

void MainWindow::fadeOutAndClose()
{
    // Reverse the fade animation for smooth exit / 反向淡出动画实现平滑退出
    fadeAnim->setDirection(QAbstractAnimation::Backward);
    connect(fadeAnim, &QPropertyAnimation::finished, this, &QMainWindow::close);
    connect(fadeAnim, &QPropertyAnimation::finished, qApp, &QApplication::quit);
    fadeAnim->start();
}

void MainWindow::smoothSwitch(std::function<void()> changeLogic)
{
    /**
     * Perform a smooth visual transition when changing UI state / 在更改UI状态时执行平滑视觉过渡
     * @param changeLogic: The actual UI change to perform / 要执行的实际UI更改逻辑
     */

    // Capture current screen / 捕获当前屏幕
    QPixmap pixmap = this->grab();
    QLabel *overlay = new QLabel(this);
    overlay->setPixmap(pixmap);
    overlay->setGeometry(0, 0, this->width(), this->height());
    overlay->show();

    // Execute the change logic / 执行更改逻辑
    changeLogic();

    // Apply fade-out animation to overlay / 对覆盖层应用淡出动画
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(overlay);
    overlay->setGraphicsEffect(effect);

    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(300);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);

    connect(anim, &QPropertyAnimation::finished, overlay, &QLabel::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::toggleLanguage()
{
    // 💡 1. First save the current complete configuration copy / 先保存当前完整配置副本
    AppConfig currentCfg = getUiConfig();

    smoothSwitch([this, currentCfg]()
                 {
                     // 💡 2. Switch language flag / 切换语言标志位
                     m_currentLang = (m_currentLang == 0) ? 1 : 0;

                     // 💡 3. Update UI text (blocker already added internally) / 更新 UI 文字（内部已加 blocker）
                     updateUIText();

                     // 💡 4. The processing here is critical: / 这里的处理很关键：
                     // After updateUIText finishes, we need to ensure apiAddressCombo restores correctly
                     // Since we already have currentCfg, directly use its value to restore
                     // updateUIText 结束后，我们需要确保 apiAddressCombo 恢复正确
                     // 既然我们已经有了 currentCfg，直接用它的值恢复
                     apiAddressCombo->setCurrentText(currentCfg.api_address);

                     if (themeBtn)
                         themeBtn->setText(m_isDarkTheme ? STR_THEME_LIGHT[m_currentLang] : STR_THEME_DARK[m_currentLang]);
                     toggleControls(m_isServerRunning);

                     // 💡 5. Update server configuration / 更新服务器配置
                     // At this point, currentCfg's language is still old, we need to update it before giving it to the server
                     // 此时 currentCfg 的 language 还是旧的，我们需要更新它再交给服务器
                     AppConfig finalCfg = currentCfg;
                     finalCfg.language = m_currentLang;
                     server->updateConfig(finalCfg);

                     // qApp->processEvents();
                     // adjustSize();
                     // resize(400, 800);
                 });
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // 🔥 核心修复：实时同步 Server 状态到 UI
    if (server)
    {
        AppConfig liveCfg = server->getConfig(); // 获取服务器正在运行的真实配置

        // 1. 强制同步语言 (解决您之前提到的语言状态同步)
        if (m_currentLang != liveCfg.language)
        {
            m_currentLang = liveCfg.language;
            updateUIText();
        }

        // 2. 强制同步术语表路径 (无视锁定，因为这是“同一程序”的内部同步)
        if (!liveCfg.glossary_path.isEmpty())
        {
            // 如果下拉框里没有这个路径，加进去
            if (glossaryCombo->findText(liveCfg.glossary_path) == -1)
            {
                glossaryCombo->insertItem(0, liveCfg.glossary_path);
            }
            // 强制选中服务器正在用的路径
            glossaryCombo->setCurrentText(liveCfg.glossary_path);
        }

        // 3. 强制同步自进化勾选框
        chkGlossary->setChecked(liveCfg.enable_glossary);

        // 4. 同步锁定状态 (确保勾选框本身也是同步的)
        chkLockGlossary->setChecked(liveCfg.lock_glossary);
        chkLockSysPrompt->setChecked(liveCfg.lock_system_prompt);

        // 5. 修正 API 地址显示
        apiAddressCombo->setCurrentText(liveCfg.api_address);

        // 美化：光标移到路径开头
        if (glossaryCombo->lineEdit())
        {
            glossaryCombo->lineEdit()->setCursorPosition(0);
        }
    }

    if (logGroup && lblTokens)
    {
        if (logGroup && lblTokens)
        {
            lblTokens->adjustSize();
            if (logGroup->width() > 0)
            {
                lblTokens->move(logGroup->width() - lblTokens->width() - 10, 0);
            }
        }
    }
}

void MainWindow::toggleTheme()
{
    smoothSwitch([this]()
                 {
        applyTheme(!m_isDarkTheme);
        toggleControls(m_isServerRunning); });
}

void MainWindow::addToGlossaryHistory(const QString &path)
{
    if (path.isEmpty())
        return;

    // Extract current history items
    QStringList items;
    for (int i = 0; i < glossaryCombo->count(); ++i)
    {
        items << glossaryCombo->itemText(i);
    }

    // Remove duplicates and add new path to top
    items.removeAll(path);
    items.insert(0, path);

    // Limit history size to 10 items
    while (items.size() > 10)
    {
        items.removeLast();
    }

    // Update combo box
    glossaryCombo->clear();
    glossaryCombo->addItems(items);
    glossaryCombo->setCurrentIndex(0);

    // 🔥 CAN: 强制将光标移到最左侧 (Focus on Start)
    if (glossaryCombo->lineEdit())
    {
        glossaryCombo->lineEdit()->setCursorPosition(0);
    }
}

void MainWindow::onSelectGlossary()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Glossary"), "", "_Substitutions (*.txt);;All Files (*)");
    if (!fileName.isEmpty())
    {
        addToGlossaryHistory(fileName);
        if (m_isServerRunning && server)
        {
            AppConfig cfg = getUiConfig();
            server->updateConfig(cfg);
            // 🔥 错误：onLogMessage(...)
            // 🔥 修正：使用 LogManager 确保全局同步
            LogManager::instance().addLog(m_currentLang == 0 ? "✅ New glossary applied." : "✅ 新术语表已应用。");
        }
    }
}

void MainWindow::onOpenAutoTranslations()
{
    // 获取当前术语表文件路径
    QString currentPath = glossaryCombo->currentText();

    // 🌟 修复：拦截空路径或无效占位符，给出明确提示，消除“沉默的失败”
    if (currentPath.isEmpty() || currentPath == "未选择" || currentPath == "None")
    {
        QMessageBox::warning(this,
                             (m_currentLang == 1 ? "⚠️ 提示" : "⚠️ Notice"),
                             (m_currentLang == 1 ? "请先选择一个有效的术语表路径！\n程序需要基于该路径来定位自动翻译记录文件。"
                                                 : "Please select a valid glossary path first!\nThe program needs it to locate the auto-translation file."));
        return;
    }

    // 构造自动生成的翻译文件路径
    QFileInfo fi(currentPath);
    QString dir = fi.absolutePath();
    QString targetFile = dir + "/_AutoGeneratedTranslations.txt";

    QFileInfo targetFi(targetFile);
    if (!targetFi.exists())
    {
        // 文件不存在，显示警告
        QMessageBox::warning(this,
                             (m_currentLang == 1 ? "文件未找到" : "File Not Found"),
                             (m_currentLang == 1 ? "未找到 _AutoGeneratedTranslations.txt。\n请确认游戏是否已经运行并生成了翻译。"
                                                 : "Could not find _AutoGeneratedTranslations.txt.\nPlease ensure the game has run and generated translations."));
        return;
    }

    // 使用系统默认应用程序打开文件
    QDesktopServices::openUrl(QUrl::fromLocalFile(targetFile));
}

void MainWindow::updateUIText()
{
    int i = m_currentLang;

    // 💡 Lock current API address and combo box state to prevent jumping during update
    // 锁定当前 API 地址和下拉框状态，防止更新时跳变
    QString currentApi = apiAddressCombo->currentText();
    QSignalBlocker blocker(apiAddressCombo); // Block all signals triggered by modifications / 阻止所有因修改而触发的信号

    // Update window title / 更新窗口标题
    setWindowTitle(STR_TITLE[i]);

    // Update group box titles / 更新分组框标题
    cfgGroup->setTitle(STR_API_CFG[i]);
    logGroup->setTitle(STR_LOG_AREA[i]);

    // Update labels / 更新标签
    lblApiAddr->setText(STR_API_ADDR[i]);
    lblApiKey->setText(STR_API_KEY[i]);
    lblModel->setText(STR_MODEL[i]);
    fetchModelBtn->setText(STR_FETCH[i]);

    lblPort->setText(STR_PORT[i]);
    lblThread->setText(STR_THREAD[i]);
    lblTemp->setText(STR_TEMP[i]);
    lblCtx->setText(STR_CTX[i]);

    lblSysPrompt->setText(STR_SYS_PROMPT[i]);

    // ✅ Update new lock system prompt checkbox text / 更新新增锁定系统提示词复选框文本
    chkLockSysPrompt->setText(STR_LOCK_SYS[i]);
    chkLockSysPrompt->setToolTip(TIP_LOCK_SYS[i]);

    lblPrePrompt->setText(STR_PRE_PROMPT[i]);

    clearCtxBtn->setText(STR_CLEAR_CTX[i]);
    clearCtxBtn->setToolTip(TIP_CLEAR_CTX[i]);

    lblGlossary->setText(STR_GLOSSARY[i]);
    chkGlossary->setText(STR_CHK_GLOSSARY[i]);

    chkDebug->setText(STR_DEBUG_MODE[i]);
    chkDebug->setToolTip(TIP_DEBUG_MODE[i]);

    chkHandleRichText->setText(STR_HANDLE_RICH_TEXT[i]);
    chkHandleRichText->setToolTip(TIP_HANDLE_RICH_TEXT[i]);

    chkExtractNewline->setText(STR_EXTRACT_NEWLINE[i]);
    chkExtractNewline->setToolTip(TIP_EXTRACT_NEWLINE[i]);

    chkBatch->setText(STR_BATCH_MODE[i]);
    chkBatch->setToolTip(TIP_BATCH_MODE[i]);

    // Update button text based on server state / 根据服务器状态更新按钮文本
    if (m_isServerRunning)
    {
        startBtn->setText(STR_RELOAD[i]);
    }
    else
    {
        startBtn->setText(STR_START[i]);
    }

    stopBtn->setText(STR_STOP[i]);
    hudBtn->setText(STR_HUD[i]);
    testBtn->setText(STR_TEST[i]);
    loadBtn->setText(STR_LOAD[i]);
    saveBtn->setText(STR_SAVE[i]);
    exportBtn->setText(STR_EXPORT[i]);
    langBtn->setText(STR_LANG_BTN[i]);

    if (themeBtn)
    {
        // 如果当前是暗色，按钮功能是"切到亮色"；反之亦然
        if (m_isDarkTheme)
        {
            themeBtn->setText(STR_THEME_LIGHT[i]);
        }
        else
        {
            themeBtn->setText(STR_THEME_DARK[i]);
        }
    }

    modernBtn->setText(STR_BTN_MODERN[i]);
    modernBtn->setToolTip(TIP_BTN_MODERN[i]);

    if (editGlossaryBtn)
    {
        editGlossaryBtn->setText(STR_BTN_EDIT_GLOS[i]);
        editGlossaryBtn->setToolTip(TIP_BTN_EDIT_GLOS[i]);
    }

    // Update glossary open button / 更新术语表打开按钮
    if (btnOpenAuto)
    {
        btnOpenAuto->setText(STR_BTN_AUTO[i]);
        btnOpenAuto->setToolTip(TIP_BTN_AUTO[i]);
    }

    // Update tooltips / 更新工具提示
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
    btnSelectGlossary->setToolTip(TIP_GLOSSARY[i]);
    if (glossaryCombo)
        glossaryCombo->setToolTip(TIP_GLOSSARY[i]);

    hudBtn->setToolTip(i == 0 ? "Switch to Mini-HUD mode" : "切换至迷你悬浮窗模式");

    // Update API address combo box tooltips / 更新API地址组合框工具提示
    if (apiAddressCombo)
    {
        apiAddressCombo->setToolTip(TIP_COMBO_MAIN[i]);
        for (int k = 0; k < apiAddressCombo->count(); ++k)
        {
            QString itemUrl = apiAddressCombo->itemText(k);
            if (itemUrl == "...") {
                apiAddressCombo->setItemData(k, i == 1 ? "添加自定义 API 地址" : "Add custom API URL", Qt::ToolTipRole);
                continue;
            }
            bool isPreset = false;
            for (const auto &preset : PRESETS_DATA)
            {
                if (itemUrl == preset.url)
                {
                    apiAddressCombo->setItemData(k, preset.tips[i], Qt::ToolTipRole);
                    isPreset = true;
                    break;
                }
            }
            if (!isPreset) {
                const QString presetName = ConfigManager::loadPresetNameForBaseUrl(itemUrl, "config.ini");
                apiAddressCombo->setItemData(k,
                    presetName.isEmpty() ? (i == 1 ? "自定义 API 地址" : "Custom API URL") : presetName,
                    Qt::ToolTipRole);
            }
        }
        // 💡 Force restore original text to prevent changing to preset first item or random item
        // 强制恢复原来的文本，防止变成预设的第一项或随机项
        apiAddressCombo->setCurrentText(currentApi);
    }

    // 💡 Fix: Restore data from properties and refresh display
    // 修正：从属性中恢复数据并刷新显示
    long long t = lblTokens->property("total").toLongLong();
    long long p = lblTokens->property("prompt").toLongLong();
    long long c = lblTokens->property("completion").toLongLong();

    // Call modified function to refresh text and Tooltip in one go
    // 调用刚才修改过的函数，一键刷新文字和 Tooltip
    updateTokenDisplay(t, p, c);

    // Update lock glossary checkbox text / 更新锁定术语表复选框文本
    chkLockGlossary->setText("");
    chkLockGlossary->setToolTip(i == 1 ? "锁定当前术语表路径，防止读取配置时被覆盖跳变"
                                       : "Lock Glossary Path to prevent overwriting when loading configs");

    if (logGroup && lblTokens)
    {
        lblTokens->adjustSize();
        if (logGroup->width() > 0)
        {
            lblTokens->move(logGroup->width() - lblTokens->width() - 10, 0);
        }
    }

    // 🌟 术语表编辑器的多语言同步
    if (m_glossaryEditor)
    {
        m_glossaryEditor->setWindowTitle(STR_GLOS_TITLE[i]);
        m_glossarySaveBtn->setText(STR_GLOS_SAVE[i]);
        m_glossaryCancelBtn->setText(STR_GLOS_CLOSE[i]);
    }
}

void MainWindow::applyTheme(bool isDark)
{
    // Set Fusion style for consistent look across platforms / 设置Fusion风格以实现跨平台一致外观
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QColor windowColor, baseColor, textColor, btnColor, highlightColor, linkColor;
    QString qssBtnBorder, qssBtnBg, qssBtnHover;
    QString dropDownBg, dropDownHover;

    // Set color scheme based on theme / 根据主题设置配色方案
    if (isDark)
    {
        // Dark theme colors / 暗色主题颜色
        windowColor = QColor(30, 30, 30);
        baseColor = QColor(37, 37, 38);
        textColor = QColor(220, 220, 220);
        btnColor = QColor(60, 60, 60);
        highlightColor = QColor(0, 122, 204);
        linkColor = QColor(86, 156, 214);
        qssBtnBorder = "#555555";
        qssBtnBg = "#3C3C3C";
        qssBtnHover = "#505050";
        dropDownBg = "#C0C0C0";
        dropDownHover = "#FFFFFF";
        if (themeBtn)
            themeBtn->setText(STR_THEME_LIGHT[m_currentLang]);
    }
    else
    {
        // Light theme colors / 亮色主题颜色
        windowColor = QColor(240, 240, 240);
        baseColor = QColor(255, 255, 255);
        textColor = QColor(0, 0, 0);
        btnColor = QColor(225, 225, 225);
        highlightColor = QColor(0, 120, 215);
        linkColor = QColor(0, 0, 255);
        qssBtnBorder = "#C0C0C0";
        qssBtnBg = "#E1E1E1";
        qssBtnHover = "#D0D0D0";
        dropDownBg = "#4D4D4D";
        dropDownHover = "#2D2D2D";
        if (themeBtn)
            themeBtn->setText(STR_THEME_DARK[m_currentLang]);
    }

    // Apply palette colors / 应用调色板颜色
    QPalette p;
    p.setColor(QPalette::Window, windowColor);
    p.setColor(QPalette::WindowText, textColor);
    p.setColor(QPalette::Base, baseColor);
    p.setColor(QPalette::AlternateBase, windowColor);
    p.setColor(QPalette::ToolTipBase, baseColor);
    p.setColor(QPalette::ToolTipText, textColor);
    p.setColor(QPalette::Text, textColor);
    p.setColor(QPalette::Button, btnColor);
    p.setColor(QPalette::ButtonText, textColor);
    p.setColor(QPalette::Link, linkColor);
    p.setColor(QPalette::Highlight, highlightColor);
    p.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(p);

    // Apply CSS stylesheet for detailed styling / 应用CSS样式表进行详细样式设置
    QString qss = QString(R"(
        QGroupBox { border: 1px solid %1; border-radius: 5px; margin-top: 1.2em; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 3px; color: %6; }
        QPushButton { border: 1px solid %3; border-radius: 4px; background-color: %4; padding: 5px; font-weight: bold; }
        QPushButton:hover { background-color: %5; border-color: %2; }
        QPushButton:pressed { background-color: %2; color: white; border-color: %2; }
        QPushButton:disabled { background-color: transparent; border: 1px solid %1; color: gray; }
        
        QLineEdit, QComboBox { border: 1px solid %3; border-radius: 4px; background-color: %7; padding: 4px; color: palette(text); selection-background-color: %2; }
        QComboBox:hover, QLineEdit:hover { border-color: %2; }
        
        /* === 极致极简的下拉框样式 === */
        QComboBox::drop-down { 
            subcontrol-origin: padding; 
            subcontrol-position: top right; 
            width: 20px; 
            background: transparent; /* 去除原本的颜色块 */
            border-left: 1px solid %3; 
        }
        QComboBox::drop-down:hover { background: rgba(128, 128, 128, 0.15); }
        QComboBox::down-arrow { 
            image: none; 
            width: 10px; 
            height: 2px; /* 高度2像素的矩形，完美充当 '-' 减号 */
            background-color: %8; /* 跟随主题文字颜色 */
        }
        QComboBox QAbstractItemView { border: 1px solid %2; selection-background-color: %2; background-color: %7; outline: none; }
        
        QPushButton#btnStart { background-color: #388E3C; color: white; border: 1px solid #2E7D32; }
        QPushButton#btnStart:hover { background-color: #4CAF50; border-color: #43A047; }
        QPushButton#btnStart:pressed { background-color: #1B5E20; border-color: #1B5E20; }
        QPushButton#btnStart:disabled { background-color: transparent; border: 1px solid %1; color: gray; }

        QPushButton#btnReload { background-color: #0078D4; color: white; border: 1px solid #005A9E; }
        QPushButton#btnReload:hover { background-color: #2B88D8; border-color: #005A9E; }
        QPushButton#btnReload:pressed { background-color: #004578; border-color: #004578; }

        QLabel#lblTokens { color: #E6B422; font-weight: bold; }
        QToolTip { border: 1px solid %2; background-color: %7; color: #E6B422; opacity: 230; padding: 4px; border-radius: 3px; }
    )")
                      .arg(qssBtnBorder)
                      .arg(highlightColor.name())
                      .arg(qssBtnBorder)
                      .arg(qssBtnBg)
                      .arg(qssBtnHover)
                      .arg(highlightColor.name())
                      .arg(baseColor.name())
                      .arg(textColor.name())
                      .arg(dropDownBg)
                      .arg(dropDownHover);

    qApp->setStyleSheet(qss);
    m_isDarkTheme = isDark;

    // 🌟 术语表编辑器的主题同步
    if (m_glossaryTextEdit)
    {
        if (isDark)
        {
            m_glossaryTextEdit->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; font-family: Consolas, monospace; font-size: 13px; border-radius: 4px; border: 1px solid #555555;}");
        }
        else
        {
            m_glossaryTextEdit->setStyleSheet("QTextEdit { background-color: #f5f5f5; color: #333333; font-family: Consolas, monospace; font-size: 13px; border-radius: 4px; border: 1px solid #cccccc;}");
        }

        auto highlighters = m_glossaryTextEdit->document()->findChildren<QSyntaxHighlighter *>();
        if (!highlighters.isEmpty())
        {
            static_cast<GlossaryHighlighter *>(highlighters.first())->setTheme(isDark);
        }
    }
}

// ==========================================
// 🚀 完整的 setupUi 函数
// ==========================================
void MainWindow::setupUi()
{
    // Create central widget / 创建中心部件
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // API Configuration Group / API配置分组
    cfgGroup = new QGroupBox(this);
    // Restoration: Clean style / 还原：清除之前的顶部边距样式，回归原生
    cfgGroup->setStyleSheet("");

    // 因为窗口定死500宽，所以完美坐标就是 X=286 和 X=382。
    // Y=2 将它微微向下推移 2 个像素，不仅和文字中心对齐，更确保发光上边框 100% 完整存活！

    modernBtn = new QPushButton(STR_BTN_MODERN[m_currentLang], cfgGroup);
    modernBtn->setCursor(Qt::PointingHandCursor);
    modernBtn->setFixedSize(88, 17);
    modernBtn->setToolTip(TIP_BTN_MODERN[m_currentLang]);
    modernBtn->setStyleSheet(
        "QPushButton { background-color: rgba(156, 39, 176, 0.08); color: #9C27B0; border: 1px solid rgba(156, 39, 176, 0.5); border-radius: 4px; font-size: 11px; font-weight: bold; text-align: center; padding: 0px; } "
        "QPushButton:hover { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 #E6B422, stop:1 #FFD700); color: #222222; border: 1px solid #FFD700; } "
        "QPushButton:pressed { background-color: #B8860B; border-color: #B8860B; }");
    connect(modernBtn, &QPushButton::clicked, this, &MainWindow::onSwitchToModern);
    modernBtn->move(300, 0); // 绝对定位！
    modernBtn->raise();

    editGlossaryBtn = new QPushButton(STR_BTN_EDIT_GLOS[m_currentLang], cfgGroup);
    editGlossaryBtn->setCursor(Qt::PointingHandCursor);
    editGlossaryBtn->setFixedSize(88, 17);
    editGlossaryBtn->setToolTip(TIP_BTN_EDIT_GLOS[m_currentLang]);
    editGlossaryBtn->setStyleSheet(
        "QPushButton { background-color: rgba(33, 150, 243, 0.08); color: #2196F3; border: 1px solid rgba(33, 150, 243, 0.5); border-radius: 4px; font-size: 11px; font-weight: bold; text-align: center; padding: 0px; } "
        "QPushButton:hover { background-color: rgba(33, 150, 243, 0.8); color: white; border: 1px solid #2196F3; } "
        "QPushButton:pressed { background-color: #1976D2; }");
    connect(editGlossaryBtn, &QPushButton::clicked, this, &MainWindow::openGlossaryEditor);
    editGlossaryBtn->move(391, 0); // 绝对定位！
    editGlossaryBtn->raise();

    QGridLayout *grid = new QGridLayout(cfgGroup);
    grid->setColumnStretch(1, 1);

    grid->setVerticalSpacing(8);
    grid->setHorizontalSpacing(10);

    // Lambda function for creating aligned labels / 用于创建对齐标签的Lambda函数
    auto createLabel = [this](QLabel *&memberPtr)
    {
        memberPtr = new QLabel(this);
        memberPtr->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return memberPtr;
    };

    // Row 0: API Address (Standard)
    apiAddressCombo = new QComboBox(this);
    apiAddressCombo->setEditable(true);
    apiAddressCombo->setMinimumHeight(28);
    apiAddressCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(apiAddressCombo, &QComboBox::customContextMenuRequested, this, &MainWindow::onApiComboContextMenu);
    for (const auto &p : PRESETS_DATA)
    {
        apiAddressCombo->addItem(p.url);
        apiAddressCombo->setItemData(apiAddressCombo->count() - 1, p.tips[m_currentLang], Qt::ToolTipRole);
    }
    
    // 加载自定义 API URLs
    AppConfig startupCfg = ConfigManager::loadConfig();
    for (const QString &customUrl : startupCfg.custom_api_urls)
    {
        if (!customUrl.isEmpty() && apiAddressCombo->findText(customUrl) == -1)
        {
            apiAddressCombo->addItem(customUrl);
            const QString presetName = ConfigManager::loadPresetNameForBaseUrl(customUrl, "config.ini");
            apiAddressCombo->setItemData(apiAddressCombo->count() - 1,
                presetName.isEmpty() ? (m_currentLang == 1 ? "自定义 API 地址" : "Custom API URL") : presetName,
                Qt::ToolTipRole);
        }
    }
    
    // 添加 "+" 选项
    apiAddressCombo->addItem("+");
    apiAddressCombo->setItemData(apiAddressCombo->count() - 1,
        m_currentLang == 1 ? "添加自定义 API 地址" : "Add custom API URL", Qt::ToolTipRole);
        
    apiAddressCombo->setCurrentIndex(0);

    connect(apiAddressCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (apiAddressCombo->itemText(index) == "+") {
            apiAddressCombo->blockSignals(true);
            apiAddressCombo->setCurrentIndex(0);
            apiAddressCombo->blockSignals(false);

            ClassicPresetDialog dlg(this, m_currentLang, m_isDarkTheme);
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
                newName.isEmpty() ? (m_currentLang == 1 ? "自定义 API 地址" : "Custom API URL") : newName,
                Qt::ToolTipRole);
            
            apiAddressCombo->setCurrentIndex(targetIndex);
            apiAddressCombo->setCurrentText(newUrl);
            apiKeyEdit->setText(newKey);
            modelCombo->setCurrentText(newModel);
        }
    });

    grid->addWidget(createLabel(lblApiAddr), 0, 0);
    grid->addWidget(apiAddressCombo, 0, 1);

    // Row 1: API Key
    apiKeyEdit = new QLineEdit(this);
    apiKeyEdit->setEchoMode(QLineEdit::Normal);
    grid->addWidget(createLabel(lblApiKey), 1, 0);
    grid->addWidget(apiKeyEdit, 1, 1);

    // Row 2: Model Selection
    QWidget *modelContainer = new QWidget(this);
    QHBoxLayout *modelLayout = new QHBoxLayout(modelContainer);
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelLayout->setSpacing(5);

    modelCombo = new QComboBox(this);
    modelCombo->setEditable(true);
    modelCombo->setMinimumHeight(28);

    fetchModelBtn = new QPushButton(this);
    connect(fetchModelBtn, &QPushButton::clicked, this, &MainWindow::onFetchModels);

    modelLayout->addWidget(modelCombo, 1);
    modelLayout->addWidget(fetchModelBtn);
    grid->addWidget(createLabel(lblModel), 2, 0);
    grid->addWidget(modelContainer, 2, 1);

    // ---------------------------------------------------------
    // Row 3: Server Parameters
    // ---------------------------------------------------------

    // 1. 将 Port 标签单独剥离，放回网格的第 0 列，确保它和 Model Name 绝对垂直右对齐！
    grid->addWidget(createLabel(lblPort), 3, 0);

    // 2. 剩下的参数放入容器
    QWidget *paramContainer = new QWidget(this);
    QHBoxLayout *paramLayout = new QHBoxLayout(paramContainer);
    paramLayout->setContentsMargins(0, 0, 0, 0);
    paramLayout->setSpacing(4); // 保持紧凑间距

    portEdit = new QLineEdit(this);
    portEdit->setMinimumWidth(48); // 🌟 解开死锁
    portEdit->setAlignment(Qt::AlignCenter);

    lblThread = new QLabel(this);
    threadSpin = new QSpinBox(this);
    threadSpin->setRange(1, 1000);
    threadSpin->setMinimumWidth(50); // 🌟 解开死锁
    threadSpin->setAlignment(Qt::AlignCenter);

    lblTemp = new QLabel(this);
    tempSpin = new QDoubleSpinBox(this);
    tempSpin->setRange(0, 2);
    tempSpin->setSingleStep(0.1);
    tempSpin->setMinimumWidth(50); // 🌟 解开死锁
    tempSpin->setAlignment(Qt::AlignCenter);

    lblCtx = new QLabel(this);
    contextSpin = new QSpinBox(this);
    contextSpin->setRange(0, 20);
    contextSpin->setMinimumWidth(50); // 🌟 解开死锁
    contextSpin->setAlignment(Qt::AlignCenter);

    clearCtxBtn = new QPushButton(this);
    clearCtxBtn->setFixedWidth(40);
    connect(clearCtxBtn, &QPushButton::clicked, this, &MainWindow::onClearContext);

    lblTokens = new QLabel(this);
    lblTokens->setObjectName("lblTokens");

    // 依次放入容器（注意这里不再包含 lblPort）
    paramLayout->addWidget(portEdit, 1); // 🌟 权重 1
    paramLayout->addWidget(lblThread);
    paramLayout->addWidget(threadSpin, 1); // 🌟 权重 1
    paramLayout->addWidget(lblTemp);
    paramLayout->addWidget(tempSpin, 1); // 🌟 权重 1
    paramLayout->addWidget(lblCtx);
    paramLayout->addWidget(contextSpin, 1); // 🌟 权重 1

    // 🌟 核心魔法：放一个弹簧，将 Clear/Clr 按钮死死推到最右侧边界！
    // paramLayout->addStretch();
    paramLayout->addWidget(clearCtxBtn);

    // 3. 将容器整体放进网格的第 1 列，它将和上方的 Fetch 按钮共享右侧对齐线！
    grid->addWidget(paramContainer, 3, 1);

    // ---------------------------------------------------------
    // ---------------------------------------------------------

    // Right: Text Area
    systemPromptEdit = new QTextEdit(this);
    systemPromptEdit->setFixedHeight(166);

    // Left: Sidebar Container
    QWidget *sysLabelContainer = new QWidget(this);
    QVBoxLayout *sysLabelLayout = new QVBoxLayout(sysLabelContainer);
    sysLabelLayout->setContentsMargins(0, 0, 0, 0);
    sysLabelLayout->setSpacing(4);

    // 1. 标签：系统提示
    lblSysPrompt = new QLabel(this);
    lblSysPrompt->setAlignment(Qt::AlignRight | Qt::AlignTop); // 靠右，靠上

    // 2. 容器：锁定开关
    chkLockSysPrompt = new QCheckBox(this);
    chkLockSysPrompt->setLayoutDirection(Qt::RightToLeft); // 文字在左，框在右
    QWidget *lockContainer = new QWidget(this);
    QHBoxLayout *lockLayout = new QHBoxLayout(lockContainer);
    lockLayout->setContentsMargins(0, 0, 0, 0);
    lockLayout->addStretch(); // 弹簧在左，把控件推到右边
    lockLayout->addWidget(chkLockSysPrompt);

    // 3. 容器：测速开关
    chkDebug = new QCheckBox(this);
    chkDebug->setObjectName("chkDebug");
    chkDebug->setCursor(Qt::PointingHandCursor);
    chkDebug->setLayoutDirection(Qt::RightToLeft); // 文字在左，框在右

    QWidget *debugContainer = new QWidget(this);
    QHBoxLayout *debugLayout = new QHBoxLayout(debugContainer);
    debugLayout->setContentsMargins(0, 0, 0, 0);
    debugLayout->addStretch(); // 弹簧在左，把控件推到右边
    debugLayout->addWidget(chkDebug);

    // 连接测速开关信号
    connect(chkDebug, &QCheckBox::toggled, this, [this](bool checked)
            {
        QString msg = (m_currentLang == 1) ?
            QString("🛠️ 测速模式: %1").arg(checked ? "已开启" : "已关闭") :
            QString("🛠️ Speed Test Mode: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);

        if (m_isServerRunning && server) {
            server->updateConfig(getUiConfig());
        } });

    // --- 4. 🌟 新增：文本处理开关 (HandleRichText) ---
    chkHandleRichText = new QCheckBox(this);
    chkHandleRichText->setObjectName("chkHandleRichText");
    chkHandleRichText->setCursor(Qt::PointingHandCursor);
    chkHandleRichText->setLayoutDirection(Qt::RightToLeft); // 文字在左，框在右

    QWidget *handleRichTextContainer = new QWidget(this);
    QHBoxLayout *handleRichTextLayout = new QHBoxLayout(handleRichTextContainer);
    handleRichTextLayout->setContentsMargins(0, 0, 0, 0);
    handleRichTextLayout->addStretch();
    handleRichTextLayout->addWidget(chkHandleRichText);

    // 连接文本处理开关信号
    connect(chkHandleRichText, &QCheckBox::toggled, this, [this](bool checked)
            {
        if (!chkBatch->isChecked()) return; // 🔥 在多行模式未启用时，屏蔽相关日志与服务器动态更新

        QString msg = (m_currentLang == 1) ?
            QString("📝 文本处理: %1").arg(checked ? "已开启" : "已关闭") :
            QString("📝 HandleRichText: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);

        if (m_isServerRunning && server) {
            server->updateConfig(getUiConfig());
        } });

    // --- 5. 🌟 新增：打包翻译开关 ---
    chkBatch = new QCheckBox(this);
    chkBatch->setObjectName("chkBatch");
    chkBatch->setCursor(Qt::PointingHandCursor);
    chkBatch->setLayoutDirection(Qt::RightToLeft); // 保持队形：文字左，框右

    // 🌟 新增：绑定右键菜单策略
    chkBatch->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(chkBatch, &QCheckBox::customContextMenuRequested, this, &MainWindow::onBatchContextMenu);

    connect(chkBatch, &QCheckBox::toggled, this, [this](bool checked)
            {
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

        QString msg = (m_currentLang == 1) ?
            QString("📦 多行模式: %1").arg(checked ? "已开启" : "已关闭") :
            QString("📦 Batch Mode: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);

        if (m_isServerRunning && server) {
            server->updateConfig(getUiConfig());
        } });

    QWidget *batchContainer = new QWidget(this);
    QHBoxLayout *batchLayout = new QHBoxLayout(batchContainer);
    batchLayout->setContentsMargins(0, 0, 0, 0);
    batchLayout->addStretch();
    batchLayout->addWidget(chkBatch);

    // --- 6. 🌟 新增：提取换行开关 ---
    chkExtractNewline = new QCheckBox(this);
    chkExtractNewline->setObjectName("chkExtractNewline");
    chkExtractNewline->setCursor(Qt::PointingHandCursor);
    chkExtractNewline->setLayoutDirection(Qt::RightToLeft); // 保持队形：文字左，框右

    connect(chkExtractNewline, &QCheckBox::toggled, this, [this](bool checked)
            {
        if (!chkBatch->isChecked()) return; // 🔥 在多行模式未启用时，屏蔽相关日志与服务器动态更新

        QString msg = (m_currentLang == 1) ?
            QString("↩️ 保留换行: %1").arg(checked ? "已开启" : "已关闭") :
            QString("↩️ Keep \\n: %1").arg(checked ? "ON" : "OFF");
        LogManager::instance().addLog(msg);

        if (m_isServerRunning && server) {
            server->updateConfig(getUiConfig());
        } });

    // 🔥 初始化子功能状态
    if (chkHandleRichText) {
        chkHandleRichText->setEnabled(chkBatch->isChecked());
        QGraphicsOpacityEffect *op1 = new QGraphicsOpacityEffect(chkHandleRichText);
        op1->setOpacity(chkBatch->isChecked() ? 1.0 : 0.4);
        chkHandleRichText->setGraphicsEffect(op1);
    }
    if (chkExtractNewline) {
        chkExtractNewline->setEnabled(chkBatch->isChecked());
        QGraphicsOpacityEffect *op2 = new QGraphicsOpacityEffect(chkExtractNewline);
        op2->setOpacity(chkBatch->isChecked() ? 1.0 : 0.4);
        chkExtractNewline->setGraphicsEffect(op2);
    }

    QWidget *newlineContainer = new QWidget(this);
    QHBoxLayout *newlineLayout = new QHBoxLayout(newlineContainer);
    newlineLayout->setContentsMargins(0, 0, 0, 0);
    newlineLayout->addStretch();
    newlineLayout->addWidget(chkExtractNewline);

    // 🌟🌟🌟 核心排序区 🌟🌟🌟
    sysLabelLayout->addWidget(lblSysPrompt);      // 1. 标题
    sysLabelLayout->addWidget(lockContainer);     // 2. 锁定
    sysLabelLayout->addWidget(debugContainer);    // 3. 测速
    sysLabelLayout->addWidget(batchContainer);    // 4. 打包
    sysLabelLayout->addWidget(chkExtractNewline); // 5. 提取换行 (NEW)

    sysLabelLayout->addWidget(handleRichTextContainer); // 6. 文本处理 (新加入！)

    sysLabelLayout->addStretch(); // 7. 弹簧

    // ---------------------------------------------------------
    // 加入主网格 (Grid Addition)
    // ---------------------------------------------------------
    // 🔥 关键：只加一次，且带上 Qt::AlignTop 参数！
    grid->addWidget(sysLabelContainer, 4, 0, Qt::AlignTop);
    grid->addWidget(systemPromptEdit, 4, 1, Qt::AlignTop);

    // ---------------------------------------------------------
    // Row 5: Pre-Prompt
    // ---------------------------------------------------------
    prePromptEdit = new QLineEdit(this);
    grid->addWidget(createLabel(lblPrePrompt), 5, 0);
    grid->addWidget(prePromptEdit, 5, 1);

    // ---------------------------------------------------------
    // Row 6: Glossary (Perfect Alignment & Stretch)
    // ---------------------------------------------------------
    QWidget *glossaryContainer = new QWidget(this);
    QHBoxLayout *glossaryLayout = new QHBoxLayout(glossaryContainer);
    glossaryLayout->setContentsMargins(0, 0, 0, 0);
    glossaryLayout->setSpacing(6); // 保证组件间有安全距离，绝不拥挤

    glossaryCombo = new QComboBox(this);
    glossaryCombo->setEditable(true);
    glossaryCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(glossaryCombo, &QComboBox::customContextMenuRequested, this, &MainWindow::onGlossaryContextMenu);

    glossaryCombo->setMinimumHeight(28);
    // 🌟 破除诅咒1：放弃 Ignored，使用 Expanding 完美填满！
    // 配合 AdjustToMinimumContentsLengthWithIcon，即使选择长达 1000 字符的路径，也绝不会把窗口撑破！
    glossaryCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    glossaryCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    glossaryCombo->setMinimumWidth(100);

    btnOpenAuto = new QPushButton(STR_BTN_AUTO[m_currentLang], this);
    btnOpenAuto->setFixedSize(50, 28);
    connect(btnOpenAuto, &QPushButton::clicked, this, &MainWindow::onOpenAutoTranslations);

    btnSelectGlossary = new QPushButton("...", this);
    btnSelectGlossary->setFixedSize(35, 28);
    connect(btnSelectGlossary, &QPushButton::clicked, this, &MainWindow::onSelectGlossary);
    btnSelectGlossary->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(btnSelectGlossary, &QPushButton::customContextMenuRequested, this, &MainWindow::onGlossaryContextMenu);

    glossaryLayout->addWidget(glossaryCombo, 1); // 权重 1 保证独占拉伸
    glossaryLayout->addWidget(btnOpenAuto);
    glossaryLayout->addWidget(btnSelectGlossary);

    // --- 左侧：标签与勾选框对齐容器 ---
    QWidget *glossaryLabelContainer = new QWidget(this);
    QVBoxLayout *glossaryLabelLayout = new QVBoxLayout(glossaryLabelContainer);
    glossaryLabelLayout->setContentsMargins(0, 0, 0, 0);
    glossaryLabelLayout->setSpacing(4);

    lblGlossary = new QLabel(this);
    lblGlossary->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    chkLockGlossary = new QCheckBox(this);

    QWidget *topLabelContainer = new QWidget(this);
    QHBoxLayout *topLabelLayout = new QHBoxLayout(topLabelContainer);
    topLabelLayout->setContentsMargins(0, 0, 0, 0);
    topLabelLayout->setSpacing(4);
    topLabelLayout->addStretch();
    topLabelLayout->addWidget(lblGlossary);
    topLabelLayout->addWidget(chkLockGlossary);

    chkGlossary = new QCheckBox(this);
    chkGlossary->setLayoutDirection(Qt::RightToLeft);

    QWidget *gEvolveContainer = new QWidget(this);
    QHBoxLayout *gEvolveLayout = new QHBoxLayout(gEvolveContainer);
    gEvolveLayout->setContentsMargins(0, 0, 0, 0);
    gEvolveLayout->addStretch();
    gEvolveLayout->addWidget(chkGlossary);

    glossaryLabelLayout->addWidget(topLabelContainer);
    glossaryLabelLayout->addWidget(gEvolveContainer);
    // 🌟 破除诅咒2：已彻底移除 glossaryLabelLayout->addStretch(); ！断层再也不会出现！

    // 🌟 破除诅咒3：强制 Qt::AlignTop！确保即使网格变形，它们也死死钉在顶部对齐线上！
    grid->addWidget(glossaryLabelContainer, 6, 0, Qt::AlignTop);
    grid->addWidget(glossaryContainer, 6, 1, Qt::AlignTop);

    mainLayout->addWidget(cfgGroup, 0); // 0 表示配置区不参与额外高度分配，紧凑排列

    // ---------------------------------------------------------
    // Control buttons grid (Pixel-Perfect Restore)
    // 还原您提供的像素级工具栏布局
    // ---------------------------------------------------------
    QGridLayout *btnGridLayout = new QGridLayout();
    btnGridLayout->setSpacing(10);

    auto createBtn = [this](QPushButton *&btnPtr)
    {
        btnPtr = new QPushButton(this);
        btnPtr->setMinimumHeight(32);
        btnPtr->setCursor(Qt::PointingHandCursor);
        return btnPtr;
    };

    // Row 0
    btnGridLayout->addWidget(createBtn(startBtn), 0, 0);
    startBtn->setObjectName("btnStart");

    btnGridLayout->addWidget(createBtn(stopBtn), 0, 1);
    stopBtn->setEnabled(false);

    hudBtn = new QPushButton(this);
    hudBtn->setMinimumHeight(32);
    hudBtn->setCursor(Qt::PointingHandCursor);
    hudBtn->setEnabled(false);
    connect(hudBtn, &QPushButton::clicked, this, &MainWindow::switchToHud);
    btnGridLayout->addWidget(hudBtn, 0, 2);

    // Row 1 (Standard Layout)
    btnGridLayout->addWidget(createBtn(testBtn), 1, 0);
    btnGridLayout->addWidget(createBtn(exportBtn), 1, 1);
    btnGridLayout->addWidget(createBtn(loadBtn), 1, 2);

    // Row 2
    btnGridLayout->addWidget(createBtn(saveBtn), 2, 0);
    btnGridLayout->addWidget(createBtn(themeBtn), 2, 1);
    btnGridLayout->addWidget(createBtn(langBtn), 2, 2);

    // Connect button signals
    connect(themeBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(langBtn, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(testBtn, &QPushButton::clicked, this, &MainWindow::onTestConfig);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadConfig);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveConfig);
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportLog);

    mainLayout->addLayout(btnGridLayout);

    // Log area
    logGroup = new QGroupBox(this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(10, 20, 10, 10);

    logArea = new QTextEdit(this);
    logArea->setReadOnly(true);
    logArea->setMinimumHeight(150);
    logArea->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(logArea, &QTextEdit::customContextMenuRequested, this, &MainWindow::onLogContextMenu);
    logLayout->addWidget(logArea);
    mainLayout->addWidget(logGroup, 1); // 1 表示日志区拉伸获得所有的额外高度

    // 🌟 将 Tokens 认作 logGroup 的子控件，并悬浮在最顶层
    lblTokens->setParent(logGroup);
    lblTokens->raise();

    // Create loading overlay
    fetchLoadingOverlay = new LoadingOverlay(fetchModelBtn);

    // ✨✨✨ 新增：初始化测试按钮的遮罩
    testLoadingOverlay = new LoadingOverlay(testBtn);
    // Connect glossary signals
    connect(glossaryCombo, &QComboBox::activated, this, &MainWindow::onGlossaryChanged);

    // ✨ 监听两个 GroupBox 的自身尺寸变化
    cfgGroup->installEventFilter(this);
    logGroup->installEventFilter(this);

    connect(glossaryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index)
            {
        Q_UNUSED(index);
        QTimer::singleShot(0, [this](){
            if (glossaryCombo->lineEdit()) {
                glossaryCombo->lineEdit()->setCursorPosition(0);
            }
        }); });
}

void MainWindow::onLogContextMenu(const QPoint &pos)
{
    // Create standard context menu for text edit / 为文本编辑框创建标准上下文菜单
    QMenu *menu = logArea->createStandardContextMenu();
    menu->addSeparator();

    // 🔥 核心修改：将动作绑定到 LogManager 的全局清空
    QAction *clearAction = menu->addAction(STR_CLEAR_LOG[m_currentLang]);
    connect(clearAction, &QAction::triggered, []()
            {
                LogManager::instance().clear(); // 调用全局清空
            });

    // Show menu at cursor position / 在光标位置显示菜单
    menu->exec(logArea->mapToGlobal(pos));
    delete menu;
}

AppConfig MainWindow::getUiConfig()
{
    // 1. 创建一个新的配置对象（此时 modern_opacity 默认为 210）
    AppConfig cfg;

    // --- 🔥 核心修复开始：防止透明度丢失 ---
    // 先从文件读取一次现有的配置，为了获取那些经典模式无法修改的“隐藏参数”
    // (比如玻璃模式的透明度、可能存在的其他未来参数)
    AppConfig savedCfg = ConfigManager::loadConfig();

    // 将文件里的透明度赋值给当前的 cfg
    // 这样，无论你在经典模式怎么折腾，都不会把玻璃模式的透明度重置回 210
    cfg.modern_opacity = savedCfg.modern_opacity;
    cfg.custom_api_urls = savedCfg.custom_api_urls;
    cfg.ui_mode = savedCfg.ui_mode;
    cfg.is_dark = savedCfg.is_dark;
    cfg.is_rounded = savedCfg.is_rounded;
    cfg.glass_render_mode = savedCfg.glass_render_mode;
    cfg.hue_shift = savedCfg.hue_shift;
    cfg.tint_intensity = savedCfg.tint_intensity;
    // --- 🔥 核心修复结束 ---

    // 2. 收集当前 UI 上的状态 (覆盖 cfg 中的对应值)
    cfg.api_address = apiAddressCombo->currentText();
    cfg.api_key = apiKeyEdit->text();

    // 检查并保存自定义 API 地址
    if (!cfg.api_address.isEmpty() && cfg.api_address != "+") {
        bool isPreset = false;
        for (const auto &p : PRESETS_DATA) {
            if (cfg.api_address == p.url) {
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
    cfg.temperature = tempSpin->value();
    cfg.context_num = contextSpin->value();
    cfg.max_threads = threadSpin->value();
    cfg.system_prompt = systemPromptEdit->toPlainText();
    cfg.pre_prompt = prePromptEdit->text();
    cfg.enable_glossary = chkGlossary->isChecked();
    cfg.enable_debug_mode = chkDebug->isChecked();
    cfg.handle_rich_text = chkHandleRichText->isChecked();
    cfg.enable_batch = chkBatch->isChecked();
    cfg.extract_newline = chkExtractNewline->isChecked();

    // 术语表路径与历史
    cfg.glossary_path = glossaryCombo->currentText();
    QStringList history;
    for (int i = 0; i < glossaryCombo->count(); ++i)
    {
        history << glossaryCombo->itemText(i);
    }
    cfg.glossary_history = history;
    cfg.language = m_currentLang;

    // UI 状态
    cfg.is_dark = m_isDarkTheme;
    cfg.ui_mode = 0; // 标记为经典模式

    // 锁定状态
    cfg.lock_system_prompt = chkLockSysPrompt->isChecked();
    cfg.lock_glossary = chkLockGlossary->isChecked();

    return cfg;
}

void MainWindow::loadConfigToUi()
{
    m_apiKeyMemoryEnabled = false;

    // 1. 抢先从文件加载配置
    AppConfig cfg = ConfigManager::loadConfig();

    // 🔥🔥🔥 CAN 的神级时序修复：必须在所有动作之前，确立语言和主题规则！
    m_currentLang = cfg.language;
    if (m_isDarkTheme != cfg.is_dark)
    {
        m_isDarkTheme = cfg.is_dark;
        applyTheme(m_isDarkTheme);
    }

    // 2. 同步日志历史 (此时 onLogMessage 拦截器已经知道当前是英文了，完美发力！)
    logArea->clear();
    QStringList logs = LogManager::instance().getHistory();
    for (const QString &msg : logs)
    {
        onLogMessage(msg);
    }

    // 3. 恢复锁定状态 (优先恢复，防止后续逻辑受阻)
    chkLockSysPrompt->setChecked(cfg.lock_system_prompt);
    chkLockGlossary->setChecked(cfg.lock_glossary);

    // 清理已经被删除的自定义预设
    for (int i = apiAddressCombo->count() - 1; i >= 0; --i)
    {
        QString itemText = apiAddressCombo->itemText(i);
        if (itemText == "+") continue;

        bool isBuiltIn = false;
        for (const auto &p : PRESETS_DATA) {
            if (p.url == itemText) {
                isBuiltIn = true;
                break;
            }
        }
        if (!isBuiltIn && !cfg.custom_api_urls.contains(itemText)) {
            apiAddressCombo->removeItem(i);
        }
    }

    // 加载最新的自定义 API URLs 到下拉框
    for (const QString &customUrl : cfg.custom_api_urls)
    {
        if (!customUrl.isEmpty() && apiAddressCombo->findText(customUrl) == -1)
        {
            // 插入到 "+" 之前
            int insIndex = apiAddressCombo->count() > 0 ? apiAddressCombo->count() - 1 : 0;
            apiAddressCombo->insertItem(insIndex, customUrl);
            QString presetName = ConfigManager::loadPresetNameForBaseUrl(customUrl, "config.ini");
            apiAddressCombo->setItemData(insIndex, 
                presetName.isEmpty() ? (m_currentLang == 1 ? "自定义 API 地址" : "Custom API URL") : presetName, Qt::ToolTipRole);
        }
    }

    // 4. 基础 UI 填充
    apiAddressCombo->setCurrentText(cfg.api_address);
    apiKeyEdit->setText(cfg.api_key);
    modelCombo->setCurrentText(cfg.model_name);
    portEdit->setText(QString::number(cfg.port));
    tempSpin->setValue(cfg.temperature);
    contextSpin->setValue(cfg.context_num);
    threadSpin->setValue(cfg.max_threads);

    // 🔥 CAN 抢修：将被遗漏的前置文本填充代码补回！
    prePromptEdit->setText(cfg.pre_prompt);

    // --- 确保测速和打包开关使用正确的语言打印日志 ---
    chkDebug->setChecked(cfg.enable_debug_mode);
    chkHandleRichText->setChecked(cfg.handle_rich_text);
    chkBatch->setChecked(cfg.enable_batch);
    chkExtractNewline->setChecked(cfg.extract_newline);
    chkGlossary->setChecked(cfg.enable_glossary);

    // 5. 系统提示词逻辑
    if (!cfg.lock_system_prompt || systemPromptEdit->toPlainText().isEmpty())
    {
        systemPromptEdit->setText(cfg.system_prompt);
    }

    // 6. 术语表逻辑 (适配版)
    if (!chkLockGlossary->isChecked())
    {
        // A. 更新历史记录列表
        if (!cfg.glossary_history.isEmpty())
        {
            glossaryCombo->clear();
            glossaryCombo->addItems(cfg.glossary_history);
        }

        // B. 更新当前显示的路径
        if (!cfg.glossary_path.isEmpty())
        {
            int index = glossaryCombo->findText(cfg.glossary_path);
            if (index != -1)
            {
                glossaryCombo->setCurrentIndex(index);
            }
            else
            {
                addToGlossaryHistory(cfg.glossary_path);
            }
        }
    }

    // C. 强制光标归零
    if (glossaryCombo->lineEdit())
    {
        glossaryCombo->lineEdit()->setCursorPosition(0);
    }

    // 7. 刷新界面文本
    updateUIText();
    apiAddressCombo->setCurrentText(cfg.api_address); // 再次确认地址未被重置

    toggleControls(server->isRunning());

    // 8. 立即同步到 Server！
    if (server)
    {
        server->updateConfig(cfg);
    }

    m_lastApiBaseUrl = normalizeApiBaseUrl(apiAddressCombo->currentText());
    m_apiKeyMemoryEnabled = true;
}

// 3. 重构 closeEvent
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isClosing)
    {
        event->accept();
        return;
    }

    // 🔥 此时 saveConfig 会自动包含 lock_glossary 等状态
    ConfigManager::saveConfig(getUiConfig(), "config.ini");

    // --- ❌ 已删除旧的 QSettings 单独写入代码 ---

    if (m_hudWindow->isVisible())
    {
        m_hudWindow->close();
    }

    event->ignore();
    m_isClosing = true;
    fadeOutAndClose();
}

void MainWindow::toggleControls(bool running)
{
    m_isServerRunning = running;

    // 1. 核心网络配置锁定 (防止运行时修改端口或地址导致冲突)
    apiAddressCombo->setEnabled(true);
    apiKeyEdit->setEnabled(true);
    tempSpin->setEnabled(true);
    modelCombo->setEnabled(true);
    contextSpin->setEnabled(true);

    portEdit->setEnabled(!running);
    threadSpin->setEnabled(!running);

    // 2. 术语表逻辑 (UNFROZEN: 允许实时切换)
    glossaryCombo->setEnabled(true);

    // 3. 🔥 HUD 按钮修复: 确保在运行状态下都不被冻结
    hudBtn->setEnabled(running);

    modernBtn->setEnabled(true);

    // 4. 省略号按钮交互 (保留第一份代码的高级视觉反馈)
    QString baseDescription = (m_currentLang == 0)
                                  ? "ℹ️ Select glossary files (.txt)\n\n💡 Tips:\n• Left-click: Browse files\n• Right-click: Open clean menu (Remove path / Clear history)"
                                  : "ℹ️ 选择术语表文件 (.txt)\n\n💡 提示：\n• 左键点击：浏览文件\n• 右键点击：打开清理菜单 (移除路径/清空历史)";

    if (running)
    {
        // 追加运行状态提示
        QString runningNotice = (m_currentLang == 0)
                                    ? "\n\n(ℹ️ Server is running: You can still click to change paths \n\n 💡 （You can also left-click to select different glossaries or right-click to open the clean menu)"
                                    : "\n\n(ℹ️ 服务运行中：您仍可点击此处更改路径 \n\n 💡 （您依旧可以左键选择不同的术语表 右键以打开清理菜单)";
        btnSelectGlossary->setToolTip(baseDescription + runningNotice);

        // 样式：平时低调，悬浮点亮
        btnSelectGlossary->setStyleSheet(
            "QPushButton { "
            "  color: rgba(200, 200, 200, 120); "
            "  border: 1px solid rgba(150, 150, 150, 60); "
            "  background-color: transparent; "
            "} "
            "QPushButton:hover { "
            "  color: #ffffff; "
            "  background-color: #3d8af7; "
            "  border: 1px solid #3d8af7; "
            "}");
    }
    else
    {
        btnSelectGlossary->setToolTip(baseDescription);
        btnSelectGlossary->setStyleSheet("");
    }

    // 5. 功能性按钮保持可用
    themeBtn->setEnabled(true);
    langBtn->setEnabled(true);
    clearCtxBtn->setEnabled(true); // 运行时允许手动清空记忆

    // 6. 服务控制按钮切换
    if (running)
    {
        startBtn->setText(STR_RELOAD[m_currentLang]); // 直接复用已有的字典
        startBtn->setEnabled(true);                   // 允许点击进行热重载
    }
    else
    {
        startBtn->setText(STR_START[m_currentLang]); // 直接复用已有的字典
    }
    stopBtn->setEnabled(running);
}

// 🔥 CRITICAL: Robust Hot Reload Logic from 1.txt / 关键：来自1.txt的鲁棒热重载逻辑
void MainWindow::onStartClicked()
{
    AppConfig newCfg = getUiConfig();

    if (m_isServerRunning)
    {
        // === 🔥 完整热重载逻辑 (Hot Reload) ===
        AppConfig oldCfg = server->getConfig();

        // 1. 模型切换检测记录
        if (oldCfg.model_name != newCfg.model_name)
        {
            LogManager::instance().addLog(QString(LOG_MODEL_SWITCH[m_currentLang])
                                              .arg(oldCfg.model_name)
                                              .arg(newCfg.model_name));
        }
        else
        {
            LogManager::instance().addLog(QString(LOG_MODEL_SAME[m_currentLang]).arg(newCfg.model_name));
        }

        // 2. 更新服务器配置
        server->updateConfig(newCfg);

        LogManager::instance().addLog(LOG_RELOADED[m_currentLang]);

        // 3. 视觉反馈：按钮闪烁动画 (从原始版本恢复)
        QPropertyAnimation *anim = new QPropertyAnimation(startBtn, "windowOpacity");
        anim->setDuration(150);
        anim->setStartValue(0.5);
        anim->setEndValue(1.0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        // 4. 自动测试连通性
        LogManager::instance().addLog(LOG_AUTO_TESTING[m_currentLang]);
        onTestConfig();
    }
    else
    {
        // === 🚀 正常启动逻辑 (Start) ===
        server->updateConfig(newCfg);
        server->startServer();
        // toggleControls(true);
    }
}

void MainWindow::onStopClicked()
{
    server->stopServer();
    // toggleControls(false);
}

void MainWindow::onLogMessage(QString msg)
{
    if (!logArea)
        return;

    // 🔥 CAN 拦截器升级版：同时涵盖"测速模式"、"多行模式"与"文本处理"！
    // 针对本地生成的纯文本日志进行语言适配
    if (m_currentLang == 0) // 0 代表英文模式 (English)
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
        if (msg.contains("文本处理"))
        {
            msg.replace("已开启", "ON");
            msg.replace("已关闭", "OFF");
            msg.replace("文本处理: ", "HandleRichText: ");
            msg.replace("文本处理：", "HandleRichText: ");
            msg.replace("文本处理", "HandleRichText");
        }
        if (msg.contains("保留换行"))
        {
            msg.replace("已开启", "ON");
            msg.replace("已关闭", "OFF");
            msg.replace("保留换行: ", "Keep \\n: ");
        }
    }
    else if (m_currentLang == 1) // 1 代表中文模式 (Chinese)
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
        if (msg.contains("Keep \\n"))
        {
            msg.replace("ON", "已开启");
            msg.replace("OFF", "已关闭");
            msg.replace("Keep \\n: ", "保留换行: ");
            msg.replace("Keep \\n", "保留换行");
        }
    }

    // 🔥 核心：append() 会自动检测并渲染 HTML 标签
    // Server 发来的类似 <span style='color:...'> 的内容将在此处被正确渲染为彩色文本
    logArea->append(msg);

    // Limit log size to prevent memory issues / 限制日志大小以防止内存问题
    const int MAX_LOG_LINES = 2000;
    if (logArea->document()->blockCount() > MAX_LOG_LINES)
    {
        QTextCursor cursor(logArea->document());
        cursor.movePosition(QTextCursor::Start);
        for (int i = 0; i < 500; ++i)
        {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
    }

    /*
    QScrollBar *sb = logArea->verticalScrollBar();
    if (sb)
    {
        sb->setValue(sb->maximum());
    }
    */
}

void MainWindow::onSaveConfig()
{
    persistCurrentApiKeyMemory();
    // Open save file dialog / 打开保存文件对话框
    QString fileName = QFileDialog::getSaveFileName(this, STR_SAVE[m_currentLang], "config.ini", "Config Files (*.ini)");
    if (!fileName.isEmpty())
    {
        ConfigManager::saveConfig(getUiConfig(), fileName);
        server->injectLog(QString(LOG_CFG_SAVED[m_currentLang]) + fileName);
    }
}

void MainWindow::onLoadConfig()
{
    // Open load file dialog / 打开加载文件对话框
    QString fileName = QFileDialog::getOpenFileName(this, STR_LOAD[m_currentLang], "", "Config Files (*.ini)");
    if (fileName.isEmpty())
        return;

    // 1. 从文件加载配置（Manager 依然只负责它能读到的基础数据）
    AppConfig cfg = ConfigManager::loadConfig(fileName);

    m_apiKeyMemoryEnabled = false;

    // 2. 更新基础 UI 元素
    apiAddressCombo->setCurrentText(cfg.api_address);
    apiKeyEdit->setText(cfg.api_key);
    modelCombo->setCurrentText(cfg.model_name);
    portEdit->setText(QString::number(cfg.port));
    tempSpin->setValue(cfg.temperature);
    contextSpin->setValue(cfg.context_num);
    threadSpin->setValue(cfg.max_threads);

    // 系统提示词锁定逻辑
    if (!chkLockSysPrompt->isChecked())
    {
        systemPromptEdit->setText(cfg.system_prompt);
    }

    prePromptEdit->setText(cfg.pre_prompt);
    chkGlossary->setChecked(cfg.enable_glossary);

    chkDebug->setChecked(cfg.enable_debug_mode);
    chkBatch->setChecked(cfg.enable_batch);

    // -------------------------------------------------------------
    // 🔥 CAN: 冻结方法实现 (The Freeze Method)
    // -------------------------------------------------------------
    // 只有在当前【锁定框未勾选】的情况下，才允许从配置文件更新术语表环境。
    // 如果已锁定，下面整个代码块将被跳过，实现术语表路径和历史记录的“瞬间冻结”。
    if (!chkLockGlossary->isChecked())
    {
        // A. 更新历史记录列表
        if (!cfg.glossary_history.isEmpty())
        {
            glossaryCombo->clear();
            glossaryCombo->addItems(cfg.glossary_history);
        }

        // B. 更新当前显示的路径
        if (!cfg.glossary_path.isEmpty())
        {
            glossaryCombo->setCurrentText(cfg.glossary_path);
        }

        // 注意：这里不再从 cfg 读取 lock_glossary_path，
        // 因为我们已经解耦了它。锁定状态由 UI 当前状态决定。
    }

    m_lastApiBaseUrl = normalizeApiBaseUrl(apiAddressCombo->currentText());
    m_apiKeyMemoryEnabled = true;

    // -------------------------------------------------------------
    // ✅ 保持强制焦点前移
    // 确保无论是否更新了路径，显示的一直是路径的开头
    // -------------------------------------------------------------
    if (glossaryCombo->lineEdit())
    {
        glossaryCombo->lineEdit()->setCursorPosition(0);
    }

    server->injectLog(QString(LOG_CFG_LOADED[m_currentLang]) + fileName);
}

void MainWindow::onExportLog()
{
    // Export log to file / 将日志导出到文件
    QString fileName = "run_log.txt";
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif
        out << logArea->toPlainText();
        server->injectLog(LOG_EXPORTED[m_currentLang]);
    }
}

void MainWindow::onFetchModels()
{
    // Get API address from combo box / 从组合框获取API地址
    QString url = apiAddressCombo->currentText();
    if (url.isEmpty())
        return;
    if (url.endsWith("/"))
        url.chop(1);

    // Show loading overlay / 显示加载覆盖层
    if (fetchLoadingOverlay)
    {
        fetchLoadingOverlay->setGeometry(fetchModelBtn->rect());
        fetchLoadingOverlay->raise();
        fetchLoadingOverlay->start();
    }

    server->injectLog(m_currentLang == 1 ? "🔍 正在获取模型列表..." : "🔍 Fetching models...");

    // Create network request / 创建网络请求
    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    QNetworkRequest req(url + "/models");
    req.setTransferTimeout(10000); // 10 second timeout / 10秒超时

    // Get first API key from comma-separated list / 从逗号分隔的列表中获取第一个API密钥
    QString key = apiKeyEdit->text().split(',')[0].trimmed();
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());

    QNetworkReply *reply = mgr->get(req);
    connect(reply, &QNetworkReply::finished, [this, reply, mgr]()
            {
        // Hide loading overlay / 隐藏加载覆盖层
        if(fetchLoadingOverlay) fetchLoadingOverlay->stop();

        if(reply->error() == QNetworkReply::NoError) {
            try {
                // Parse JSON response / 解析JSON响应
                auto jsonDoc = nlohmann::json::parse(reply->readAll().toStdString());
                modelCombo->clear();
                int count = 0;
                for(const auto& item : jsonDoc["data"]) {
                    modelCombo->addItem(QString::fromStdString(item["id"]));
                    count++;
                }
                server->injectLog(m_currentLang == 1 ? QString("✅ 获取成功！共发现 %1 个可用模型。").arg(count) 
                                                 : QString("✅ Success! Found %1 available models.").arg(count));
                modelCombo->setFocus();
            } catch(...) {
                server->injectLog("❌ " + QString(LOG_PARSE_ERR[m_currentLang]));
            }
        } else {
            // Handle error / 处理错误
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() == QNetworkReply::TimeoutError) statusCode = 999;
            server->injectLog("❌ " + getFriendlyErrorMessage(statusCode, m_currentLang));
        }
        reply->deleteLater();
        mgr->deleteLater(); });
}

void MainWindow::onTestConfig()
{
    server->injectLog(LOG_TEST_START[m_currentLang]);

    // Get API keys from comma-separated list / 从逗号分隔的列表中获取API密钥
    QStringList keys = apiKeyEdit->text().split(',', Qt::SkipEmptyParts);
    if (keys.isEmpty())
    {
        server->injectLog(LOG_NO_KEY[m_currentLang]);
        return;
    }

    // 🔥 移除：testBtn->setEnabled(false);
    // 原因：禁用按钮会导致子控件（遮罩层）也变灰，影响动画效果。
    // LoadingOverlay 已经设置了拦截鼠标事件，所以不用担心重复点击。

    // ✨✨✨ 启动加载遮罩动画
    if (testLoadingOverlay)
    {
        testLoadingOverlay->setGeometry(testBtn->rect()); // 确保覆盖整个按钮
        testLoadingOverlay->raise();                      // 浮在最上层
        testLoadingOverlay->start();                      // 开始旋转
    }

    // Prepare API endpoint URL / 准备API端点URL
    QString url = apiAddressCombo->currentText();
    if (url.endsWith("/"))
        url.chop(1);
    url += "/chat/completions";
    QString model = modelCombo->currentText();

    // Shared counters for tracking completion / 用于跟踪完成的共享计数器
    auto finishedCount = std::make_shared<int>(0);
    auto successCount = std::make_shared<int>(0);
    int total = keys.size();

    // Test each API key / 测试每个API密钥
    for (int i = 0; i < total; ++i)
    {
        QString key = keys[i].trimmed();
        // 💡 Restore previous masking logic: (... + last 8 digits) / 还原之前的脱敏逻辑：(... + 后8位)
        QString keyMasked = (key.length() > 8) ? ("..." + key.right(8)) : key;

        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        QNetworkRequest req(url);
        req.setTransferTimeout(10000); // 10 second timeout / 10秒超时

        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());

        // Prepare test request JSON / 准备测试请求JSON
        nlohmann::json j;
        j["model"] = model.toStdString();
        j["messages"] = nlohmann::json::array({{{"role", "user"}, {"content", "Hi"}}});
        j["max_tokens"] = 5;

        QNetworkReply *reply = mgr->post(req, QByteArray::fromStdString(j.dump()));

        // 💡 Capture [=] to ensure correct copy of i / 捕获 [=] 以确保 i 副本正确
        connect(reply, &QNetworkReply::finished, [this, reply, mgr, keyMasked, i, finishedCount, successCount, total]()
                {
            (*finishedCount)++;
            bool isOk = (reply->error() == QNetworkReply::NoError);
            if(isOk) (*successCount)++;

            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() == QNetworkReply::TimeoutError) statusCode = 999;

            // 💡 Construct classic output line / 构造经典输出行
            QString icon = isOk ? "✅" : "❌";
            QString status = isOk ? (m_currentLang == 1 ? "测试通过" : "PASS") 
                                 : getFriendlyErrorMessage(statusCode, m_currentLang);
            
            // Format: ✅ Key-1 (...SEPO): 测试通过 / 格式：✅ Key-1 (...SEPO): 测试通过
            server->injectLog(QString("%1 Key-%2 (%3): %4")
                            .arg(icon).arg(i + 1).arg(keyMasked).arg(status));

            // 💡 Summary prompt / 汇总提示
            if (*finishedCount == total) {
                // 🔥 移除：testBtn->setEnabled(true);
                
                // ✨✨✨ 停止加载遮罩动画
                if(testLoadingOverlay) testLoadingOverlay->stop();

                
                server->injectLog("----------------------------------");

                QString highlightColor = m_isDarkTheme ? "#f2be45" : "#9C27B0";
                QString summary = (m_currentLang == 1) 
                    ? QString("<font color='#4CAF50'>📊 测试结束！</font> <font color='%1'>成功: %2</font>, <font color='#F44336'>失败: %3</font>")
                        .arg(highlightColor).arg(*successCount).arg(total - *successCount)
                    : QString("<font color='#4CAF50'>📊 Finished!</font> <font color='%1'>Success: %2</font>, <font color='#F44336'>Failed: %3</font>")
                        .arg(highlightColor).arg(*successCount).arg(total - *successCount);
                
                server->injectLog("<b>" + summary + "</b>");
            }

            reply->deleteLater();
            mgr->deleteLater(); });
    }
}

void MainWindow::updateTokenDisplay(long long total, long long prompt, long long completion)
{
    // 1. Store values in widget properties (Qt dynamic properties, no need to define variables in .h)
    // 1. 将数值存入控件属性中 (Qt 动态属性，不需要在 .h 中定义变量)
    lblTokens->setProperty("total", total);
    lblTokens->setProperty("prompt", prompt);
    lblTokens->setProperty("completion", completion);

    // 2. Update main interface text / 2. 更新主界面文字
    lblTokens->setText(QString("%1 %2").arg(STR_TOKENS[m_currentLang]).arg(total));

    // 3. Combine tooltip: description + detailed data (fix "splitting" issue) / 3. 组合提示：说明文字 + 详细数据 (修正"分裂"问题)
    QString strPrompt = (m_currentLang == 1) ? "输入 (Prompt):" : "Input (Prompt):";
    QString strCompletion = (m_currentLang == 1) ? "输出 (Completion):" : "Output (Completion):";

    QString fullTip = QString("<b>%1</b><br><br>%2 %3<br>%4 %5")
                          .arg(TIP_TOKENS[m_currentLang])
                          .arg(strPrompt)
                          .arg(prompt)
                          .arg(strCompletion)
                          .arg(completion);

    lblTokens->setToolTip(fullTip);

    lblTokens->adjustSize();
    if (logGroup->width() > 0)
    {
        int tokensX = logGroup->width() - lblTokens->width() - 10;
        lblTokens->move(tokensX, 0);
    }
}

void MainWindow::onClearContext()
{
    server->clearAllContexts();
}

void MainWindow::switchToHud()
{
    if (!server)
        return;

    // Fade out animation / 淡出动画
    QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(300);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);

    connect(anim, &QPropertyAnimation::finished, [this]()
            {
        this->hide();
        // Position HUD window near main window / 将HUD窗口定位在主窗口附近
        m_hudWindow->move(this->geometry().topRight() - QPoint(280, -20)); 
        m_hudWindow->show();
        m_hudWindow->setStatus(false);
        m_hudWindow->updateTokens(m_tokenManager->getTotal()); });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::restoreFromHud()
{
    m_hudWindow->hide();
    this->setWindowOpacity(0.0);
    this->show();

    // Fade in animation / 淡入动画
    QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onServerWorkStarted()
{
    if (m_hudWindow && m_hudWindow->isVisible())
    {
        m_hudWindow->setStatus(true);
    }
}

void MainWindow::onServerWorkFinished(bool success)
{
    if (m_hudWindow && m_hudWindow->isVisible())
    {
        m_hudWindow->setStatus(false, !success);
    }
}

// Glossary Mangagement Context Menu Handler / 术语表管理右键菜单处理器

void MainWindow::onGlossaryContextMenu(const QPoint &pos)
{
    int lang = m_currentLang;

    // 1. 🔥 核心修复：获取是谁触发了右键 (可能是下拉框，也可能是按钮)
    QWidget *senderWidget = qobject_cast<QWidget *>(sender());
    if (!senderWidget)
        return;

    // 创建右键菜单
    QMenu menu(this);

    // 动作 1：删除当前选中的这一条
    QAction *removeAction = menu.addAction(STR_REMOVE_PATH[lang]);
    // 动作 2：清空全部
    QAction *clearAction = menu.addAction(STR_CLEAR_HISTORY[lang]);

    // 如果当前下拉框是空的，禁用删除动作
    if (glossaryCombo->currentText().isEmpty())
    {
        removeAction->setEnabled(false);
    }

    // 2. 🔥 核心修复：根据触发者 (senderWidget) 来映射坐标
    // mapToGlobal 会将局部坐标 pos 转换为屏幕坐标
    QPoint globalPos = senderWidget->mapToGlobal(pos);

    // 在计算出的位置弹出菜单
    QAction *selectedAction = menu.exec(globalPos);

    if (selectedAction == removeAction)
    {
        // --- 逻辑：删除当前项 ---
        int index = glossaryCombo->currentIndex();
        if (index != -1)
        {
            glossaryCombo->removeItem(index);
            server->injectLog(lang == 1 ? "🧽 已从历史记录中移除该路径。" : "🧽 Removed current path from history.");
        }

        // 如果删完了，或者删的是当前显示的文本，清空输入框
        if (glossaryCombo->count() == 0 || index == -1)
        {
            glossaryCombo->setEditText("");
        }
    }
    else if (selectedAction == clearAction)
    {
        // --- 逻辑：清空全部 ---
        glossaryCombo->clear();
        glossaryCombo->setEditText(""); // 同时清空当前显示
        server->injectLog(lang == 1 ? "🗑️ 术语表历史记录已清空。" : "🗑️ Glossary history cleared.");
    }
}

// 🛠️ Human-friendly error message mapping / 人性化错误信息映射
// Added at the end of MainWindow.cpp / 在 MainWindow.cpp 的末尾添加：

QString MainWindow::getFriendlyErrorMessage(int code, int lang)
{
    if (lang == 1)
    { // 简体中文
        switch (code)
        {
        case 0:
            return "<font color=red><font color=red>网络连接超时: 无法连接到服务器，请检查 API 地址或网络代理。</font></font>";
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
    else
    { // English
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

void MainWindow::onGlossaryChanged()
{
    AppConfig currentCfg = getUiConfig();
    if (m_isServerRunning && server)
    {
        server->updateConfig(currentCfg);
        QString msg = (m_currentLang == 0)
                          ? QString("🔄 Glossary switched to: %1").arg(currentCfg.glossary_path)
                          : QString("🔄 术语表已切换至: %1").arg(currentCfg.glossary_path);

        // 🔥 错误：onLogMessage(msg);
        LogManager::instance().addLog(msg);
    }
}

void MainWindow::onSwitchToModern()
{
    // 🌟 3. 生命周期管理：当切换到了 modern model 时，优雅或直接关闭术语表编辑窗口
    if (m_glossaryEditor && m_glossaryEditor->isVisible())
    {
        m_glossaryEditor->hide(); // 直接隐藏，因为主界面马上要切换了
    }

    // 发送信号给 main.cpp 进行窗口切换
    emit requestModernView();
}

// ==========================================
// 📝 术语表实时编辑器逻辑 (优雅滑出加强版)
// ==========================================
void MainWindow::openGlossaryEditor()
{
    // 🌟 1. Toggle 逻辑：如果已显示，直接触发“淡出收回”并返回
    if (m_glossaryEditor && m_glossaryEditor->isVisible())
    {
        m_glossaryCancelBtn->click();
        return;
    }

    QString path = glossaryCombo->currentText();
    if (path.isEmpty() || !QFile::exists(path))
    {
        QMessageBox::warning(this, STR_GLOS_WARN_TITLE[m_currentLang], STR_GLOS_WARN_MSG[m_currentLang]);
        return;
    }
    m_currentEditingPath = path;

    // 2. 初始化窗口（仅在第一次点击时执行）
    if (!m_glossaryEditor)
    {
        m_glossaryEditor = new QDialog(this);
        m_glossaryEditor->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        m_glossaryEditor->setFixedSize(320, 707); // 🌟 像素级固定尺寸

        auto *dlgLayout = new QVBoxLayout(m_glossaryEditor);
        m_glossaryTextEdit = new QTextEdit(m_glossaryEditor);

        // 🌟🌟🌟 【新增核心代码】：为编辑器注入实时语法高亮器
        GlossaryHighlighter *highlighter = new GlossaryHighlighter(m_glossaryTextEdit->document());
        highlighter->setTheme(m_isDarkTheme); // 同步当前主题

        auto *bottomLayout = new QHBoxLayout();
        m_glossarySaveBtn = new QPushButton();
        m_glossaryCancelBtn = new QPushButton();
        bottomLayout->addStretch();
        bottomLayout->addWidget(m_glossarySaveBtn);
        bottomLayout->addWidget(m_glossaryCancelBtn);
        dlgLayout->addWidget(m_glossaryTextEdit);
        dlgLayout->addLayout(bottomLayout);

        connect(m_glossarySaveBtn, &QPushButton::clicked, this, &MainWindow::saveGlossaryEditor);

        // 🌟 核心：淡出收回逻辑
        connect(m_glossaryCancelBtn, &QPushButton::clicked, this, [this]()
                {
            QPropertyAnimation *fadeOut = new QPropertyAnimation(m_glossaryEditor, "windowOpacity");
            fadeOut->setDuration(250); // 快速淡出
            fadeOut->setStartValue(1.0);
            fadeOut->setEndValue(0.0);
            connect(fadeOut, &QPropertyAnimation::finished, m_glossaryEditor, &QDialog::hide);
            fadeOut->start(QAbstractAnimation::DeleteWhenStopped); });
    }

    // 3. 智能坐标计算（确保贴合、居中、不越界）
    QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    int spacing = 1;                                                 // 稍微留出一点阴影空隙
    int targetY = this->geometry().y() + (this->height() - 707) / 2; // 🌟 完美的垂直居中

    // 智能避障检测：优先尝试在左侧浮现
    bool canShowLeft = (this->geometry().x() - 320 - spacing >= screen.left());
    int targetX = canShowLeft ? (this->geometry().left() - 320 - spacing) : (this->geometry().right() + spacing);

    // 4. 数据与样式同步
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_glossaryTextEdit->setPlainText(f.readAll());
        f.close();
    }
    updateUIText();
    applyTheme(m_isDarkTheme);

    // 5. 🌟 核心：执行“浮现”动画
    m_glossaryEditor->setGeometry(targetX, targetY, 320, 707); // 直接瞬移到目标位置
    m_glossaryEditor->setWindowOpacity(0.0);                   // 初始全透明
    m_glossaryEditor->show();                                  // 此时是看不见的，因为透明度为0

    QPropertyAnimation *fadeIn = new QPropertyAnimation(m_glossaryEditor, "windowOpacity");
    fadeIn->setDuration(400); // 优雅的浮现时间
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic); // 使用平滑的贝塞尔曲线
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::saveGlossaryEditor()
{
    if (m_currentEditingPath.isEmpty())
        return;

    QFile outFile(m_currentEditingPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        outFile.write(m_glossaryTextEdit->toPlainText().toUtf8());
        outFile.close();
        LogManager::instance().addLog(LOG_GLOS_SAVED[m_currentLang]); // 极简调用

        // 通知服务器热重载新术语表
        if (m_isServerRunning && server)
        {
            server->updateConfig(getUiConfig());
        }
    }
}

void MainWindow::onBatchContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QString actionText = (m_currentLang == 1) ? "🔄 强制还原备份 (恢复初始 .xua_bak)"
                                              : "🔄 Hard Restore Backup (.xua_bak)";
    QAction *restoreAction = menu.addAction(actionText);

    QAction *selectedAction = menu.exec(chkBatch->mapToGlobal(pos));
    if (selectedAction == restoreAction)
    {
        QString path = glossaryCombo->currentText();
        int res = XuaConfigHijacker::hardRestoreFromBackup(path);

        if (res == 0)
        {
            LogManager::instance().addLog((m_currentLang == 1) ? "✅ [多行模式] 强制还原成功！已恢复初始备份。"
                                                               : "✅ [Batch Mode] Hard restore success! Back to original .xua_bak");
        }
        else if (res == 1)
        {
            LogManager::instance().addLog((m_currentLang == 1) ? "❌ 找不到 Config.ini 路径 (请检查术语表路径是否正确)"
                                                               : "❌ Config.ini path not found.");
        }
        else if (res == 2)
        {
            LogManager::instance().addLog((m_currentLang == 1) ? "⚠️ 未找到备份文件 (.xua_bak)，说明您的配置尚未被劫持修改过。"
                                                               : "⚠️ Backup file (.xua_bak) not found. Config was not hijacked.");
        }
        else
        {
            LogManager::instance().addLog((m_currentLang == 1) ? "❌ 文件还原失败，请检查文件是否被游戏占用。"
                                                               : "❌ File restore failed, check if file is locked.");
        }
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 当监听到控件自身发生 Resize (尺寸变化) 时，重新计算悬浮按钮坐标
    if (event->type() == QEvent::Resize)
    {
        if (watched == cfgGroup && modernBtn && editGlossaryBtn)
        {
            int w = cfgGroup->width();
            editGlossaryBtn->move(w - editGlossaryBtn->width() - 9, 0);
            modernBtn->move(w - editGlossaryBtn->width() - modernBtn->width() - 15, 0);
        }
        else if (watched == logGroup && lblTokens)
        {
            lblTokens->move(logGroup->width() - lblTokens->width() - 10, 0);
        }
    }

// 其他事件正常放行
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onApiComboContextMenu(const QPoint &pos)
{
    // 只在点击下拉框自身（而不是LineEdit）时响应
    QMenu menu(this);
    QAction *actDelete = menu.addAction(m_currentLang == 1 ? "🗑️ 删除此预设" : "🗑️ Delete Preset");
    
    QAction *sel = menu.exec(apiAddressCombo->mapToGlobal(pos));
    if (sel == actDelete) {
        QString currentUrl = apiAddressCombo->currentText();
        
        // 禁止删除加号以及内置预设
        bool isBuiltIn = false;
        for (const auto &p : PRESETS_DATA) {
            if (p.url == currentUrl) {
                isBuiltIn = true;
                break;
            }
        }
        
        if (currentUrl == "+" || isBuiltIn) {
            QMessageBox::warning(this, m_currentLang == 1 ? "无法删除" : "Cannot Delete",
                               m_currentLang == 1 ? "内置预设与默认选项无法删除。" : "Built-in presets and default options cannot be deleted.");
            return;
        }

        int reply = QMessageBox::question(this, m_currentLang == 1 ? "确认删除" : "Confirm Deletion",
                                        (m_currentLang == 1 ? "确定要删除以下自定义 API 预设吗？\n" : "Are you sure you want to delete this custom API preset?\n") + currentUrl,
                                        QMessageBox::Yes | QMessageBox::No);
                                        
        if (reply == QMessageBox::Yes) {
            AppConfig cfg = ConfigManager::loadConfig();
            const QString normalizedCurrentUrl = normalizeApiBaseUrl(currentUrl);
            const QString fallbackUrl = QString(PRESETS_DATA[0].url);

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