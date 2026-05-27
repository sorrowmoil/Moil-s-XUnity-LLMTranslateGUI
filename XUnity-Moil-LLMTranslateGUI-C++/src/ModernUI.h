#pragma once

#include <QWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
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
#include <QSlider>
#include <QGraphicsEffect>

#include "TranslationServer.h"
#include "GlossaryManager.h"

// ==========================================
// 🎨 主题渲染模式枚举
// ==========================================
enum class GlassRenderMode
{
    Frosted, // 原生毛玻璃效果（新版）
    Legacy   // 伪毛玻璃发光效果（旧版）
};

// ==========================================
// 🎨 色相偏移工具函数
// ==========================================
inline QColor shiftHue(const QColor &color, int hueOffset)
{
    if (hueOffset == 0) return color;
    int h, s, l, a;
    color.getHsl(&h, &s, &l, &a);
    if (h != -1) // 忽略灰度颜色
    {
        h = (h + hueOffset) % 360;
        if (h < 0) h += 360;
        return QColor::fromHsl(h, s, l, a);
    }
    return color;
}

// ==========================================
// 🎨 全局流光样式表生成器 (重构质感爆炸版)
// ==========================================
inline QString getModernStyle(bool isDark, bool isRounded)
{
    QString uiFont;
    QString monoFont;
    QString style;

    // UI 字体优先级选择
    if (QFontDatabase().families().contains("萝莉体"))
    {
        uiFont = "萝莉体";
    }
    else if (QFontDatabase().families().contains("Microsoft YaHei"))
    {
        uiFont = "Microsoft YaHei";
    }
    else
    {
        uiFont = "Source Han Sans"; // 思源黑体
    }

    // Mono 字体优先级选择
    if (QFontDatabase().families().contains("萝莉体"))
    {
        monoFont = "萝莉体";
    }
    else if (QFontDatabase().families().contains("Microsoft YaHei"))
    {
        monoFont = "Microsoft YaHei";
    }
    else
    {
        monoFont = "Consolas";
    }

    // 样式拼接
    style += QString("font-family: '%1';").arg(uiFont);

    if (isRounded)
    {
        style += " border-radius: 8px;";
    }
    else
    {
        style += " border-radius: 0px;";
    }

    if (isDark)
    {
        style = QString(R"(
QWidget { font-family: '%1'; font-size: 12px; font-weight: 400; }
QToolTip { color: #EAEAEA; background-color: rgba(30, 30, 35, 230); border: 1px solid rgba(255, 255, 255, 30); border-radius: 4px; padding: 4px; }
QWidget#Card { background: rgba(30, 30, 35, 90); border-radius: 8px; border: 1px solid rgba(255, 255, 255, 12); border-top: 1px solid rgba(255, 255, 255, 20); }
QLabel { color: #EAEAEA; font-weight: 500; }
QLabel#LblTokens { color: #FF8C00; font-weight: bold; font-size: 12px; } /* 🔥 暗色模式：使用标志性高亮橙色 */
QPushButton#WinBtnMin, QPushButton#WinBtnClose { background: transparent; border: none; font-size: 14px; color: rgba(255,255,255,150); }
QPushButton#WinBtnMin:hover { background: rgba(255,255,255,20); color: #fff; }
QPushButton#WinBtnClose:hover { background: rgba(255,50,50,200); color: #fff; }

QPushButton#TopBarBtn {
    background: transparent;
    border: none;
    border-radius: 4px;
    color: #EAEAEA;
    font-size: 15px;
    font-family: 'Segoe Fluent Icons', 'Segoe MDL2 Assets', 'Segoe UI Symbol';
    outline: none;
}
QPushButton#TopBarBtn:hover {
    background: rgba(255, 255, 255, 30);
    color: #FFFFFF;
}
QPushButton#TopBarBtn:pressed {
    background: rgba(255, 255, 255, 50);
}
QPushButton#TopBarBtn[debugOn="true"] {
    background: rgba(255, 140, 0, 40);
    color: #FFA500;
}
QFrame#TopBarSep {
    border-left: 1px solid rgba(255, 255, 255, 40);
    margin: 0px;
}

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
QComboBox QAbstractItemView {
    background: rgba(30, 30, 35, 240);
    border: 1px solid rgba(255, 255, 255, 30);
    border-radius: 4px;
    selection-background-color: #FF8C00;
    selection-color: #FFFFFF;
    outline: none;
    font-size: 13px;
    padding: 2px;
}
QComboBox QAbstractItemView::item {
    min-height: 28px;
    padding: 0 8px;
    color: #EAEAEA;
}
QComboBox QAbstractItemView::item:hover {
    background: rgba(255, 140, 0, 50);
}

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

QTextEdit#LogArea { background: transparent; color: #E0E0E0; font-family: '%2'; font-size: 12px; border: none; }
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
        style = QString(R"(
QWidget { font-family: '%1'; font-size: 12px; font-weight: 400; }
QToolTip { color: #222222; background-color: rgba(245, 245, 250, 230); border: 1px solid rgba(0, 0, 0, 30); border-radius: 4px; padding: 4px; }
QWidget#Card { background: rgba(255, 255, 255, 180); border-radius: 8px; border: 1px solid rgba(255, 255, 255, 255); box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
QLabel { color: #222222; font-weight: 500; }
QLabel#LblTokens { color: #9400D3; font-weight: bold; font-size: 12px; } /* 🔥 亮色模式：使用高对比深邃赛博紫 */
QPushButton#WinBtnMin, QPushButton#WinBtnClose { background: transparent; border: none; font-size: 14px; color: #555; }
QPushButton#WinBtnMin:hover { background: rgba(0,0,0,10); }
QPushButton#WinBtnClose:hover { background: rgba(255,50,50,200); color: #fff; }

QPushButton#TopBarBtn {
    background: transparent;
    border: none;
    border-radius: 4px;
    color: #333333;
    font-size: 15px;
    font-family: 'Segoe Fluent Icons', 'Segoe MDL2 Assets', 'Segoe UI Symbol';
    outline: none;
}
QPushButton#TopBarBtn:hover {
    background: rgba(0, 0, 0, 20);
    color: #000000;
}
QPushButton#TopBarBtn:pressed {
    background: rgba(0, 0, 0, 40);
}
QPushButton#TopBarBtn[debugOn="true"] {
    background: rgba(148, 0, 211, 20);
    color: #9400D3;
}
QFrame#TopBarSep {
    border-left: 1px solid rgba(0, 0, 0, 40);
    margin: 0px;
}

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

QTextEdit#LogArea { background: transparent; color: #222; font-family: '%2'; font-size: 12px; border: none; }
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
    
    if (!isRounded)
    {
        style.replace(QRegularExpression("radius:\\s*\\d+px;"), "radius: 0px;");
    }
    return style;
}

// ==========================================
// 🌫️ 全局毛玻璃效果工具函数
// ==========================================
inline QPixmap applyBlurEffect(const QPixmap &source, int blurRadius)
{
    if (source.isNull() || blurRadius <= 0)
    {
        return source;
    }

    static QPixmapCache cache;
    QString key = QString("blur_%1_%2")
                      .arg(source.cacheKey())
                      .arg(blurRadius);

    QPixmap cached;
    if (QPixmapCache::find(key, &cached))
        return cached;

    // 使用QGraphicsBlurEffect实现真正的模糊效果
    QGraphicsBlurEffect *blurEffect = new QGraphicsBlurEffect();
    blurEffect->setBlurRadius(blurRadius);
    blurEffect->setBlurHints(QGraphicsBlurEffect::QualityHint);

    // 创建临时widget来应用模糊效果
    QLabel tempLabel;
    tempLabel.setPixmap(source);
    tempLabel.setGraphicsEffect(blurEffect);

    // 渲染模糊后的图像
    QPixmap result(source.size());
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    tempLabel.render(&painter);

    QPixmapCache::insert(key, result);
    return result;
}

