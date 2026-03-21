#pragma once
#include <QWidget>
#include <QPainter>
#include <QTimer>

/**
 * Loading overlay widget that displays a semi‑transparent mask and a spinning indicator.
 * 加载覆盖层控件，显示半透明遮罩和旋转指示器。
 *
 * This widget is intended to be placed on top of another widget (e.g., a button or a panel)
 * to indicate that an operation is in progress and to block user interaction during that time.
 * 该控件设计放置在其他控件（如按钮或面板）之上，以指示操作正在进行中，并在该期间阻止用户交互。
 */
class LoadingOverlay : public QWidget
{
    Q_OBJECT
public:
    /**
     * Constructor. Sets up the widget attributes and the animation timer.
     * 构造函数。设置控件属性和动画定时器。
     *
     * The widget is initially hidden. The timer is connected to update the rotation angle.
     * 控件初始为隐藏状态。定时器连接到更新旋转角度。
     *
     * @param parent Parent widget (the widget over which the overlay is shown) ; 父控件（覆盖层所覆盖的控件）
     */
    explicit LoadingOverlay(QWidget *parent = nullptr) : QWidget(parent)
    {
        // Set transparent background so the underlying widget remains visible.
        // 设置透明背景，使底层控件可见。
        setAttribute(Qt::WA_TransparentForMouseEvents, false); // Set to false to intercept clicks ; 设为 false 以拦截点击
        hide();

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, [this]()
                {
                    m_angle = (m_angle + 30) % 360;
                    update(); // Request a repaint ; 请求重绘
                });
    }

    /**
     * Start the loading animation and show the overlay.
     * 启动加载动画并显示覆盖层。
     *
     * Resizes the overlay to match the parent widget's size, resets the rotation angle,
     * makes the overlay visible, and starts the timer with a 50 ms interval (≈20 FPS).
     * 将覆盖层大小调整为与父控件相同，重置旋转角度，显示覆盖层，并以50毫秒间隔启动定时器（≈20帧/秒）。
     */
    void start()
    {
        if (parentWidget())
            resize(parentWidget()->size());
        m_angle = 0;
        show();
        m_timer->start(50); // Approximately 20 frames per second ; 约20帧/秒
    }

    /**
     * Stop the loading animation and hide the overlay.
     * 停止加载动画并隐藏覆盖层。
     */
    void stop()
    {
        m_timer->stop();
        hide();
    }

protected:
    /**
     * Custom paint event to draw the overlay.
     * 自定义绘制事件，绘制覆盖层。
     *
     * Draws a semi‑transparent mask over the entire widget, then draws a rotating circle
     * consisting of eight lines with varying opacity to create a spinning effect.
     * 在整个控件上绘制半透明遮罩，然后绘制由八条线条组成的旋转圆圈，线条透明度渐变以产生旋转效果。
     *
     * @param event Paint event (unused) ; 绘制事件（未使用）
     */
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // 1. Draw the semi‑transparent mask (slightly dims the underlying content)
        // 1. 绘制半透明遮罩（稍微压暗底层内容）
        QColor bgColor = palette().color(QPalette::Window);
        bgColor.setAlpha(180);
        p.setBrush(bgColor);
        p.setPen(Qt::NoPen);
        // Draw a rounded rectangle with a corner radius of 4 (matching typical button roundness)
        // 绘制圆角矩形，圆角半径为4（与常见按钮圆角匹配）
        p.drawRoundedRect(rect(), 4, 4);

        // 2. Draw the rotating spinner
        // 2. 绘制旋转圆圈
        p.translate(width() / 2, height() / 2); // Move canvas center to widget center ; 将画布中心移至控件中心
        p.rotate(m_angle);                      // Apply current rotation ; 应用当前旋转角度

        QColor spinnerColor = palette().color(QPalette::Highlight); // Use theme highlight color ; 使用主题高亮色
        QPen pen(spinnerColor, 2);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);

        // Draw eight lines around the center, each with increasing opacity to create a spinning trail.
        // 绘制围绕中心的八条线条，每条线条透明度递增以产生旋转拖尾效果。
        for (int i = 0; i < 8; ++i)
        {
            p.rotate(45);                  // Rotate 45° for each line ; 每条线旋转45度
            p.setOpacity(0.125 * (i + 1)); // Opacity increases from 0.125 to 1.0 ; 透明度从0.125增加到1.0
            p.drawLine(0, -6, 0, -10);     // Draw a small line segment ; 绘制短线段
        }
    }

private:
    QTimer *m_timer; ///< Timer driving the animation ; 驱动动画的定时器
    int m_angle = 0; ///< Current rotation angle in degrees ; 当前旋转角度（度）
};