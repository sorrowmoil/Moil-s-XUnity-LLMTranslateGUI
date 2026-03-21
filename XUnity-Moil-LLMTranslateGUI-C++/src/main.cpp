// --- main.cpp ---
// Program entry point: initializes the application, loads configuration, and manages window switching logic.
// 程序入口：负责初始化应用、加载配置及管理窗口切换逻辑。

#include <QApplication>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QTimer>
#include <cmath>
#include <algorithm>
#include "MainWindow.h"    // Classic mode window header ; 经典模式窗口头文件
#include "ModernWindow.h"  // Modern mode window header ; 流光模式窗口头文件
#include "ConfigManager.h" // Configuration manager header ; 配置管理器头文件

/**
 * Transition overlay that creates a "ripple" effect when switching from Classic to Modern mode.
 * 过渡覆盖层，用于在从经典模式切换到流光模式时产生“涟漪”效果。
 *
 * This widget captures a screenshot of the Classic window, then displays it with a circular cut‑out
 * that expands from the cursor position, revealing the Modern window underneath. A colored wave
 * follows the edge of the cut‑out for visual flair.
 * 该控件捕获经典窗口的截图，然后显示它，并从鼠标光标位置开始扩展一个圆形剪裁区域，
 * 露出底层的流光窗口。剪裁边缘跟随一个彩色波进行视觉增强。
 */
class TransitionRippleOverlay : public QWidget
{
public:
    QPixmap m_pixmap;    ///< Screenshot of the Classic window before the transition ; 过渡前经典窗口的截图
    QPoint m_center;     ///< Epicenter of the ripple (cursor position) ; 涟漪中心点（鼠标位置）
    float m_radius = 0;  ///< Current radius of the circular cut‑out ; 圆形剪裁区域的当前半径
    bool m_isDarkTarget; ///< Whether the target (Modern) window is in dark mode (affects wave color) ; 目标（流光）窗口是否为暗色模式（影响波的颜色）

    /**
     * Constructor. Sets up the overlay as a top‑level, frameless, always‑on‑top widget.
     * 构造函数。将覆盖层设置为顶层、无边框、置顶控件。
     *
     * @param pix     Screenshot of the Classic window ; 经典窗口截图
     * @param center  Epicenter of the effect (cursor position) ; 效果中心点（鼠标位置）
     * @param isDark  Dark mode status of the target window ; 目标窗口的暗色模式状态
     */
    TransitionRippleOverlay(const QPixmap &pix, const QPoint &center, bool isDark)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint),
          m_pixmap(pix), m_center(center), m_isDarkTarget(isDark)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents); // Allow clicks to pass through ; 允许点击穿透
        setAttribute(Qt::WA_TranslucentBackground);     // Enable transparency for the wave ; 启用透明度以绘制波
        setAttribute(Qt::WA_DeleteOnClose);             // Automatically delete when closed ; 关闭时自动删除
    }