// ==========================================
// 💧 苹果 iOS 风格极致液态毛玻璃效果 (Ultra Liquid Glass)
// ==========================================
inline void drawFrostedGlassEffect(QPainter &painter, const QRect &rect, bool isDark, int alpha, int radius, int hueShift = 0)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    // 计算透明度因子 (0.0 ~ 1.0)
    float a = alpha / 255.0f;

    // ==========================================
    // 💧 1. 极简的基础透色层 (极大降低纯色遮盖，主要靠光影表现)
    // ==========================================
    QColor baseColor;
    if (isDark) {
        // 深色模式：极深的冷灰，低透明度
        baseColor = shiftHue(QColor(25, 25, 30, 255 * (a * 0.4f)), hueShift); 
    } else {
        // 亮色模式：纯白，极低透明度
        baseColor = shiftHue(QColor(255, 255, 255, 255 * (a * 0.5f)), hueShift); 
    }
    painter.setBrush(baseColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, radius, radius);

    // ==========================================
    // 💧 2. 强烈的全局光线漫反射 (iOS 标志性的大面积柔光)
    // ==========================================
    if (alpha > 20) {
        QLinearGradient ambientLight(rect.topLeft(), rect.bottomLeft());
        if (isDark) {
            ambientLight.setColorAt(0.0, QColor(255, 255, 255, 25 * a));
            ambientLight.setColorAt(0.5, QColor(255, 255, 255, 5 * a));
            ambientLight.setColorAt(1.0, QColor(0, 0, 0, 40 * a));
        } else {
            ambientLight.setColorAt(0.0, QColor(255, 255, 255, 160 * a));
            ambientLight.setColorAt(0.5, QColor(255, 255, 255, 60 * a));
            ambientLight.setColorAt(1.0, QColor(255, 255, 255, 10 * a));
        }
        painter.setBrush(ambientLight);
        painter.drawRoundedRect(rect, radius, radius);
    }

    // ==========================================
    // 💧 3. 锋利的对角切割反光 (模拟坚硬的玻璃表面折射)
    // ==========================================
    if (alpha > 40) {
        QLinearGradient gloss(rect.topLeft(), rect.bottomRight());
        if (isDark) {
            gloss.setColorAt(0.0, QColor(255, 255, 255, 30 * a));
            gloss.setColorAt(0.3, QColor(255, 255, 255, 8 * a));
            gloss.setColorAt(0.31, QColor(255, 255, 255, 0)); // 锐利断层
            gloss.setColorAt(1.0, QColor(255, 255, 255, 0));
        } else {
            gloss.setColorAt(0.0, QColor(255, 255, 255, 220 * a));
            gloss.setColorAt(0.4, QColor(255, 255, 255, 80 * a));
            gloss.setColorAt(0.41, QColor(255, 255, 255, 10 * a)); // 锐利断层
            gloss.setColorAt(1.0, QColor(255, 255, 255, 0));
        }
        painter.setBrush(gloss);
        painter.drawRoundedRect(rect, radius, radius);
    }

    // ==========================================
    // 💧 4. 内发光描边 (Rim Light - 玻璃的厚度感)
    // ==========================================
    if (alpha > 50) {
        QLinearGradient rimLight(rect.topLeft(), rect.bottomRight());
        if (isDark) {
            rimLight.setColorAt(0.0, QColor(255, 255, 255, 80 * a));
            rimLight.setColorAt(0.2, QColor(255, 255, 255, 15 * a));
            rimLight.setColorAt(0.8, QColor(255, 255, 255, 5 * a));
            rimLight.setColorAt(1.0, QColor(255, 255, 255, 40 * a)); // 右下角反光
        } else {
            rimLight.setColorAt(0.0, QColor(255, 255, 255, 255 * a));
            rimLight.setColorAt(0.3, QColor(255, 255, 255, 120 * a));
            rimLight.setColorAt(0.7, QColor(255, 255, 255, 50 * a));
            rimLight.setColorAt(1.0, QColor(255, 255, 255, 150 * a)); // 右下角反光
        }
        QPen rimPen(rimLight, 1.5);
        painter.setPen(rimPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), radius > 0 ? radius - 1 : 0, radius > 0 ? radius - 1 : 0);
    }

    // ==========================================
    // 💧 5. 极细的高级外边框 (定义边界轮廓)
    // ==========================================
    QColor borderColor = isDark ? QColor(0, 0, 0, 100 * a) : QColor(0, 0, 0, 25 * a);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect, radius, radius);

    painter.restore();
}

// ==========================================
// ✨ 霓虹发光效果绘制函数 (Neon Glow Effect) - 优雅极光版
// ==========================================
inline void drawLegacyGlowEffect(QPainter &painter, const QRect &rect, bool isDark, int alpha, int radius, float glowFactor, float flowOffset = 0.0f)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 基础底色
    QColor baseColor = isDark ? QColor(25, 25, 30, alpha) : QColor(250, 250, 255, alpha);
    painter.setBrush(baseColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, radius, radius);

    // 2. 优雅的固定对角渐变 + 重心游移呼吸
    // 废弃粗暴的 360 度跑马灯旋转，改为锁定对角线，但让中间的颜色交界点来回平滑推移
    QLinearGradient glowGrad(rect.topLeft(), rect.bottomRight());
    
    // 利用 flowOffset 做一个极度平滑的摇摆波 (Wave)
    float wave = qSin(flowOffset * M_PI * 2.0f);
    float midStop = 0.5f + 0.25f * wave; // 中心颜色在 0.25 到 0.75 之间悠然荡漾，如同呼吸

    if (isDark) {
        // 暗色模式：经典的 青-紫-橙，深邃神秘
        glowGrad.setColorAt(0.0, QColor(0, 229, 255));
        glowGrad.setColorAt(qBound(0.01f, midStop, 0.99f), QColor(156, 39, 176));
        glowGrad.setColorAt(1.0, QColor(255, 140, 0));
    } else {
        // 亮色模式：专属的 晨曦极光 (珊瑚粉 -> 柔和紫 -> 天际蓝)，全息珍珠光泽
        glowGrad.setColorAt(0.0, QColor(255, 110, 120));   // Coral Pink
        glowGrad.setColorAt(qBound(0.01f, midStop, 0.99f), QColor(160, 100, 255)); // Soft Violet
        glowGrad.setColorAt(1.0, QColor(0, 210, 255));     // Sky Cyan
    }

    painter.setBrush(Qt::NoBrush);

    // 3. 柔和适中的外发光
    const int glowLayers = 4;
    for (int i = glowLayers; i >= 1; --i) {
        float op = (1.0f / i) * glowFactor * (isDark ? 0.55f : 0.35f);
        painter.setOpacity(qBound(0.0f, op, 1.0f));
        
        QPen pen(glowGrad, i * 2.0);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), radius, radius);
    }

    // 4. 核心霓虹锐利管线
    painter.setOpacity(qBound(0.0f, 0.95f * glowFactor, 1.0f)); 
    painter.setPen(QPen(glowGrad, 1.2)); 
    painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), radius, radius);

    // 5. 顶部的微弱反光
    painter.setOpacity(1.0);
    QLinearGradient glint(rect.topLeft(), rect.topRight());
    glint.setColorAt(0.0, QColor(255, 255, 255, isDark ? 80 : 150)); 
    glint.setColorAt(0.5, QColor(255, 255, 255, 0));
    glint.setColorAt(1.0, QColor(255, 255, 255, isDark ? 30 : 90));
    painter.setPen(QPen(glint, 1.0));
    painter.drawLine(rect.left() + radius, rect.top() + 1, rect.right() - radius, rect.top() + 1);

    painter.restore();
}

