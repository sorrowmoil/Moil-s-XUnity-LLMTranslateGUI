#pragma once

#include <QWidget>
#include <QMainWindow>
#include <QPainter>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QListWidget>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QFile>
#include <QTextStream>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QPointer>
#include <QStyleOption>
#include <QLinearGradient>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QApplication>
#include <QRandomGenerator>
#include <QPixmapCache>
#include <QImage>

#include "TranslationServer.h"
#include "GlossaryManager.h"

// ==========================================
// Global modern style sheet generator (refactored with enhanced texture)
// 全局流光样式表生成器（重构质感增强版）
// ==========================================
inline QString getModernStyle(bool isDark, bool isRounded)
{
    // Retrieve system fonts for consistent appearance
    // 获取系统字体以保证外观一致
    QString uiFont = QApplication::font().family();
    QString monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    QString style;

    if (isDark)
    {
        // Dark theme stylesheet
        // 暗色主题样式表
        style = QString(R"(
QWidget { font-family: '%1'; font-size: 12px; font-weight: 400; }
QWidget#Card { background: rgba(30, 30, 35, 90); border-radius: 8px; border: 1px solid rgba(255, 255, 255, 12); border-top: 1px solid rgba(255, 255, 255, 20); }
QLabel { color: #EAEAEA; font-weight: 500; }
QPushButton#WinBtnMin, QPushButton#WinBtnClose { background: transparent; border: none; font-size: 14px; color: rgba(255,255,255,150); }
QPushButton#WinBtnMin:hover { background: rgba(255,255,255,20); color: #fff; }
QPushButton#WinBtnClose:hover { background: rgba(255,50,50,200); color: #fff; }

QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit { 
    background: rgba(0, 0, 0, 110); 
    border: 1px solid rgba(255, 255, 255, 15); 
    color: #ffffff; padding: 2px 4px; border-radius: 4px; 
    selection-background-color: #FF8C00; selection-color: #000; 
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QTextEdit:focus { 
    background: rgba(0, 0, 0, 160); 
    border: 1px solid #FF8C00; 
}
QComboBox::drop-down { border: none; width: 20px; }
QComboBox::down-arrow { image: none; border: none; background: #FF8C00; width: 8px; height: 2px; border-radius: 1px;}
QComboBox QLineEdit { background: transparent; border: none; padding: 0px; margin: 0px; }

QPushButton#SmallFuncBtn { background: rgba(255, 255, 255, 12); color: #D0D0D0; border: 1px solid rgba(255, 255, 255, 15); border-radius: 4px; }
QPushButton#SmallFuncBtn:hover { background: rgba(255, 140, 0, 40); border-color: #FF8C00; color: #fff; }

QCheckBox { color: #CCCCCC; }
QCheckBox::indicator { border: 1px solid rgba(255,255,255,30); background: rgba(0,0,0,80); width:14px; height:14px; border-radius: 3px; }
QCheckBox::indicator:checked { background: #FF8C00; border-color: #FF8C00; }

QSlider { min-height: 20px; }
QSlider::groove:horizontal { border: none; height: 4px; background: rgba(255, 255, 255, 15); border-radius: 2px; }
QSlider::sub-page:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E5FF, stop:1 #FF8C00); border-radius: 2px; }
QSlider::handle:horizontal { background: #FF8C00; width: 14px; height: 14px; margin: -5px 0px; border-radius: 7px; }
QSlider::handle:horizontal:hover { background: #00E5FF; }

QTextEdit#LogArea { background: rgba(10, 10, 15, 160); color: #E0E0E0; font-family: '%2'; font-size: 12px; border: 1px solid rgba(255,255,255,10); border-radius: 6px; }
QTextEdit#LogArea b { color: #FF8C00; font-weight: bold; }

QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; }
QScrollBar::handle:vertical { background: rgba(255, 255, 255, 30); min-height: 20px; border-radius: 3px; }
QScrollBar::handle:vertical:hover { background: #FF8C00; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
QScrollBar:horizontal { height: 0px; background: transparent; }
)")
                    .arg(uiFont, monoFont);
    }
    else
    {
        // Light theme stylesheet
        // 亮色主题样式表
        style = QString(R"(
QWidget { font-family: '%1'; font-size: 12px; font-weight: 400; }
QWidget#Card { background: rgba(255, 255, 255, 180); border-radius: 8px; border: 1px solid rgba(255, 255, 255, 255); box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
QLabel { color: #222222; font-weight: 500; }
QPushButton#WinBtnMin, QPushButton#WinBtnClose { background: transparent; border: none; font-size: 14px; color: #555; }
QPushButton#WinBtnMin:hover { background: rgba(0,0,0,10); }
QPushButton#WinBtnClose:hover { background: rgba(255,50,50,200); color: #fff; }

QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit { 
    background: rgba(255, 255, 255, 200); 
    border: 1px solid rgba(0, 0, 0, 15); 
    color: #111111; padding: 2px 4px; border-radius: 4px; 
    selection-background-color: #9400D3; selection-color: #fff; 
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QTextEdit:focus { 
    background: #ffffff; border: 1px solid #9400D3; 
}
QComboBox::drop-down { border: none; width: 20px; }
QComboBox::down-arrow { image: none; border: none; background: #9400D3; width: 8px; height: 2px; border-radius: 1px;}
QComboBox QLineEdit { background: transparent; border: none; padding: 0px; margin: 0px; }

QPushButton#SmallFuncBtn { background: rgba(255, 255, 255, 220); color: #333; border: 1px solid rgba(0, 0, 0, 15); border-radius: 4px; }
QPushButton#SmallFuncBtn:hover { background: rgba(148, 0, 211, 20); border-color: #9400D3; color: #9400D3; }

QCheckBox { color: #333333; }
QCheckBox::indicator { border: 1px solid rgba(0,0,0,30); background: rgba(255,255,255,200); width:14px; height:14px; border-radius: 3px; }
QCheckBox::indicator:checked { background: #9400D3; border-color: #9400D3; }

QSlider { min-height: 20px; }
QSlider::groove:horizontal { border: none; height: 4px; background: rgba(0, 0, 0, 15); border-radius: 2px; }
QSlider::sub-page:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #FF8C00, stop:1 #9400D3); border-radius: 2px; }
QSlider::handle:horizontal { background: #9400D3; width: 14px; height: 14px; margin: -5px 0px; border-radius: 7px; }
QSlider::handle:horizontal:hover { background: #FF8C00; }

QTextEdit#LogArea { background: rgba(255, 255, 255, 190); color: #222; font-family: '%2'; font-size: 12px; border: 1px solid rgba(0,0,0,10); border-radius: 6px; }
QTextEdit#LogArea b { color: #9400D3; font-weight: bold; }

QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; }
QScrollBar::handle:vertical { background: rgba(0, 0, 0, 25); min-height: 20px; border-radius: 3px; }
QScrollBar::handle:vertical:hover { background: #9400D3; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
QScrollBar:horizontal { height: 0px; background: transparent; }
)")
                    .arg(uiFont, monoFont);
    }

    // If rounded corners are disabled, replace all border-radius values with 0
    // 如果禁用了圆角，将所有border-radius值替换为0
    if (!isRounded)
    {
        style.replace(QRegularExpression("radius:\\s*\\d+px;"), "radius: 0px;");
    }
    return style;
}

// ==========================================
// GlassMessageBox: a transparent, noise‑textured message box
// GlassMessageBox：一个透明、带噪点纹理的消息框
// ==========================================
class GlassMessageBox : public QDialog
{
public:
    static void warning(QWidget *parent, const QString &title, const QString &text, bool isDark, int alpha, bool isRounded)
    {
        GlassMessageBox msg(parent, title, text, isDark, alpha, isRounded);
        msg.exec();
    }

private:
    GlassMessageBox(QWidget *parent, const QString &title, const QString &text, bool isDark, int alpha, bool isRounded)
        : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog), m_isDark(isDark), m_alpha(alpha), m_isRounded(isRounded)
    {
        // Enable translucent background for custom drawing
        // 启用半透明背景以进行自定义绘制
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);
        resize(320, 150);
        if (parent)
        {
            QPoint center = parent->geometry().center();
            move(center.x() - width() / 2, center.y() - height() / 2);
        }

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(20, 20, 20, 20);
        QLabel *lblTitle = new QLabel(title);
        lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #FF8C00;");
        QLabel *lblText = new QLabel(text);
        lblText->setWordWrap(true);
        lblText->setStyleSheet(QString("font-size: 13px; color: %1;").arg(isDark ? "#E0E0E0" : "#333"));
        QPushButton *btnOk = new QPushButton("OK");
        btnOk->setFixedSize(80, 30);
        btnOk->setCursor(Qt::PointingHandCursor);
        btnOk->setStyleSheet(QString("QPushButton { background: rgba(128,128,128,30); border: 1px solid rgba(128,128,128,50); border-radius: %1px; color: %2; } QPushButton:hover { background: rgba(255,140,0,80); color: #fff; border-color: #FF8C00; }").arg(isRounded ? 6 : 0).arg(isDark ? "#fff" : "#000"));
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        btnLayout->addWidget(btnOk);
        layout->addWidget(lblTitle);
        layout->addSpacing(10);
        layout->addWidget(lblText, 1);
        layout->addLayout(btnLayout);

        // Fade‑in animation
        // 淡入动画
        setWindowOpacity(0.0);
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
        fadeAnim->setDuration(250);
        fadeAnim->setStartValue(0.0);
        fadeAnim->setEndValue(1.0);
        fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRect r = rect();
        int rds = m_isRounded ? 12 : 0;
        p.setBrush(m_isDark ? QColor(25, 30, 35, m_alpha) : QColor(245, 250, 255, m_alpha));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, rds, rds);
        // Simulate noise by drawing a second layer with varying opacity
        // 通过绘制第二层不同透明度来模拟噪点
        float noiseFactor = (m_alpha - 50) / 205.0f * 0.6f;
        if (noiseFactor > 0)
        {
            p.setOpacity(noiseFactor);
            p.drawRoundedRect(r, rds, rds);
            p.setOpacity(1.0);
        }
        // Orange border
        // 橙色边框
        p.setPen(QPen(QColor(255, 140, 0, 150), 2));
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
    }

private:
    bool m_isDark, m_isRounded;
    int m_alpha;
};

// ==========================================
// Syntax highlighter for glossary entries (format: key=value)
// 术语表条目语法高亮器（格式：键=值）
// ==========================================
class GlossaryHighlighter : public QSyntaxHighlighter
{
public:
    GlossaryHighlighter(QTextDocument *parent, bool isDark) : QSyntaxHighlighter(parent) { setTheme(isDark); }
    void setTheme(bool isDark)
    {
        eqFormat.setForeground(QColor(isDark ? "#00e5ff" : "#9400D3"));
        eqFormat.setFontWeight(QFont::Bold);
        keyFormat.setForeground(QColor(isDark ? "#FF8C00" : "#B8860B"));
        valFormat.setForeground(QColor(isDark ? "#ffffff" : "#111111"));
        commentFormat.setForeground(QColor(isDark ? "#888888" : "#999999"));
        commentFormat.setFontItalic(true);
        rehighlight();
    }

protected:
    void highlightBlock(const QString &text) override
    {
        int eqIdx = text.indexOf('=');
        int commentIdx = text.indexOf("//");
        if (commentIdx == -1)
            commentIdx = text.indexOf('#');
        // Whole line is a comment
        // 整行是注释
        if (commentIdx == 0)
        {
            setFormat(0, text.length(), commentFormat);
            return;
        }
        // Highlight key=value pair (if comment appears after the pair, stop before comment)
        // 高亮键=值对（如果注释出现在对之后，则在注释前停止）
        if (eqIdx != -1 && (commentIdx == -1 || eqIdx < commentIdx))
        {
            setFormat(0, eqIdx, keyFormat);
            setFormat(eqIdx, 1, eqFormat);
            setFormat(eqIdx + 1, (commentIdx == -1 ? text.length() : commentIdx) - eqIdx - 1, valFormat);
        }
        if (commentIdx != -1)
            setFormat(commentIdx, text.length() - commentIdx, commentFormat);
    }

private:
    QTextCharFormat keyFormat, eqFormat, valFormat, commentFormat;
};

// ==========================================
// GlossaryDrawer: a sliding panel for editing glossary files
// GlossaryDrawer：用于编辑术语表文件的滑动面板
// ==========================================
class GlossaryDrawer : public QWidget
{
    Q_OBJECT
public:
    GlossaryDrawer(QWidget *parent, const QString &filePath, bool isDark, int alpha, bool isRounded, int lang, TranslationServer *server)
        : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint), m_parent(parent), m_filePath(filePath), m_server(server)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(15, 15, 15, 15);
        m_lblTitle = new QLabel();
        layout->addWidget(m_lblTitle);
        m_editor = new QTextEdit();
        m_highlighter = new GlossaryHighlighter(m_editor->document(), isDark);
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            m_editor->setPlainText(QString::fromUtf8(file.readAll()));
            file.close();
        }
        layout->addWidget(m_editor);
        QHBoxLayout *btnLayout = new QHBoxLayout();
        m_btnSave = new QPushButton();
        m_btnClose = new QPushButton();
        connect(m_btnSave, &QPushButton::clicked, this, &GlossaryDrawer::saveAndApply);
        connect(m_btnClose, &QPushButton::clicked, this, &GlossaryDrawer::animateClose);
        btnLayout->addWidget(m_btnSave);
        btnLayout->addWidget(m_btnClose);
        layout->addLayout(btnLayout);
        updateLanguage(lang);
        updateEnv(isDark, alpha, isRounded);

        // Calculate initial and final geometries for slide‑in animation
        // 计算滑入动画的初始和最终几何位置
        QRect parentRect = m_parent->geometry();
        int w = 320;
        int h = parentRect.height() * 0.85;
        int y = parentRect.y() + (parentRect.height() - h) / 2;
        int x = parentRect.right() + 10;
        if (x + w > QGuiApplication::primaryScreen()->geometry().right())
            x = parentRect.left() - w - 10;
        m_finalRect = QRect(x, y, w, h);
        m_startRect = QRect(x > parentRect.x() ? parentRect.right() : parentRect.left(), y, 0, h);
        setGeometry(m_startRect);
        setWindowOpacity(0.0);

        // Parallel animation group for geometry and opacity
        // 用于几何和透明度的并行动画组
        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        QPropertyAnimation *geoAnim = new QPropertyAnimation(this, "geometry");
        geoAnim->setDuration(400);
        geoAnim->setStartValue(m_startRect);
        geoAnim->setEndValue(m_finalRect);
        geoAnim->setEasingCurve(QEasingCurve::OutExpo);
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
        fadeAnim->setDuration(300);
        fadeAnim->setStartValue(0.0);
        fadeAnim->setEndValue(1.0);
        group->addAnimation(geoAnim);
        group->addAnimation(fadeAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // Update visual parameters (theme, opacity, rounded corners)
    // 更新视觉参数（主题、透明度、圆角）
    void updateEnv(bool isDark, int alpha, bool isRounded)
    {
        m_isDark = isDark;
        m_alpha = alpha;
        m_isRounded = isRounded;
        m_lblTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: " + QString(isDark ? "#FF8C00" : "#9400D3") + ";");
        QString textColor = isDark ? "#ffffff" : "#000000";
        QString scrollHandle = isDark ? "rgba(255, 255, 255, 50)" : "rgba(0, 0, 0, 50)";
        QString scrollHover = isDark ? "#FF8C00" : "#9400D3";
        QString editorStyle = QString(R"(QTextEdit { font-family: "Consolas", "Microsoft YaHei"; font-size: 13px; background: transparent; border: 1px solid rgba(128,128,128,50); border-radius: 6px; padding: 5px; color: %1; } QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; } QScrollBar::handle:vertical { background: %2; min-height: 20px; border-radius: 3px; } QScrollBar::handle:vertical:hover { background: %3; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; } QScrollBar:horizontal { height: 0px; background: transparent; })").arg(textColor, scrollHandle, scrollHover);
        QString btnStyle = R"(QPushButton { background: rgba(128, 128, 128, 30); border: 1px solid rgba(128,128,128,50); border-radius: 6px; color: %1; padding: 6px; } QPushButton:hover { background: %2; color: #fff; border-color: %2; })";
        if (!isRounded)
        {
            editorStyle.replace(QRegularExpression("radius:\\s*\\d+px;"), "radius: 0px;");
            btnStyle.replace(QRegularExpression("radius:\\s*\\d+px;"), "radius: 0px;");
        }
        m_editor->setStyleSheet(editorStyle);
        m_btnSave->setStyleSheet(btnStyle.arg(textColor, isDark ? "rgba(255, 140, 0, 80)" : "rgba(148, 0, 211, 80)"));
        m_btnClose->setStyleSheet(btnStyle.arg(textColor, isDark ? "rgba(255, 50, 50, 80)" : "rgba(255, 50, 50, 80)"));
        m_highlighter->setTheme(isDark);
        update();
    }

    // Update language (texts on buttons and title)
    // 更新语言（按钮和标题的文字）
    void updateLanguage(int lang)
    {
        m_lang = lang;
        m_lblTitle->setText(lang == 1 ? "📝 术语表编辑" : "📝 Glossary Editor");
        m_btnSave->setText(lang == 1 ? "💾 保存并应用" : "💾 Save & Apply");
        m_btnClose->setText(lang == 1 ? "❌ 放弃" : "❌ Cancel");
    }

    // Set alpha value (opacity) and trigger repaint
    // 设置透明度值并触发重绘
    void setAlpha(int alpha)
    {
        if (m_alpha != alpha)
        {
            m_alpha = alpha;
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRect r = rect();
        int rds = m_isRounded ? 10 : 0;
        p.setBrush(m_isDark ? QColor(25, 30, 35, m_alpha) : QColor(245, 250, 255, m_alpha));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, rds, rds);
        float noiseFactor = (m_alpha - 50) / 205.0f * 0.6f;
        if (noiseFactor > 0)
        {
            p.setOpacity(noiseFactor);
            p.drawRoundedRect(r, rds, rds);
            p.setOpacity(1.0);
        }
        p.setPen(QPen(m_isDark ? QColor(255, 140, 0, 120) : QColor(148, 0, 211, 120), 1));
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
    }

    // Dragging support (only when clicking on top area)
    // 拖动支持（仅当点击顶部区域时）
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && e->pos().y() <= 40)
        {
            m_isDrawerDragging = true;
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        else
            m_isDrawerDragging = false;
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if ((e->buttons() & Qt::LeftButton) && m_isDrawerDragging)
            move(e->globalPosition().toPoint() - m_dragPos);
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton)
            m_isDrawerDragging = false;
    }

private:
    // Save the edited content to file and notify the server
    // 保存编辑内容到文件并通知服务器
    void saveAndApply()
    {
        QFile file(m_filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << m_editor->toPlainText();
            file.close();
            if (m_server)
            {
                m_server->injectLog(m_lang == 1 ? "✅ 术语表已更新！" : "✅ Glossary updated!");
                GlossaryManager::instance().setFilePath(m_filePath);
            }
        }
        // Temporary visual feedback on the save button
        // 保存按钮上的临时视觉反馈
        QString orig = m_btnSave->styleSheet();
        m_btnSave->setText(m_lang == 1 ? "✅ 保存成功" : "✅ Saved");
        m_btnSave->setStyleSheet(QString("QPushButton { background: rgba(56, 239, 125, 80); border: 1px solid #38ef7d; border-radius: %1px; color: #fff; padding: 6px; font-weight: bold; }").arg(m_isRounded ? 6 : 0));
        QTimer::singleShot(1500, this, [this, orig]()
                           { if(this && m_btnSave){ updateLanguage(m_lang); m_btnSave->setStyleSheet(orig); } });
    }

public:
    // Animate the closing of the drawer (reverse of opening)
    // 动画关闭抽屉（开启动画的逆过程）
    void animateClose()
    {
        if (m_isClosing)
            return;
        m_isClosing = true;
        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        QPropertyAnimation *geoAnim = new QPropertyAnimation(this, "geometry");
        geoAnim->setDuration(300);
        geoAnim->setStartValue(geometry());
        QRect endRect = geometry();
        endRect.setWidth(0);
        geoAnim->setEndValue(endRect);
        geoAnim->setEasingCurve(QEasingCurve::InExpo);
        QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "windowOpacity");
        fadeAnim->setDuration(250);
        fadeAnim->setStartValue(windowOpacity());
        fadeAnim->setEndValue(0.0);
        group->addAnimation(geoAnim);
        group->addAnimation(fadeAnim);
        connect(group, &QParallelAnimationGroup::finished, this, &QWidget::close);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

private:
    QWidget *m_parent;
    QString m_filePath;
    bool m_isDark, m_isRounded;
    int m_alpha, m_lang;
    TranslationServer *m_server;
    QLabel *m_lblTitle;
    QTextEdit *m_editor;
    QPushButton *m_btnSave, *m_btnClose;
    GlossaryHighlighter *m_highlighter;
    QRect m_finalRect, m_startRect;
    QPoint m_dragPos;
    bool m_isClosing = false, m_isDrawerDragging = false;
};

// Custom item delegate for GlassMenu list items (adapts colors to theme)
// GlassMenu列表项的自定义项委托（根据主题调整颜色）
class GlassDelegate : public QStyledItemDelegate
{
public:
    explicit GlassDelegate(bool isDark, QObject *parent = nullptr) : QStyledItemDelegate(parent), m_isDark(isDark) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        bool isSelected = option.state & QStyle::State_Selected;
        bool isHover = option.state & QStyle::State_MouseOver;

        // Use the stored theme flag instead of palette inference
        // 使用存储的主题标志而不是调色板推断
        bool isDark = m_isDark;

        QRect drawRect = option.rect.adjusted(4, 2, -4, -2);
        QColor mainColor, subColor, bgBrush, bgPen;

        if (isDark)
        {
            // Dark mode: orange‑based interaction colors
            // 暗色模式：基于橙色的交互颜色
            if (isSelected)
            {
                bgBrush = QColor(255, 140, 0, 40);
                bgPen = QColor(255, 140, 0, 180);
            }
            else if (isHover)
            {
                bgBrush = QColor(255, 140, 0, 20);
                bgPen = Qt::NoPen;
            }
            mainColor = isSelected ? QColor("#FF8C00") : QColor("#EAEAEA");
            subColor = isSelected ? QColor("#FFDAB9") : QColor("#AAAAAA");
        }
        else
        {
            // Light mode: purple‑based interaction colors
            // 亮色模式：基于紫色的交互颜色
            if (isSelected)
            {
                bgBrush = QColor(148, 0, 211, 25);
                bgPen = QColor(148, 0, 211, 150);
            }
            else if (isHover)
            {
                bgBrush = QColor(148, 0, 211, 15);
                bgPen = Qt::NoPen;
            }
            mainColor = isSelected ? QColor("#9400D3") : QColor("#222222");
            subColor = isSelected ? QColor("#7B1FA2") : QColor("#555555");
        }

        if (isSelected || isHover)
        {
            painter->setBrush(bgBrush);
            painter->setPen(bgPen);
            painter->drawRoundedRect(drawRect, 4, 4);
        }

        QStringList parts = index.data(Qt::DisplayRole).toString().split('\n');
        QString mainText = parts[0];
        QString subText = (parts.size() > 1) ? parts[1] : "";
        QRect mainRect = subText.isEmpty() ? drawRect : drawRect.adjusted(10, 2, -10, -drawRect.height() / 2);
        QFont mainFont = option.font;
        mainFont.setWeight(QFont::Medium);
        mainFont.setPointSize(mainFont.pointSize() + (subText.isEmpty() ? 0 : 1));
        painter->setFont(mainFont);
        QString elidedMain = QFontMetrics(mainFont).elidedText(mainText, Qt::ElideRight, mainRect.width());
        painter->setPen(mainColor);
        painter->drawText(mainRect, Qt::AlignVCenter | Qt::AlignLeft, elidedMain);

        if (!subText.isEmpty())
        {
            QRect subRect = drawRect.adjusted(10, drawRect.height() / 2 - 2, -10, -2);
            QFont subFont = option.font;
            subFont.setPointSize(option.font.pointSize() - 1);
            painter->setFont(subFont);
            QString elidedSub = QFontMetrics(subFont).elidedText(subText, Qt::ElideRight, subRect.width());
            painter->setPen(subColor);
            painter->drawText(subRect, Qt::AlignVCenter | Qt::AlignLeft, elidedSub);
        }
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        // Larger height if item contains a second line (subtext)
        // 如果项目包含第二行（子文本），则高度更大
        return QSize(0, index.data(Qt::DisplayRole).toString().contains('\n') ? 56 : 40);
    }

private:
    bool m_isDark;
};

// ==========================================
// GlassMenu: a custom popup menu for SideGlassCombo (with sliding animation)
// GlassMenu：为SideGlassCombo定制的弹出菜单（带滑动动画）
// ==========================================
class GlassMenu : public QWidget
{
    Q_OBJECT
public:
    explicit GlassMenu(QWidget *combo, bool isDark, int alpha, bool isRounded) : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint), m_combo(combo), m_isDark(isDark), m_alpha(alpha), m_isRounded(isRounded)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        qApp->installEventFilter(this);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 6, 4);
        m_list = new QListWidget(this);

        // Pass the accurate theme flag to the delegate
        // 将准确的主题标志传递给委托
        m_list->setItemDelegate(new GlassDelegate(m_isDark, this));

        m_list->setFrameShape(QFrame::NoFrame);
        m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QString scrollHandle = isDark ? "rgba(255, 255, 255, 50)" : "rgba(0, 0, 0, 30)";
        QString scrollHover = isDark ? "#FF8C00" : "#9400D3";
        m_list->setStyleSheet(QString(R"(QListWidget { background: transparent; outline: none; border: none; padding: 4px; } QScrollBar:vertical { border: none; background: transparent; width: 4px; margin: 0; } QScrollBar::handle:vertical { background: %1; min-height: 20px; border-radius: 2px; } QScrollBar::handle:vertical:hover { background: %2; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; })").arg(scrollHandle, scrollHover));
        layout->addWidget(m_list);
    }
    ~GlassMenu() { qApp->removeEventFilter(this); }
    QListWidget *listWidget() const { return m_list; }
    void setStartRect(const QRect &rect) { m_startRect = rect; }

    // Animate the closing of the menu (shrink and fade)
    // 动画关闭菜单（收缩并淡出）
    void animateClose()
    {
        if (m_isClosing)
            return;
        m_isClosing = true;
        m_list->setEnabled(false);
        QParallelAnimationGroup *exitGroup = new QParallelAnimationGroup(this);
        QPropertyAnimation *geoExit = new QPropertyAnimation(this, "geometry");
        geoExit->setStartValue(this->geometry());
        geoExit->setEndValue(m_startRect);
        geoExit->setDuration(300);
        geoExit->setEasingCurve(QEasingCurve::InExpo);
        QPropertyAnimation *fadeExit = new QPropertyAnimation(this, "windowOpacity");
        fadeExit->setStartValue(this->windowOpacity());
        fadeExit->setEndValue(0.0);
        fadeExit->setDuration(250);
        exitGroup->addAnimation(geoExit);
        exitGroup->addAnimation(fadeExit);
        connect(exitGroup, &QParallelAnimationGroup::finished, this, &QWidget::close);
        exitGroup->start(QAbstractAnimation::DeleteWhenStopped);
    }
signals:
    void menuClosed();

protected:
    // Event filter to detect clicks outside the menu and close it
    // 事件过滤器，检测菜单外的点击并关闭菜单
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (m_isClosing)
            return false;
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            if (this->geometry().contains(globalPos))
                return false;
            if (m_combo && m_combo->geometry().contains(m_combo->mapFromGlobal(globalPos)))
                return false;
            animateClose();
            return true;
        }
        if (event->type() == QEvent::WindowDeactivate)
        {
            if (!isActiveWindow() && !m_isClosing)
                animateClose();
        }
        return false;
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRect r = rect();
        int rds = m_isRounded ? 8 : 0;
        p.setBrush(m_isDark ? QColor(25, 30, 35, m_alpha) : QColor(245, 250, 255, m_alpha));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, rds, rds);
        float noiseFactor = (m_alpha - 50) / 205.0f * 0.6f;
        if (noiseFactor > 0)
        {
            p.setOpacity(noiseFactor);
            p.drawRoundedRect(r, rds, rds);
            p.setOpacity(1.0);
        }
        p.setPen(QPen(QColor(255, 255, 255, m_isDark ? 30 : 100), 1));
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
    }

private:
    QWidget *m_combo;
    QListWidget *m_list;
    bool m_isDark, m_isRounded, m_isClosing = false;
    int m_alpha;
    QRect m_startRect;
};

// ==========================================
// SideGlassCombo: a custom combobox that opens a sliding GlassMenu
// SideGlassCombo：一个打开滑动GlassMenu的自定义组合框
// ==========================================
class SideGlassCombo : public QComboBox
{
    Q_OBJECT
public:
    explicit SideGlassCombo(QWidget *parent = nullptr) : QComboBox(parent) {}
    void setEnv(bool isDark, int alpha, bool isRounded)
    {
        m_isDark = isDark;
        m_alpha = alpha;
        m_isRounded = isRounded;
    }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (m_activeMenu)
        {
            m_activeMenu->animateClose();
            e->accept();
            return;
        }
        showPopup();
        e->accept();
    }

    // Override to display a custom GlassMenu instead of the standard popup
    // 重写以显示自定义的GlassMenu而不是标准弹出框
    void showPopup() override
    {
        if (m_activeMenu)
            return;
        GlassMenu *menu = new GlassMenu(this, m_isDark, m_alpha, m_isRounded);
        m_activeMenu = menu;
        QListWidget *list = menu->listWidget();
        QFont baseFont = list->font();
        if (baseFont.pointSize() < 9)
            baseFont.setPointSize(9);
        list->setFont(baseFont);
        int maxContentWidth = 0;
        QFontMetrics fm(baseFont);
        QFont boldFont = baseFont;
        boldFont.setWeight(QFont::Medium);
        QFontMetrics fmBold(boldFont);
        // Populate list with items from the combobox, using tooltips for second line
        // 使用组合框的项目填充列表，将工具提示用作第二行
        for (int i = 0; i < count(); ++i)
        {
            QString url = itemText(i);
            QString tip = itemData(i, Qt::ToolTipRole).toString();
            QString display = tip.isEmpty() ? url : QString("%1\n%2").arg(tip, url);
            QListWidgetItem *item = new QListWidgetItem(display, list);
            item->setSizeHint(QSize(0, tip.isEmpty() ? 40 : 56));
            if (i == currentIndex())
                list->setCurrentRow(i);
            int currentItemWidth = tip.isEmpty() ? fmBold.horizontalAdvance(url) : qMax(fmBold.horizontalAdvance(tip), fm.horizontalAdvance(url));
            if (currentItemWidth > maxContentWidth)
                maxContentWidth = currentItemWidth;
        }
        int menuW = qBound(300, maxContentWidth + 60, 600);
        int totalH = 0;
        for (int i = 0; i < list->count(); ++i)
            totalH += list->item(i)->sizeHint().height();
        int menuH = qMin(totalH + 20, 600);
        QPoint globalPos = mapToGlobal(QPoint(0, 0));
        int finalX = globalPos.x() + width() + 10;
        bool isOnRight = true;
        if (finalX + menuW > QGuiApplication::primaryScreen()->geometry().right())
        {
            finalX = globalPos.x() - menuW - 10;
            isOnRight = false;
        }
        QRect finalRect(finalX, globalPos.y(), menuW, menuH);
        QRect startRect = finalRect;
        startRect.translate(isOnRight ? -60 : 60, 0);
        menu->setStartRect(startRect);
        menu->setGeometry(startRect);
        menu->show();
        // Animate menu entrance
        // 动画菜单进入
        QParallelAnimationGroup *enterGroup = new QParallelAnimationGroup(menu);
        QPropertyAnimation *geoEnter = new QPropertyAnimation(menu, "geometry");
        geoEnter->setStartValue(startRect);
        geoEnter->setEndValue(finalRect);
        geoEnter->setDuration(500);
        geoEnter->setEasingCurve(QEasingCurve::OutExpo);
        QPropertyAnimation *fadeEnter = new QPropertyAnimation(menu, "windowOpacity");
        fadeEnter->setStartValue(0.0);
        fadeEnter->setEndValue(1.0);
        fadeEnter->setDuration(300);
        enterGroup->addAnimation(geoEnter);
        enterGroup->addAnimation(fadeEnter);
        enterGroup->start(QAbstractAnimation::DeleteWhenStopped);
        connect(list, &QListWidget::itemClicked, [this, menu, list](QListWidgetItem *item)
                { this->setCurrentIndex(list->row(item)); menu->animateClose(); });
    }

private:
    bool m_isDark = true, m_isRounded = true;
    int m_alpha = 200;
    QPointer<GlassMenu> m_activeMenu;
};

