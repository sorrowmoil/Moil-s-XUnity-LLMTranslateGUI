// --- main.cpp ---
// 程序入口：负责初始化应用、加载配置及管理窗口切换逻辑

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
#include <QScreen>
#include "MainWindow.h"    // 经典模式窗口头文件
#include "ModernWindow.h"  // 流光模式窗口头文件
#include "ConfigManager.h" // 配置管理器头文件

// ==========================================
// � 扑克牌翻面覆盖层：专用于 经典 -> 流光 的物理撕裂效果
// ==========================================
class TransitionFlipOverlay : public QWidget
{
public:
    QPixmap m_currentPix;
    QPixmap m_newPix;
    float m_angle = 0;
    QPoint m_centerGlobal; // 翻转的绝对物理中心参考点

    TransitionFlipOverlay(const QPixmap &currentPix, const QPixmap &newPix, const QRect &overlayGeo, const QPoint &centerGlobal)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint),
          m_currentPix(currentPix), m_newPix(newPix), m_centerGlobal(centerGlobal)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        setGeometry(overlayGeo);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        QPixmap activePix;
        if (m_angle < 90.0) {
            activePix = m_currentPix;
        } else {
            // 翻转背后需要使用新图片的镜像
            activePix = m_newPix.transformed(QTransform().scale(-1, 1), Qt::SmoothTransformation);
        }

        // --- 核心修复 1：动态尺寸插值融合 ---
        // 利用 progress 让两张不同长宽的图片在 90 度（0.5）交汇时，被强行缩放到同一个精确尺寸，实现物理上的超平滑过渡！
        double progress = m_angle / 180.0;
        double targetWidth = m_currentPix.width() * (1.0 - progress) + m_newPix.width() * progress;
        double targetHeight = m_currentPix.height() * (1.0 - progress) + m_newPix.height() * progress;
        
        double baseScaleX = targetWidth / activePix.width();
        double baseScaleY = targetHeight / activePix.height();

        // 加上透视纵深感的扭曲
        double perspectiveScale = 1.0 - (1.0 - std::abs(90.0 - m_angle) / 90.0) * 0.1;

        QTransform transform;
        // --- 核心修复 2：自坐标旋转系 ---
        transform.translate(activePix.width() / 2.0, activePix.height() / 2.0); // 原点归心
        transform.rotate(m_angle, Qt::YAxis);                                   // 旋转
        transform.scale(baseScaleX, baseScaleY * perspectiveScale);             // 形变+透视
        transform.translate(-activePix.width() / 2.0, -activePix.height() / 2.0);
        
        QPixmap transformedPix = activePix.transformed(transform, Qt::SmoothTransformation);

        // --- 核心修复 3：无论图片怎么形变，永远铆死在屏幕的同一个绝对坐标核心 ---
        QPoint localCenter = this->mapFromGlobal(m_centerGlobal);
        int dx = localCenter.x() - transformedPix.width() / 2;
        int dy = localCenter.y() - transformedPix.height() / 2;
        
        p.drawPixmap(dx, dy, transformedPix);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    MainWindow classicWin;
    ModernWindow modernWin(classicWin.getServer());

    AppConfig startupCfg = ConfigManager::loadConfig("config.ini");

    if (startupCfg.ui_mode == 1)
    {
        modernWin.move(classicWin.pos());
        modernWin.show();
    }
    else
    {
        classicWin.show();
    }

        // ============================================================
    // 🚀 [经典] -> [流光] (迎接未来)
    // 动效：扑克牌翻面 (Poker Card Flip)
    // ============================================================
    QObject::connect(&classicWin, &MainWindow::requestModernView, [&]()
                     {
        AppConfig cfg = classicWin.getUiConfig();
        cfg.ui_mode = 1; 
        classicWin.getServer()->updateConfig(cfg);
        ConfigManager::saveConfig(cfg, "config.ini");

        // 截取经典界面的旧图像(包含真实的系统边框外壳)
        QRect startGeo = classicWin.frameGeometry();
        QPoint startPos = classicWin.pos();
        QScreen *screen = classicWin.screen();
        if (!screen) screen = QGuiApplication::primaryScreen();
        // 直接从全屏幕缓冲区把这个窗口带边框完美“抠”下来
        QPixmap currentPix = screen->grabWindow(0, startGeo.x(), startGeo.y(), startGeo.width(), startGeo.height());
        
        // 若系统权限不足，降级方案
        if (currentPix.isNull()) {
            currentPix = QPixmap(classicWin.size());
            currentPix.fill(Qt::transparent);
            classicWin.render(&currentPix);
            startGeo = QRect(classicWin.mapToGlobal(QPoint(0,0)), classicWin.size());
        }

        // 把将要浮起的流光窗口与原窗口【中央对齐】
        modernWin.setProperty("bypass_fade", true);
        int targetX = startGeo.center().x() - modernWin.width() / 2;
        int targetY = startGeo.center().y() - modernWin.height() / 2;
        modernWin.move(targetX, targetY);
        modernWin.show();
        
        // 🔥 极其关键：物理隐形现代窗口
        modernWin.setWindowOpacity(0.0); 
        
        QTimer::singleShot(25, &modernWin, [&, currentPix, startGeo]() {
            // 截取新界面大图
            QPixmap newPix(modernWin.size());
            newPix.fill(Qt::transparent);
            modernWin.render(&newPix);

            // 构造容纳得下大翻面、且囊括两个窗口大小的巨大翻转舞台包围区
            QRect endGeo = modernWin.frameGeometry();
            QRect overlayGeo = startGeo.united(endGeo);
            overlayGeo.adjust(-150, -150, 150, 150); // 增加呼吸空间，防切割边角

            QPoint flipCenterGlobal = startGeo.center();

            // 召唤无形虚幻图层
            TransitionFlipOverlay *overlay = new TransitionFlipOverlay(currentPix, newPix, overlayGeo, flipCenterGlobal);
            overlay->show();
            overlay->repaint(); // 强制刷出掩人耳目！

            // 把被替身护住的本体经典界面立刻干掉
            classicWin.hide();

            // 引擎燃烧，展开物理级大翻板
            QVariantAnimation *anim = new QVariantAnimation(overlay);
            anim->setDuration(350); 
            anim->setStartValue(0.0f);
            anim->setEndValue(180.0f);
            anim->setEasingCurve(QEasingCurve::InOutSine); 

            QObject::connect(anim, &QVariantAnimation::valueChanged, [overlay](const QVariant& val){
                overlay->m_angle = val.toFloat();
                overlay->update();
            });

            QObject::connect(anim, &QVariantAnimation::finished, overlay, [overlay, &modernWin]() {
                overlay->close();
                modernWin.setWindowOpacity(1.0); // 替身使命结束，真身傲然显现场
            });

            anim->start(QAbstractAnimation::DeleteWhenStopped);
        });
    });

    // 4. 切换逻辑：从 [流光] 到 [经典]
    // 4. Switch Logic: From [Modern] to [Classic]
    QObject::connect(&modernWin, &ModernWindow::requestClassicMode, [&]()
                     {
        AppConfig cfg = modernWin.getUiConfig();
        
        // 🔥 关键修改：在切换前，强制将配置改为目标模式
        // 🔥 Key Modification: Before switching, force config to target mode
        cfg.ui_mode = 0; // 准备回到经典模式，下次启动就是经典 | Prepare to return to Classic, next launch will be Classic
        
        modernWin.getServer()->updateConfig(cfg);
        ConfigManager::saveConfig(cfg, "config.ini");

        // 创建流光窗口的淡出动画 | Create fade-out animation for Modern window
        QPropertyAnimation *fadeOut = new QPropertyAnimation(&modernWin, "windowOpacity");
        fadeOut->setDuration(250);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        
        // 动画结束后的回调 | Callback after animation finishes
        QObject::connect(fadeOut, &QPropertyAnimation::finished, [&](){
            modernWin.hide();             // 隐藏流光窗口 | Hide modern window
            classicWin.move(modernWin.pos()); // 同步位置 | Sync position
            
            // 重新从文件加载配置，以获取最新的 is_dark 和其他参数
            // Reload config from file to get latest is_dark and other parameters
            classicWin.loadConfigToUi(); 
            
            classicWin.setWindowOpacity(0.0); // 初始透明度设为 0 | Set initial opacity to 0
            classicWin.show();                // 显示经典窗口 | Show classic window
            
            // 创建经典窗口的淡入动画 | Create fade-in animation for Classic window
            QPropertyAnimation *fadeIn = new QPropertyAnimation(&classicWin, "windowOpacity");
            fadeIn->setDuration(250);
            fadeIn->setStartValue(0.0);
            fadeIn->setEndValue(1.0);
            fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        });
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped); });

    // 进入事件循环 | Enter event loop
    return app.exec();
}