// ==========================================
// 🌫️ 菜单专用毛玻璃效果绘制函数 (增强版：轻量加强毛玻璃效果 + 完整流光渲染)
// ==========================================
inline void drawMenuGlassEffect(QPainter &painter, const QRect &rect, bool isDark, int alpha, int radius, int hueShift = 0, int tintIntensity = 100)
{
    painter.save();

    float a = alpha / 255.0f;
    float tint = qBound(0, tintIntensity, 200) / 100.0f;
    float tintLerp = qMin(1.0f, tint);
    float tintBoost = (tint > 1.0f) ? (1.0f + (tint - 1.0f) * 0.5f) : 1.0f;

    // 🔥 0. 毛玻璃底层起雾层（基础色）
    QColor frostedBase = isDark ? QColor(25, 25, 30, static_cast<int>(alpha * 0.9f)) : QColor(245, 245, 250, static_cast<int>(alpha * 0.85f));
    frostedBase = shiftHue(frostedBase, hueShift);
    painter.setBrush(frostedBase);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, radius, radius);

    // 🔥 1. 强化的流光渐变叠加层 (完整多色流光梯度)
    QLinearGradient bgGrad(rect.topLeft(), rect.bottomRight());
    if (isDark)
    {
        // 深色模式：青-紫-橙 流光梯度，强烈浓度感
        int a0 = qMin(255, static_cast<int>(alpha * 0.8f * tintBoost));
        int a1 = qMin(255, static_cast<int>(alpha * 0.6f * tintBoost));
        int a2 = qMin(255, static_cast<int>(alpha * 0.7f * tintBoost));
        int a3 = qMin(255, static_cast<int>(alpha * 0.85f * tintBoost));
        
        // 基础色与目标色的混合
        auto mix = [&](const QColor &base, const QColor &target, int baseA) {
            int r = base.red() + (target.red() - base.red()) * tintLerp;
            int g = base.green() + (target.green() - base.green()) * tintLerp;
            int b = base.blue() + (target.blue() - base.blue()) * tintLerp;
            return shiftHue(QColor(r, g, b, baseA), hueShift);
        };
        
        bgGrad.setColorAt(0.00, mix(QColor(28, 28, 32), QColor(0, 100, 140), a0));   // 青-冷
        bgGrad.setColorAt(0.33, mix(QColor(32, 28, 40), QColor(80, 30, 120), a1));   // 紫-深
        bgGrad.setColorAt(0.66, mix(QColor(38, 28, 35), QColor(150, 70, 50), a2));   // 橙-暖
        bgGrad.setColorAt(1.00, mix(QColor(20, 20, 25), QColor(100, 50, 80), a3));   // 紫-回
    }
    else
    {
        // 亮色模式：粉-紫-青 流光梯度
        int a0 = qMin(255, static_cast<int>(alpha * 0.75f * tintBoost));
        int a1 = qMin(255, static_cast<int>(alpha * 0.65f * tintBoost));
        int a2 = qMin(255, static_cast<int>(alpha * 0.6f * tintBoost));
        int a3 = qMin(255, static_cast<int>(alpha * 0.75f * tintBoost));
        
        auto mix = [&](const QColor &base, const QColor &target, int baseA) {
            int r = base.red() + (target.red() - base.red()) * tintLerp;
            int g = base.green() + (target.green() - base.green()) * tintLerp;
            int b = base.blue() + (target.blue() - base.blue()) * tintLerp;
            return shiftHue(QColor(r, g, b, baseA), hueShift);
        };
        
        bgGrad.setColorAt(0.00, mix(QColor(255, 240, 245), QColor(255, 180, 220), a0)); // 粉-暖
        bgGrad.setColorAt(0.33, mix(QColor(245, 240, 255), QColor(200, 150, 255), a1)); // 紫-雅
        bgGrad.setColorAt(0.66, mix(QColor(235, 245, 255), QColor(150, 220, 255), a2)); // 青-冷
        bgGrad.setColorAt(1.00, mix(QColor(250, 245, 250), QColor(220, 200, 255), a3)); // 紫-回
    }
    painter.setBrush(bgGrad);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, radius, radius);

    // 🔥 2. 光线漫反射与锋利对角反光
    if (alpha > 20)
    {
        QLinearGradient softLight(rect.topLeft(), rect.bottomLeft());
        if (isDark)
        {
            softLight.setColorAt(0.0, QColor(255, 255, 255, 20 * a));
            softLight.setColorAt(0.4, QColor(255, 255, 255, 5 * a));
            softLight.setColorAt(1.0, QColor(255, 255, 255, 0));
        }
        else
        {
            softLight.setColorAt(0.0, QColor(255, 255, 255, 90 * a));
            softLight.setColorAt(0.5, QColor(255, 255, 255, 30 * a));
            softLight.setColorAt(1.0, QColor(255, 255, 255, 0));
        }
        painter.setBrush(softLight);
        painter.drawRoundedRect(rect, radius, radius);
    }

    if (alpha > 50)
    {
        QPainterPath path;
        path.addRoundedRect(rect, radius, radius);
        QPainterPath innerPath;
        innerPath.addRoundedRect(rect.adjusted(1, 1, -1, -1), radius > 0 ? radius - 1 : 0, radius > 0 ? radius - 1 : 0);
        path = path.subtracted(innerPath);

        QLinearGradient rimGrad(rect.topLeft(), rect.bottomRight());
        if (isDark)
        {
            rimGrad.setColorAt(0.0, QColor(255, 255, 255, 60 * a));
            rimGrad.setColorAt(0.3, QColor(255, 255, 255, 5 * a));
            rimGrad.setColorAt(0.7, QColor(255, 255, 255, 0));
            rimGrad.setColorAt(1.0, QColor(255, 255, 255, 20 * a));
        }
        else
        {
            rimGrad.setColorAt(0.0, QColor(255, 255, 255, 180 * a));
            rimGrad.setColorAt(0.3, QColor(255, 255, 255, 40 * a));
            rimGrad.setColorAt(0.7, QColor(255, 255, 255, 0));
            rimGrad.setColorAt(1.0, QColor(255, 255, 255, 60 * a));
        }
        painter.fillPath(path, rimGrad);
    }

    // 🔥 3. 极细高级外边框
    QColor borderColor = isDark ? QColor(0, 0, 0, 100 * a) : QColor(0, 0, 0, 25 * a);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect, radius, radius);

    painter.restore();
}

// ==========================================
// 🚨 GlassMessageBox: 纯净透明噪点提示框
// ==========================================
class GlassMessageBox : public QDialog
{
public:
    static void warning(QWidget *parent, const QString &title, const QString &text, bool isDark, int alpha, bool isRounded, GlassRenderMode mode = GlassRenderMode::Frosted, int hueShift = 0, int tintIntensity = 100)
    {
        GlassMessageBox msg(parent, title, text, isDark, alpha, isRounded, mode, hueShift, tintIntensity);
        msg.exec();
    }

private:
    GlassMessageBox(QWidget *parent, const QString &title, const QString &text, bool isDark, int alpha, bool isRounded, GlassRenderMode mode, int hueShift, int tintIntensity)
        : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog), m_isDark(isDark), m_isRounded(isRounded), m_alpha(alpha), m_hueShift(hueShift), m_tintIntensity(tintIntensity), m_mode(mode)
    {
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

        if (m_mode == GlassRenderMode::Frosted)
        {
            // 🌫️ 使用与主窗一致的菜单毛玻璃材质，确保子窗口观感完全同步
            drawMenuGlassEffect(p, r, m_isDark, m_alpha, rds, m_hueShift, m_tintIntensity);
            
            // 绘制边框
            p.setPen(QPen(QColor(255, 140, 0, 150), 2));
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
        }
        else
        {
            // ✨ 旧版原汁霓虹厚玻璃
            drawLegacyGlowEffect(p, r, m_isDark, m_alpha, rds, 1.0f);
        }
    }

private:
    bool m_isDark, m_isRounded;
    int m_alpha;
    int m_hueShift;
    int m_tintIntensity;
    GlassRenderMode m_mode;
};