// ==========================================
// GlassCard: a widget with a glowing animated border
// GlassCard：带有发光动画边框的控件
// ==========================================
class GlassCard : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float glowFactor READ glowFactor WRITE setGlowFactor)
public:
    explicit GlassCard(bool isDark, QWidget *parent = nullptr) : QWidget(parent), m_isDark(isDark)
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setObjectName("Card");
        m_pulseAnim = new QPropertyAnimation(this, "glowFactor", this);
        m_pulseAnim->setDuration(6000);
        m_pulseAnim->setStartValue(0.7f);
        m_pulseAnim->setEndValue(1.1f);
        m_pulseAnim->setEasingCurve(QEasingCurve::InOutSine);
        m_pulseAnim->setLoopCount(-1);
        m_pulseAnim->start();
    }
    void setTheme(bool isDark)
    {
        m_isDark = isDark;
        update();
    }
    void setRounded(bool r)
    {
        m_isRounded = r;
        update();
    }
    float glowFactor() const { return m_glowFactor; }
    void setGlowFactor(float f)
    {
        m_glowFactor = f;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QStyleOption opt;
        opt.initFrom(this);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        QRect r = rect();
        int radius = m_isRounded ? 8 : 0;
        QLinearGradient glowGrad(r.topLeft(), r.bottomRight());
        if (m_isDark)
        {
            glowGrad.setColorAt(0.0, QColor(0, 229, 255));
            glowGrad.setColorAt(0.5, QColor(156, 39, 176));
            glowGrad.setColorAt(1.0, QColor(255, 140, 0));
        }
        else
        {
            glowGrad.setColorAt(0.0, QColor(178, 34, 34));
            glowGrad.setColorAt(0.5, QColor(218, 165, 32));
            glowGrad.setColorAt(1.0, QColor(255, 140, 0));
        }
        float maxAlpha = m_isDark ? 0.6f : 0.4f;
        // Draw multiple layers of glow with decreasing size and increasing transparency
        // 绘制多层发光，尺寸递减，透明度递增
        for (int i = 0; i < 6; ++i)
        {
            float layerFactor = (6 - i) / 6.0f;
            float finalOpacity = maxAlpha * layerFactor * m_glowFactor;
            if (finalOpacity <= 0.05f)
                break;
            p.setOpacity(qBound(0.0f, finalOpacity, 1.0f));
            QPen pen;
            pen.setBrush(glowGrad);
            pen.setWidth(1);
            p.setPen(pen);
            int shrink = 1 + i;
            p.drawRoundedRect(r.adjusted(shrink, shrink, -shrink, -shrink), radius > 0 ? radius - 1 : 0, radius > 0 ? radius - 1 : 0);
        }
        p.setOpacity(1.0);
        QPen glintPen;
        QLinearGradient glint(r.topLeft(), r.bottomRight());
        glint.setColorAt(0.0, QColor(255, 255, 255, m_isDark ? 100 : 180));
        glint.setColorAt(0.4, QColor(255, 255, 255, 0));
        glint.setColorAt(1.0, QColor(255, 255, 255, m_isDark ? 60 : 120));
        p.setPen(QPen(glint, 1));
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), radius, radius);
        p.setPen(QPen(QColor(255, 255, 255, m_isDark ? 50 : 150), 1));
        p.drawLine(r.left() + radius, r.top() + 1, r.right() - radius, r.top() + 1);
    }

private:
    bool m_isDark, m_isRounded = true;
    float m_glowFactor = 1.0f;
    QPropertyAnimation *m_pulseAnim;
};