protected:
    /**
     * Custom paint event that draws the clipped screenshot and the expanding wave.
     * 自定义绘制事件，绘制剪裁后的截图和扩展的波。
     *
     * @param event Unused ; 未使用
     */
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        // 1. Create a clipping path: the whole widget minus a circular hole.
        // 1. 创建剪裁路径：整个控件减去一个圆形孔洞。
        QPainterPath path;
        path.addRect(rect());
        path.addEllipse(QPointF(m_center), m_radius, m_radius);
        p.setClipPath(path);
        // Draw the screenshot – only the part outside the circle is visible.
        // 绘制截图 – 只有圆形外部的部分可见。
        p.drawPixmap(0, 0, m_pixmap);
        p.setClipping(false);

        // 2. Draw the colored wave that follows the expanding edge.
        // 2. 绘制跟随扩展边缘的彩色波。
        if (m_radius > 0)
        {
            float waveFront = m_radius + 5.0f;                 // Outer edge of the wave ; 波的外边缘
            float waveTail = std::max(0.0f, m_radius - 70.0f); // Inner edge of the wave ; 波的内边缘
            if (waveFront <= 0)
                return;

            QRadialGradient grad(m_center, waveFront);
            float cutStop = m_radius / waveFront;  // Position of the cut edge in gradient space ; 剪裁边缘在渐变空间中的位置
            float tailStop = waveTail / waveFront; // Position of the wave tail ; 波尾的位置

            // Wave colors: for dark target → hot orange, for light target → purple.
            // 波的颜色：暗色目标 → 炽热橙色，亮色目标 → 紫色。
            QColor coreColor = m_isDarkTarget ? QColor(255, 120, 0, 255) : QColor(148, 0, 211, 220);
            QColor midColor = m_isDarkTarget ? QColor(255, 120, 0, 90) : QColor(148, 0, 211, 90);

            grad.setColorAt(1.0, Qt::transparent);
            grad.setColorAt(qMin(1.0f, cutStop + 0.015f), coreColor); // Bright leading edge ; 明亮的锋刃
            grad.setColorAt(cutStop, coreColor);                      // Exact edge ; 精确边缘

            if (tailStop > 0 && tailStop < cutStop)
            {
                grad.setColorAt(std::max(tailStop, cutStop - 0.15f), midColor); // Fading inner part ; 渐隐的内部
                grad.setColorAt(tailStop, Qt::transparent);
            }
            grad.setColorAt(0.0, Qt::transparent);

            p.setBrush(grad);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(m_center), waveFront, waveFront);
        }
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion"); // Use Fusion style for consistent appearance ; 使用Fusion风格以保证外观一致

    // Create both windows (only one will be shown initially).
    // 创建两个窗口（初始只显示一个）。
    MainWindow classicWin;
    ModernWindow modernWin(classicWin.getServer()); // Modern window shares the same server instance ; 流光窗口共享同一个服务器实例

    // Load configuration to determine which mode to start with.
    // 加载配置以决定从哪个模式启动。
    AppConfig startupCfg = ConfigManager::loadConfig("config.ini");

    if (startupCfg.ui_mode == 1)
    {
        modernWin.move(classicWin.pos()); // Position at the same place as classic window ; 放在与经典窗口相同的位置
        modernWin.show();
    }
    else
    {
        classicWin.show();
    }

    // ==================================================================
    // Transition from Classic to Modern mode (holographic reveal effect)
    // ==================================================================
    QObject::connect(&classicWin, &MainWindow::requestModernView, [&]()
                     {
        // Save configuration with ui_mode set to 1 (Modern)
        // 保存配置，将ui_mode设置为1（流光模式）
        AppConfig cfg = classicWin.getUiConfig();
        cfg.ui_mode = 1; 
        classicWin.getServer()->updateConfig(cfg);
        ConfigManager::saveConfig(cfg, "config.ini");

        // Determine dark mode status from config (affects wave colors)
        // 从配置中确定暗色模式状态（影响波的颜色）
        bool isDark = ConfigManager::loadConfig("config.ini").is_dark;

        // 1. Get the epicenter (cursor position) in Classic window coordinates.
        // 1. 获取震中（鼠标位置）在经典窗口坐标系中的坐标。
        QPoint globalPos = QCursor::pos();
        QPoint epicenter = classicWin.mapFromGlobal(globalPos);

        // 2. Capture a screenshot of the Classic window.
        // 2. 截取经典窗口的截图。
        QPixmap pixmap = classicWin.grab();
        QRect startGeo = classicWin.geometry();
        QPoint startPos = classicWin.pos();

        // 3. Create and show the transition overlay immediately.
        // 3. 立即创建并显示过渡覆盖层。
        TransitionRippleOverlay *overlay = new TransitionRippleOverlay(pixmap, epicenter, isDark);
        overlay->setGeometry(startGeo);
        overlay->show();
        // Force an immediate repaint to ensure the overlay is drawn before hiding the classic window.
        // 强制立即重绘，确保在隐藏经典窗口前覆盖层已经绘制。
        overlay->repaint();

        // 4. Hide the Classic window and show the Modern window underneath.
        // 4. 隐藏经典窗口，并在底层显示流光窗口。
        classicWin.hide();
        
        modernWin.setProperty("bypass_fade", true); // Tell Modern window not to play its own fade‑in ; 告诉流光窗口不播放自身的淡入动画
        modernWin.move(startPos); 
        modernWin.show(); 
        modernWin.repaint(); // Force Modern window to be fully prepared (opacity 1) underneath ; 强制流光窗口在底层以100%不透明度准备就绪

        // 5. Calculate the maximum radius needed to completely reveal the Modern window.
        // 5. 计算完全露出流光窗口所需的最大半径。
        int w = startGeo.width();
        int h = startGeo.height();
        double maxRadius = std::sqrt(std::pow(std::max(epicenter.x(), w - epicenter.x()), 2) +
                                     std::pow(std::max(epicenter.y(), h - epicenter.y()), 2));
        maxRadius += 30; // Add a small margin ; 增加一点余量

        // 6. Animate the overlay's radius to create the expanding ripple.
        // 6. 动画化覆盖层的半径以产生扩展的涟漪。
        QVariantAnimation *anim = new QVariantAnimation(overlay);
        anim->setDuration(480);
        anim->setStartValue(0.0f);
        anim->setEndValue((float)maxRadius);
        anim->setEasingCurve(QEasingCurve::OutQuart);

        QObject::connect(anim, &QVariantAnimation::valueChanged, [overlay](const QVariant& val){
            overlay->m_radius = val.toFloat();
            overlay->update();  // Redraw with new radius ; 用新半径重绘
        });

        QObject::connect(anim, &QVariantAnimation::finished, overlay, &QWidget::close);

        // Start the animation after a tiny delay to ensure all widgets are ready.
        // 延迟一小段时间后启动动画，确保所有控件已准备就绪。
        QTimer::singleShot(15, overlay, [anim](){ anim->start(); }); });

    // ==================================================================
    // Transition from Modern to Classic mode (simple crossfade)
    // ==================================================================
    QObject::connect(&modernWin, &ModernWindow::requestClassicMode, [&]()
                     {
        // Update configuration to Classic mode.
        // 更新配置为经典模式。
        AppConfig cfg = modernWin.getUiConfig();
        cfg.ui_mode = 0; // Target mode ; 目标模式
        
        modernWin.getServer()->updateConfig(cfg);
        ConfigManager::saveConfig(cfg, "config.ini");

        // Create fade‑out animation for the Modern window.
        // 为流光窗口创建淡出动画。
        QPropertyAnimation *fadeOut = new QPropertyAnimation(&modernWin, "windowOpacity");
        fadeOut->setDuration(250);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        
        // After fade‑out, hide Modern and show Classic with fade‑in.
        // 淡出后，隐藏流光窗口并淡入经典窗口。
        QObject::connect(fadeOut, &QPropertyAnimation::finished, [&](){
            modernWin.hide();
            classicWin.move(modernWin.pos()); // Sync positions ; 同步位置
            
            // Reload config to get latest settings (e.g., dark mode).
            // 重新加载配置以获取最新设置（如暗色模式）。
            classicWin.loadConfigToUi(); 
            
            classicWin.setWindowOpacity(0.0); // Start fully transparent ; 初始全透明
            classicWin.show();                // Show Classic window ; 显示经典窗口
            
            // Fade in Classic window.
            // 淡入经典窗口。
            QPropertyAnimation *fadeIn = new QPropertyAnimation(&classicWin, "windowOpacity");
            fadeIn->setDuration(250);
            fadeIn->setStartValue(0.0);
            fadeIn->setEndValue(1.0);
            fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        });
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped); });

    // Enter the event loop.
    // 进入事件循环。
    return app.exec();
}