// ==========================================
// ✨ GlossaryHighlighter & Drawer & Menus
// ==========================================
class GlossaryHighlighter : public QSyntaxHighlighter
{
public:
    GlossaryHighlighter(QTextDocument *parent, bool isDark) : QSyntaxHighlighter(parent) { setTheme(isDark); }
    void setTheme(bool isDark, GlassRenderMode renderMode = GlassRenderMode::Frosted)
    {
        const bool frostedMode = (renderMode == GlassRenderMode::Frosted);

        if (frostedMode)
        {
            eqFormat.setForeground(QColor(isDark ? "#8be9fd" : "#0ea5e9"));
            keyFormat.setForeground(QColor(isDark ? "#ffd166" : "#4c1d95"));
            valFormat.setForeground(QColor(isDark ? "#f8fafc" : "#111827"));
            commentFormat.setForeground(QColor(isDark ? "#cbd5e1" : "#6b7280"));
        }
        else
        {
            eqFormat.setForeground(QColor(isDark ? "#00e5ff" : "#9400D3"));
            keyFormat.setForeground(QColor(isDark ? "#FF8C00" : "#B8860B"));
            valFormat.setForeground(QColor(isDark ? "#ffffff" : "#111111"));
            commentFormat.setForeground(QColor(isDark ? "#888888" : "#999999"));
        }

        eqFormat.setFontWeight(QFont::Bold);
        keyFormat.setFontWeight(QFont::DemiBold);
        valFormat.setFontWeight(QFont::DemiBold);
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
        if (commentIdx == 0)
        {
            setFormat(0, text.length(), commentFormat);
            return;
        }
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

class GlossaryDrawer : public QWidget
{
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

        QRect parentRect = m_parent->geometry();
        QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();

        int w = 320;
        int h = parentRect.height() * 0.85;

        // 🔥 修正：优化弹出位置计算
        // 垂直位置：与父窗口顶部对齐，而不是居中
        int y = parentRect.top() + 10;

        // 水平位置：优先从右侧弹出
        int x = parentRect.right() + 5;

        // 检查右侧是否有足够空间
        if (x + w > screenRect.right())
        {
            // 右侧空间不够，尝试从左侧弹出
            x = parentRect.left() - w - 5;

            // 如果左侧也不够，则紧贴屏幕右侧
            if (x < screenRect.left())
            {
                x = screenRect.right() - w - 5;
            }
        }

        // 确保垂直位置不超出屏幕
        if (y + h > screenRect.bottom())
        {
            y = screenRect.bottom() - h - 5;
        }
        if (y < screenRect.top())
        {
            y = screenRect.top() + 5;
        }

        m_finalRect = QRect(x, y, w, h);

        // 起始位置：从父窗口边缘开始
        if (x > parentRect.x())
        {
            m_startRect = QRect(parentRect.right(), y, 0, h);
        }
        else
        {
            m_startRect = QRect(parentRect.left(), y, 0, h);
        }
        setGeometry(m_startRect);
        setWindowOpacity(0.0);

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

        // 轻度动态流光：只做低频位移，避免影响阅读
        m_shimmerTimer = new QTimer(this);
        m_shimmerTimer->setInterval(48);
        connect(m_shimmerTimer, &QTimer::timeout, this, [this]() {
            if (m_renderMode == GlassRenderMode::Frosted)
            {
                m_shimmerPhase = (m_shimmerPhase + 3) % 360;
                update();
            }
        });
        m_shimmerTimer->start();
    }
    void updateEnv(bool isDark, int alpha, bool isRounded)
    {
        m_isDark = isDark;
        m_alpha = alpha;
        m_isRounded = isRounded;
        const bool frostedMode = (m_renderMode == GlassRenderMode::Frosted);

        m_lblTitle->setStyleSheet(QString("font-size: 14px; font-weight: bold; color: %1;")
            .arg(frostedMode ? (isDark ? "#fbbf24" : "#6d28d9") : (isDark ? "#FF8C00" : "#9400D3")));

        QString textColor = frostedMode
            ? (isDark ? "#f8fafc" : "#111827")
            : (isDark ? "#ffffff" : "#000000");
        QString scrollHandle = isDark ? "rgba(255, 255, 255, 50)" : "rgba(0, 0, 0, 50)";
        QString scrollHover = isDark ? "#FF8C00" : "#9400D3";
        QString editorStyle = QString(R"(QTextEdit { font-family: "Consolas", "Microsoft YaHei"; font-size: %4px; background: %5; border: 1px solid rgba(128,128,128,50); border-radius: 6px; padding: 5px; color: %1; } QScrollBar:vertical { border: none; background: transparent; width: 6px; margin: 0px; } QScrollBar::handle:vertical { background: %2; min-height: 20px; border-radius: 3px; } QScrollBar::handle:vertical:hover { background: %3; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; } QScrollBar:horizontal { height: 0px; background: transparent; })")
            .arg(textColor,
                 scrollHandle,
                 scrollHover,
                 frostedMode ? (isDark ? "12" : "12") : "13",
                 frostedMode ? (isDark ? "rgba(18, 24, 38, 0.10)" : "rgba(255, 255, 255, 0.20)") : "transparent");
        QString btnStyle = R"(QPushButton { background: rgba(128, 128, 128, 30); border: 1px solid rgba(128,128,128,50); border-radius: 6px; color: %1; padding: 6px; } QPushButton:hover { background: %2; color: #fff; border-color: %2; })";
        if (!isRounded)
        {
            editorStyle.replace(QRegularExpression("radius:\\s*\\d+px;"), "radius: 0px;");
            btnStyle.replace(QRegularExpression("radius:\\s*\\d+px;"), "radius: 0px;");
        }
        m_editor->setStyleSheet(editorStyle);
        m_btnSave->setStyleSheet(btnStyle.arg(textColor, isDark ? "rgba(255, 140, 0, 80)" : "rgba(148, 0, 211, 80)"));
        m_btnClose->setStyleSheet(btnStyle.arg(textColor, isDark ? "rgba(255, 50, 50, 80)" : "rgba(255, 50, 50, 80)"));
        m_highlighter->setTheme(isDark, m_renderMode);
        update();
    }

    void setRenderMode(GlassRenderMode mode)
    {
        if (m_renderMode != mode)
        {
            m_renderMode = mode;
            m_highlighter->setTheme(m_isDark, m_renderMode);
            update();
        }
    }

    void setHueShift(int hueOffset)
    {
        if (m_hueShift != hueOffset)
        {
            m_hueShift = hueOffset;
            update();
        }
    }

    void setTintIntensity(int intensity)
    {
        int newIntensity = qBound(0, intensity, 200);
        if (m_tintIntensity != newIntensity)
        {
            m_tintIntensity = newIntensity;
            update();
        }
    }

    void updateLanguage(int lang)
    {
        m_lang = lang;
        m_lblTitle->setText(lang == 1 ? "术语表编辑" : "Glossary Editor");
        m_btnSave->setText(lang == 1 ? "💾 保存并应用" : "💾 Save & Apply");
        m_btnClose->setText(lang == 1 ? "❌ 放弃" : "❌ Cancel");
    }

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

        if (m_renderMode == GlassRenderMode::Frosted)
        {
            // 🌫️ 使用菜单专用毛玻璃效果
            drawMenuGlassEffect(p, r, m_isDark, m_alpha, rds, m_hueShift, m_tintIntensity);

            // ✨ 轻度动态流光层（低透明，不干扰文字）
            const int flowOffset = (m_shimmerPhase % qMax(1, r.width()));
            QLinearGradient shimmer(QPoint(flowOffset - r.width(), 0), QPoint(flowOffset, r.height()));
            QColor shimmerA = shiftHue(m_isDark ? QColor(255, 210, 120, 18) : QColor(120, 170, 255, 16), m_hueShift + m_shimmerPhase / 3);
            QColor shimmerB = shiftHue(m_isDark ? QColor(120, 220, 255, 12) : QColor(200, 120, 255, 12), m_hueShift - m_shimmerPhase / 4);
            shimmer.setColorAt(0.00, Qt::transparent);
            shimmer.setColorAt(0.30, shimmerA);
            shimmer.setColorAt(0.65, shimmerB);
            shimmer.setColorAt(1.00, Qt::transparent);
            p.setPen(Qt::NoPen);
            p.setBrush(shimmer);
            p.drawRoundedRect(r.adjusted(2, 2, -2, -2), qMax(0, rds - 1), qMax(0, rds - 1));
            
            // 绘制单纯边框
            p.setPen(QPen(m_isDark ? QColor(255, 140, 0, 120) : QColor(148, 0, 211, 120), 1));
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
        }
        else
        {
            // ✨ 旧版原汁原味的霓虹厚玻璃效果！
            drawLegacyGlowEffect(p, r, m_isDark, m_alpha, rds, 1.0f);
        }
    }
    void mousePressEvent(QMouseEvent *e) override
    {
        // 🔥 修复：扩大拖拽区域，让整个标题栏区域（前60像素）都可以拖拽
        if (e->button() == Qt::LeftButton && e->pos().y() <= 60)
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
        QString orig = m_btnSave->styleSheet();
        m_btnSave->setText(m_lang == 1 ? "✅ 保存成功" : "✅ Saved");
        m_btnSave->setStyleSheet(QString("QPushButton { background: rgba(56, 239, 125, 80); border: 1px solid #38ef7d; border-radius: %1px; color: #fff; padding: 6px; font-weight: bold; }").arg(m_isRounded ? 6 : 0));
        QTimer::singleShot(1500, this, [this, orig]()
                           { if(this && m_btnSave){ updateLanguage(m_lang); m_btnSave->setStyleSheet(orig); } });
    }

public:
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
    int m_hueShift = 0; // 🌈 颜色偏移
    int m_tintIntensity = 100; // ✨ 流光浓度
    int m_shimmerPhase = 0;
    TranslationServer *m_server;
    GlassRenderMode m_renderMode = GlassRenderMode::Frosted; // 🎨 渲染模式
    QTimer *m_shimmerTimer = nullptr;
    QLabel *m_lblTitle;
    QTextEdit *m_editor;
    QPushButton *m_btnSave, *m_btnClose;
    GlossaryHighlighter *m_highlighter;
    QRect m_finalRect, m_startRect;
    QPoint m_dragPos;
    bool m_isClosing = false, m_isDrawerDragging = false;
};

// 🔥 CAN FIX: 废除 palette 推断，直接传入 isDark 变量，精准注入主题色！
class GlassDelegate : public QStyledItemDelegate
{
public:
    explicit GlassDelegate(bool isDark, QObject *parent = nullptr) : QStyledItemDelegate(parent), m_isDark(isDark) {}

    void setRenderMode(GlassRenderMode mode)
    {
        m_renderMode = mode;
    }

    void setIsDark(bool isDark)
    {
        m_isDark = isDark;
    }

    void setHueShift(int shift)
    {
        m_hueShift = shift;
    }

    void setTintIntensity(int intensity)
    {
        m_tintIntensity = intensity;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        bool isSelected = option.state & QStyle::State_Selected;
        bool isHover = option.state & QStyle::State_MouseOver;

        // 彻底抛弃 option.palette.window().color().lightness()，使用真实的模式状态
        bool isDark = m_isDark;

        QRect drawRect = option.rect.adjusted(4, 2, -4, -2);
        QColor mainColor, subColor;

        if (isDark)
        {
            mainColor = isSelected ? QColor("#FFFFFF") : QColor("#EAEAEA");
            subColor = QColor("#AAAAAA");
        }
        else
        {
            mainColor = isSelected ? QColor("#000000") : QColor("#222222");
            subColor = QColor("#666666");
        }

        if (isSelected || isHover)
        {
            if (m_renderMode == GlassRenderMode::Frosted)
            {
                const QRect outerRect = drawRect;
                const QRect innerRect = outerRect.adjusted(1, 1, -1, -1);
                qreal mixRatio = qBound(0.0, m_tintIntensity / 100.0, 1.0);

                if (mixRatio < 1.0)
                {
                    QColor bgBrush = isDark
                                         ? (isSelected ? QColor(255, 255, 255, 45) : QColor(255, 255, 255, 20))
                                         : (isSelected ? QColor(0, 0, 0, 40) : QColor(0, 0, 0, 18));
                    QColor bgPen = isDark 
                                         ? (isSelected ? QColor(255, 255, 255, 80) : QColor(255, 255, 255, 30))
                                         : (isSelected ? QColor(0, 0, 0, 70) : QColor(0, 0, 0, 25));
                    
                    bgBrush.setAlphaF(bgBrush.alphaF() * (1.0 - mixRatio));
                    bgPen.setAlphaF(bgPen.alphaF() * (1.0 - mixRatio));

                    painter->save();
                    painter->setBrush(bgBrush);
                    painter->setPen(bgPen);
                    painter->drawRoundedRect(drawRect, 5, 5);
                    painter->restore();
                }

                if (mixRatio > 0.0)
                {
                    painter->save();
                    painter->setOpacity(mixRatio);

                    QColor baseA = isDark ? QColor(26, 28, 36, isSelected ? 180 : 100) : QColor(245, 245, 250, isSelected ? 220 : 160);
                    QColor baseB = isDark ? QColor(46, 50, 66, isSelected ? 150 : 80) : QColor(230, 235, 245, isSelected ? 180 : 120);
                    QColor baseC = isDark ? QColor(0, 240, 255, isSelected ? 60 : 25) : QColor(168, 20, 231, isSelected ? 40 : 22);

                    baseA = shiftHue(baseA, m_hueShift);
                    baseB = shiftHue(baseB, m_hueShift);
                    baseC = shiftHue(baseC, m_hueShift);

                    QLinearGradient fillGrad(outerRect.topLeft(), outerRect.bottomRight());
                    fillGrad.setColorAt(0.0, baseA);
                    fillGrad.setColorAt(0.55, baseB);
                    fillGrad.setColorAt(1.0, baseC);

                    painter->setPen(Qt::NoPen);
                    painter->setBrush(fillGrad);
                    painter->drawRoundedRect(outerRect, 6, 6);

                    QPainterPath clipPath;
                    clipPath.addRoundedRect(QRectF(outerRect), 6, 6);
                    painter->save();
                    painter->setClipPath(clipPath);

                    QLinearGradient innerGlow(outerRect.topLeft(), outerRect.bottomRight());
                    if (isDark)
                    {
                        innerGlow.setColorAt(0.0, shiftHue(QColor(0, 255, 250, isSelected ? 200 : 100), m_hueShift));
                        innerGlow.setColorAt(0.5, shiftHue(QColor(176, 59, 196, isSelected ? 180 : 80), m_hueShift));
                        innerGlow.setColorAt(1.0, shiftHue(QColor(255, 160, 20, isSelected ? 190 : 90), m_hueShift));
                    }
                    else
                    {
                        innerGlow.setColorAt(0.0, shiftHue(QColor(255, 130, 140, isSelected ? 200 : 100), m_hueShift));
                        innerGlow.setColorAt(0.5, shiftHue(QColor(180, 120, 255, isSelected ? 190 : 90), m_hueShift));
                        innerGlow.setColorAt(1.0, shiftHue(QColor(20, 230, 255, isSelected ? 170 : 80), m_hueShift));
                    }

                    painter->setOpacity(mixRatio * (isSelected ? 0.95 : 0.55));
                    painter->setBrush(Qt::NoBrush);
                    painter->setPen(QPen(QBrush(innerGlow), isSelected ? 3.0 : 1.5));
                    painter->drawRoundedRect(innerRect.adjusted(1, 1, -1, -1), 5, 5);

                    painter->setOpacity(mixRatio * (isSelected ? 0.50 : 0.20));
                    painter->setPen(QPen(QBrush(innerGlow), isSelected ? 1.5 : 1.0));
                    painter->drawRoundedRect(innerRect.adjusted(3, 3, -3, -3), 4, 4);
                    painter->restore();

                    painter->restore();
                }
            }
            else
            {
                QColor bgBrush = isDark
                                     ? (isSelected ? QColor(255, 255, 255, 45) : QColor(255, 255, 255, 20))
                                     : (isSelected ? QColor(0, 0, 0, 40) : QColor(0, 0, 0, 18));
                QColor bgPen = isDark 
                                     ? (isSelected ? QColor(255, 255, 255, 80) : QColor(255, 255, 255, 30))
                                     : (isSelected ? QColor(0, 0, 0, 70) : QColor(0, 0, 0, 25));
                painter->setBrush(bgBrush);
                painter->setPen(bgPen);
                painter->drawRoundedRect(drawRect, 5, 5);
            }
        }

        QColor accentColor = isDark ? shiftHue(QColor("#FFB000"), m_hueShift) : shiftHue(QColor("#9400D3"), m_hueShift);

        // 🔥 左侧高亮纵柱
        if (isSelected)
        {
            painter->setBrush(accentColor);
            painter->setPen(Qt::NoPen);
            QRect indicator(drawRect.left(), drawRect.top() + 8, 3, drawRect.height() - 16);
            painter->drawRoundedRect(indicator, 1, 1);
        }

        QStringList parts = index.data(Qt::DisplayRole).toString().split('\n');
        QString mainText = parts[0];
        QString subText = (parts.size() > 1) ? parts[1] : "";
        
        // 微调内部文字绘制的内边距，避开左侧柱子和右侧可能有的图标
        QRect textRect = drawRect.adjusted(12, 0, -28, 0);
        QRect mainRect = subText.isEmpty() ? textRect : textRect.adjusted(0, 2, 0, -textRect.height() / 2);

        QFont mainFont = option.font;
        mainFont.setWeight(QFont::Medium);
        mainFont.setPointSize(mainFont.pointSize() + (subText.isEmpty() ? 0 : 1));
        painter->setFont(mainFont);
        QString elidedMain = QFontMetrics(mainFont).elidedText(mainText, Qt::ElideRight, mainRect.width());
        painter->setPen(mainColor);
        painter->drawText(mainRect, Qt::AlignVCenter | Qt::AlignLeft, elidedMain);

        if (!subText.isEmpty())
        {
            QRect subRect = textRect.adjusted(0, textRect.height() / 2 - 2, 0, -2);
            QFont subFont = option.font;
            subFont.setPointSize(option.font.pointSize() - 1);
            painter->setFont(subFont);
            QString elidedSub = QFontMetrics(subFont).elidedText(subText, Qt::ElideRight, subRect.width());
            painter->setPen(subColor);
            painter->drawText(subRect, Qt::AlignVCenter | Qt::AlignLeft, elidedSub);
        }

        painter->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override { return QSize(0, index.data(Qt::DisplayRole).toString().contains('\n') ? 50 : 32); }

private:
    bool m_isDark;
    GlassRenderMode m_renderMode = GlassRenderMode::Frosted;
    int m_hueShift = 0;
    int m_tintIntensity = 100;
};

class GlassMenu : public QWidget
{
    Q_OBJECT
public:
    explicit GlassMenu(QWidget *combo, QWidget *parent, bool isDark, int alpha, bool isRounded) : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), m_combo(combo), m_isDark(isDark), m_alpha(alpha), m_isRounded(isRounded), m_hueShift(0)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        qApp->installEventFilter(this);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetNoConstraint); // 🔥 必须：取消布局尺寸约束，允许父窗口以0高度裁剪子控件，实现无卡顿的从上撕开动画！
        layout->setContentsMargins(4, 4, 6, 4);
        m_list = new QListWidget(this);

        // 🔥 在这里将准确的主题模式 (m_isDark) 传递给委托！
        m_delegate = new GlassDelegate(m_isDark, this);
        m_delegate->setRenderMode(m_renderMode);
        m_delegate->setHueShift(m_hueShift);
        m_delegate->setTintIntensity(m_tintIntensity);
        m_list->setItemDelegate(m_delegate);

        m_list->setFrameShape(QFrame::NoFrame);
        m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QString scrollHandle = isDark ? "rgba(0, 255, 230, 50)" : "rgba(0, 0, 0, 30)";
        QString scrollHover = isDark ? "#FF5E00" : "#9400D3";
        m_list->setStyleSheet(QString(R"(QListWidget { background: transparent; outline: none; border: none; padding: 4px; } QScrollBar:vertical { border: none; background: transparent; width: 4px; margin: 0; } QScrollBar::handle:vertical { background: %1; min-height: 20px; border-radius: 2px; } QScrollBar::handle:vertical:hover { background: %2; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; })").arg(scrollHandle, scrollHover));
        layout->addWidget(m_list);
    }

    void setRenderMode(GlassRenderMode mode) {
        m_renderMode = mode;
        if (m_delegate)
            m_delegate->setRenderMode(mode);
        update();
    }

    void setEnv(bool isDark, int alpha, bool isRounded) {
        m_isDark = isDark;
        m_alpha = alpha;
        m_isRounded = isRounded;
        if (m_delegate)
            m_delegate->setIsDark(isDark);
        m_bgCache = QPixmap();
        update();
    }
    
    void setHueShift(int hueOffset) {
        m_hueShift = hueOffset;
        if (m_delegate)
            m_delegate->setHueShift(hueOffset);
        m_bgCache = QPixmap();
        update();
    }

    void setTintIntensity(int intensity) {
        m_tintIntensity = qBound(0, intensity, 200);
        if (m_delegate)
            m_delegate->setTintIntensity(m_tintIntensity);
        m_bgCache = QPixmap();
        update();
    }
    
    // 🔥 缓存方法：预先绘制菜单全貌，解决动画期间因为高度变化引发的大量复杂计算和高频卡顿！
    void prepareCache(const QSize &finalSize)
    {
        m_bgCache = QPixmap(finalSize);
        m_bgCache.fill(Qt::transparent);
        QPainter p(&m_bgCache);
        p.setRenderHint(QPainter::Antialiasing);
        QRect r(0, 0, finalSize.width(), finalSize.height());
        int rds = m_isRounded ? 8 : 0;

        if (m_renderMode == GlassRenderMode::Frosted)
        {
            drawMenuGlassEffect(p, r, m_isDark, m_alpha, rds, m_hueShift, m_tintIntensity);
        }
       else
        {
            // ✨ 旧版发光模式下，下拉菜单不需要厚重的泛光感染，维持克制
            QColor baseColor = m_isDark ? QColor(25, 25, 30, m_alpha) : QColor(250, 250, 255, m_alpha);
            p.setBrush(baseColor);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(r, rds, rds);
            
            // 极其克制的极简中性边框，消除紫边污染
            QColor borderColor = m_isDark ? QColor(255, 255, 255, 30) : QColor(0, 0, 0, 40);
            p.setPen(QPen(borderColor, 1));
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
        }
    }

    ~GlassMenu() { qApp->removeEventFilter(this); }
    QListWidget *listWidget() const { return m_list; }
    void setStartRect(const QRect &rect) { m_startRect = rect; }
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
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            
            // 1. 如果点击在菜单内部，允许正常处理（选中列表项）
            if (this->geometry().contains(globalPos))
                return false;
            
            // 2. 🔥 核心修复 1：如果点击了触发这个菜单的下拉框自身
            // 无论是否正在关闭，必须【吞噬(swallow)】该点击事件！
            // 否则底层的 QComboBox 会因为收到点击而再次触发打开操作
            if (m_combo && m_combo->rect().contains(m_combo->mapFromGlobal(globalPos)))
            {
                if (!m_isClosing) animateClose();
                return true; // 拦截并吞噬，解决下拉框二次弹出的问题
            }
            
            // 3. 🔥 核心修复 2：点击了应用程序的其他地方（例如"取消"、"保存"按钮）
            // 触发菜单关闭，但【绝对不要吞噬】事件，让正常的按钮点击能够生效！
            if (!m_isClosing) animateClose();
            return false; // 不拦截，放行！解决预设窗口按钮失去控制权的问题
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

        // 🔥 使用缓存图：只有当缓存存在时，动画期间不再进行任何复杂的高额计算
        if (!m_bgCache.isNull())
        {
            p.drawPixmap(0, 0, m_bgCache);
            return;
        }

        QRect r = rect();
        int rds = m_isRounded ? 8 : 0;

         if (m_renderMode == GlassRenderMode::Frosted)
        {
            // 🌫️ 使用菜单专用毛玻璃效果 (简洁版：无渐变，统一色调)
            drawMenuGlassEffect(p, r, m_isDark, m_alpha, rds, m_hueShift, m_tintIntensity);
            
            // 绘制单纯边框
            p.setPen(QPen(QColor(255, 255, 255, m_isDark ? 30 : 100), 1));
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
        }
       else
        {
            // ✨ 旧版发光模式下的下拉菜单实时绘制，剥离厚重的泛光
            QColor baseColor = m_isDark ? QColor(25, 25, 30, m_alpha) : QColor(250, 250, 255, m_alpha);
            p.setBrush(baseColor);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(r, rds, rds);
            
            // 极其克制的极简中性边框，消除紫边污染
            QColor borderColor = m_isDark ? QColor(255, 255, 255, 30) : QColor(0, 0, 0, 40);
            p.setPen(QPen(borderColor, 1));
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), rds, rds);
        }
    }

private:
    QWidget *m_combo;
    QListWidget *m_list;
    GlassDelegate *m_delegate = nullptr;
    bool m_isDark, m_isRounded, m_isClosing = false;
    int m_alpha;
    int m_hueShift;
    int m_tintIntensity = 100;
    GlassRenderMode m_renderMode = GlassRenderMode::Frosted;
    QRect m_startRect;
    QPixmap m_bgCache;
};

class SideGlassCombo : public QComboBox
{
    Q_OBJECT
public:
    explicit SideGlassCombo(QWidget *parent = nullptr) : QComboBox(parent) {}
    
    void setRenderMode(GlassRenderMode mode) {
        m_renderMode = mode;
        if (m_activeMenu) {
            m_activeMenu->setRenderMode(m_renderMode);
        }
    }
    void setHueShift(int shift) {
        m_hueShift = shift;
        if (m_activeMenu) {
            m_activeMenu->setHueShift(m_hueShift);
        }
    }
    void setTintIntensity(int intensity) {
        m_tintIntensity = qBound(0, intensity, 200);
        if (m_activeMenu) {
            m_activeMenu->setTintIntensity(m_tintIntensity);
        }
    }
    
    void setEnv(bool isDark, int alpha, bool isRounded)
    {
        m_isDark = isDark;
        m_alpha = alpha;
        m_isRounded = isRounded;
        if (m_activeMenu) {
            m_activeMenu->setEnv(m_isDark, m_alpha, m_isRounded);
        }
    }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() != Qt::LeftButton)
        {
            QComboBox::mousePressEvent(e);
            return;
        }
        
        if (m_activeMenu)
        {
            m_activeMenu->animateClose();
            e->accept();
            return;
        }
        showPopup();
        e->accept();
    }
    void showPopup() override
    {
        if (m_activeMenu)
            return;
        
        // 找到顶级窗口作为父级，以解决即使在 Modal QDialog 也能正常点击下拉菜单的问题
        QWidget *topLevel = this;
        while (topLevel->parentWidget())
            topLevel = topLevel->parentWidget();
            
        GlassMenu *menu = new GlassMenu(this, topLevel, m_isDark, m_alpha, m_isRounded);
        menu->setRenderMode(m_renderMode);
        menu->setHueShift(m_hueShift);
        menu->setTintIntensity(m_tintIntensity);
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
        for (int i = 0; i < count(); ++i)
        {
            QString url = itemText(i);
            QString tip = itemData(i, Qt::ToolTipRole).toString();
            QString display = tip.isEmpty() ? url : QString("%1\n%2").arg(tip, url);
            QListWidgetItem *item = new QListWidgetItem(display, list);
            item->setSizeHint(QSize(0, tip.isEmpty() ? 32 : 50));
            if (i == currentIndex())
                list->setCurrentRow(i);
            int currentItemWidth = tip.isEmpty() ? fmBold.horizontalAdvance(url) : qMax(fmBold.horizontalAdvance(tip), fm.horizontalAdvance(url));
            if (currentItemWidth > maxContentWidth)
                maxContentWidth = currentItemWidth;
        }
        int totalH = 0;
        for (int i = 0; i < list->count(); ++i)
            totalH += list->item(i)->sizeHint().height();
        int menuH = qMin(totalH + 20, 600);
        QPoint globalPos = mapToGlobal(QPoint(0, 0));
        int finalX = globalPos.x();
        int finalY = globalPos.y() + height();

        // 🔥 下拉菜单：宽度与父容器一致
        int menuW = width();

        // 检查是否超出屏幕右边界
        QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
        if (finalX + menuW > screenRect.right())
        {
            finalX = screenRect.right() - menuW;
        }
        if (finalX < screenRect.left())
        {
            finalX = screenRect.left();
        }

        QRect finalRect(finalX, finalY, menuW, menuH);

        // 🔥 防止动画期间QListWidget不断触发 Relayout 导致撕裂和突然弹出
        list->setFixedSize(menuW - 10, menuH - 8);
        list->scrollToItem(list->item(currentIndex()), QAbstractItemView::PositionAtTop);

        // 🔥 下拉动画：起始位置高度为0，从上方滑下
        QRect startRect = finalRect;
        startRect.setHeight(0);

        // 🔥 调用缓存，防止动画期间的重绘撕裂和卡顿
        menu->prepareCache(finalRect.size());

        menu->setStartRect(startRect);
        menu->setGeometry(startRect);
        
        // Pass hue shift down if menu supports it? Wait, GlassMenu has setHueShift?
        // Let's add setHueShift to GlassMenu.
        
        menu->show();
        QParallelAnimationGroup *enterGroup = new QParallelAnimationGroup(menu);
        QPropertyAnimation *geoEnter = new QPropertyAnimation(menu, "geometry");
        geoEnter->setStartValue(startRect);
        geoEnter->setEndValue(finalRect);
        geoEnter->setDuration(350);
        geoEnter->setEasingCurve(QEasingCurve::OutCubic);
        QPropertyAnimation *fadeEnter = new QPropertyAnimation(menu, "windowOpacity");
        fadeEnter->setStartValue(0.0);
        fadeEnter->setEndValue(1.0);
        fadeEnter->setDuration(300);
        enterGroup->addAnimation(geoEnter);
        enterGroup->addAnimation(fadeEnter);
        enterGroup->start(QAbstractAnimation::DeleteWhenStopped);
        connect(list, &QListWidget::itemClicked, [this, menu, list](QListWidgetItem *item)
                { 
                    int row = list->row(item);
                    this->setCurrentIndex(row); 
                    menu->animateClose(); 
                    QTimer::singleShot(0, this, [this, row]() {
                        emit this->activated(row);
                    });
                });
    }

private:
    bool m_isDark = true, m_isRounded = true;
    int m_alpha = 200;
    int m_hueShift = 0; // 🌈 颜色偏移
    int m_tintIntensity = 100; // ✨ 流光浓度
    GlassRenderMode m_renderMode = GlassRenderMode::Frosted;
    QPointer<GlassMenu> m_activeMenu;
};

class GlassCard : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float flowOffset READ flowOffset WRITE setFlowOffset)
    Q_PROPERTY(float glowFactor READ glowFactor WRITE setGlowFactor)
public:
    explicit GlassCard(bool isDark, QWidget *parent = nullptr) : QWidget(parent), m_isDark(isDark)
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setObjectName("Card");

        // 新版流动动画
        m_flowAnim = new QPropertyAnimation(this, "flowOffset", this);
        m_flowAnim->setDuration(5000);
        m_flowAnim->setStartValue(0.0f);
        m_flowAnim->setEndValue(1.0f);
        m_flowAnim->setEasingCurve(QEasingCurve::InOutSine);
        m_flowAnim->setLoopCount(-1);
        m_flowAnim->start();

        // 旧版呼吸脉冲动画
        m_pulseAnim = new QPropertyAnimation(this, "glowFactor", this);
        m_pulseAnim->setDuration(6000);
        m_pulseAnim->setStartValue(0.9f);
        m_pulseAnim->setEndValue(1.35f);
        m_pulseAnim->setEasingCurve(QEasingCurve::InOutSine);
        m_pulseAnim->setLoopCount(-1);
        m_pulseAnim->start();
    }

    // 🎨 设置渲染模式
    void setRenderMode(GlassRenderMode mode)
    {
        m_renderMode = mode;
        update();
    }
    
    void setHueShift(int hueOffset)
    {
        if (m_hueShift != hueOffset)
        {
            m_hueShift = hueOffset;
            update();
        }
    }

    GlassRenderMode getRenderMode() const { return m_renderMode; }
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
    void setAlpha(int alpha)
    {
        if (m_alpha != alpha)
        {
            m_alpha = alpha;
            update();
        }
    }
    float flowOffset() const { return m_flowOffset; }
    void setFlowOffset(float f)
    {
        m_flowOffset = f;
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

        // 🎨 严格分支：只有在这里才是原生毛玻璃 + 岩浆红浮光
        if (m_renderMode == GlassRenderMode::Frosted)
        {
            // 🌫️ 1. 底层原生毛玻璃 (保持你的全局函数不变)
            drawFrostedGlassEffect(p, r, m_isDark, m_alpha, radius, m_hueShift);

            // ==========================================
            // 🌌 2. 纯粹局部的“游走呼吸”浮光层 (不再是线性扫描)
            // ==========================================
            p.save();
            p.setRenderHint(QPainter::Antialiasing);

            // 🔥 游走算法：使用数学波形让光点在空间内画 "∞" 字形游走
            float angle1 = m_flowOffset * M_PI * 2;
            float angle2 = m_flowOffset * M_PI * 4; // 速度翻倍产生交错感

            qreal cx = r.width() * 0.5 + r.width() * 0.35 * qSin(angle1);   // 左右柔和荡漾
            qreal cy = r.height() * 0.5 + r.height() * 0.25 * qCos(angle2); // 上下交错浮动

            // 🔥 呼吸算法：伴随游走，光斑的透明度和大小像炭火一样忽明忽暗
            float breath = 0.7f + 0.3f * qSin(angle1);      // 在 0.4 到 1.0 之间呼吸
            qreal currentRadius = r.width() * 0.6 * breath; // 光圈大小随呼吸伸缩

            QRadialGradient floatGrad(QPointF(cx, cy), currentRadius);

            if (m_isDark)
            {
                // 赛博朋克深海幽紫的呼吸光斑 (高贵冷峻)
                floatGrad.setColorAt(0.0, shiftHue(QColor(0, 229, 255, 35 * breath), m_hueShift));  // 核心微亮清澈
                floatGrad.setColorAt(0.4, shiftHue(QColor(156, 39, 176, 20 * breath), m_hueShift)); // 水母紫晕染
                floatGrad.setColorAt(1.0, QColor(0, 0, 0, 0));                // 融于黑暗
            }
            else
            {
                // 亮色下的珊瑚橙呼吸
                floatGrad.setColorAt(0.0, shiftHue(QColor(255, 140, 0, 60 * breath), m_hueShift));
                floatGrad.setColorAt(0.4, shiftHue(QColor(255, 99, 71, 25 * breath), m_hueShift));
                floatGrad.setColorAt(1.0, QColor(255, 255, 255, 0));
            }

            p.setBrush(floatGrad);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(r.adjusted(1, 1, -1, -1), radius, radius);
            p.restore();
            // ==========================================

            // ✨ 3. 边缘流光层 (高级冷柔色调流转)
            float offset = m_flowOffset;
            QLinearGradient flowGrad(r.topLeft(), r.bottomRight());

            if (m_isDark)
            {
                // 用更加富有科技美学、晶莹剔透色彩流作为玻璃框 (青 -> 幽紫 -> 琥珀)
                flowGrad.setColorAt(qMin(1.0, 0.0 + offset * 0.2), shiftHue(QColor(0, 229, 255, 160), m_hueShift));
                flowGrad.setColorAt(qMin(1.0, 0.3 + offset * 0.3), shiftHue(QColor(156, 39, 176, 120), m_hueShift));
                flowGrad.setColorAt(qMin(1.0, 0.6 + offset * 0.2), shiftHue(QColor(255, 140, 0, 100), m_hueShift));
                flowGrad.setColorAt(1.0, shiftHue(QColor(30, 0, 80, 50), m_hueShift));
            }
            else
            {
                flowGrad.setColorAt(qMin(1.0, 0.0 + offset * 0.3), shiftHue(QColor(255, 99, 71, 160), m_hueShift));
                flowGrad.setColorAt(qMin(1.0, 0.3 + offset * 0.2), shiftHue(QColor(255, 140, 0, 130), m_hueShift));
                flowGrad.setColorAt(qMin(1.0, 0.6 + offset * 0.2), shiftHue(QColor(255, 215, 0, 100), m_hueShift));
                flowGrad.setColorAt(1.0, shiftHue(QColor(240, 128, 128, 50), m_hueShift));
            }

            // 边缘多层发光 (你之前写好的绝佳逻辑)
            float maxAlpha = m_isDark ? 0.9f : 0.65f;
            for (int i = 0; i < 8; ++i)
            {
                float layerFactor = (8 - i) / 8.0f;
                float finalOpacity = maxAlpha * layerFactor;
                if (finalOpacity <= 0.05f)
                    break;
                p.setOpacity(qBound(0.0f, finalOpacity, 1.0f));
                QPen pen;
                pen.setBrush(flowGrad);
                pen.setWidth(1);
                p.setPen(pen);
                int shrink = 1 + i;
                p.drawRoundedRect(r.adjusted(shrink, shrink, -shrink, -shrink), radius > 0 ? radius - 1 : 0, radius > 0 ? radius - 1 : 0);
            }
            p.setOpacity(1.0);

            // 顶部高光
            QPen glintPen;
            QLinearGradient glint(r.topLeft(), r.topRight());
            glint.setColorAt(0.0, QColor(255, 255, 255, m_isDark ? 80 : 150));
            glint.setColorAt(0.5, QColor(255, 255, 255, 0));
            glint.setColorAt(1.0, QColor(255, 255, 255, m_isDark ? 40 : 100));
            p.setPen(QPen(glint, 1));
            p.drawLine(r.left() + radius, r.top() + 1, r.right() - radius, r.top() + 1);
        }
    else
        {
            // ✨ 4. 旧版发光效果
            // 将 flowOffset 传入，瞬间激活边框的跑马灯循环流动效果！
            drawLegacyGlowEffect(p, r, m_isDark, m_alpha, radius, qBound(0.0f, m_glowFactor * 1.0f, 1.3f), m_flowOffset);
        }
    }

private:
    bool m_isDark, m_isRounded = true;
    int m_alpha = 210; // 默认透明度
    int m_hueShift = 0; // 🌈 颜色偏移
    float m_flowOffset = 0.0f;
    float m_glowFactor = 1.0f;
    GlassRenderMode m_renderMode = GlassRenderMode::Frosted; // 🎨 渲染模式
    QPropertyAnimation *m_flowAnim;
    QPropertyAnimation *m_pulseAnim;
};