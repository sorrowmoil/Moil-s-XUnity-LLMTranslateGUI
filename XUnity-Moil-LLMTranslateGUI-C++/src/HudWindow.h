#pragma once
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QMouseEvent>
#include <QPropertyAnimation>

/**
 * Custom status light widget with three states and a breathing animation.
 * 自定义状态灯控件，支持三种状态和呼吸动画。
 * 
 * The light is a small round indicator. Its color and behavior depend on the current state:
 * - Idle   (0) : static green
 * - Working (1) : breathing cyan/blue (driven by an external QPropertyAnimation)
 * - Error   (2) : static red
 * 该灯是一个小型圆形指示器。其颜色和行为取决于当前状态：
 * - 空闲   (0) : 静态绿色
 * - 工作   (1) : 呼吸青色/蓝色（由外部 QPropertyAnimation 驱动）
 * - 错误   (2) : 静态红色
 * 
 * The breathing effect is achieved by animating the `color` property, which is then used
 * as the fill color when the state is "Working".
 * 呼吸效果通过动画化 `color` 属性实现，当状态为“工作”时，该颜色用作填充色。
 */
class StatusLight : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor) // Property for animation ; 用于动画的属性

public:
    /**
     * Constructor. Sets a fixed size (14×14) and the default idle color.
     * 构造函数。设置固定尺寸（14×14）和默认空闲颜色。
     * 
     * @param parent Parent widget ; 父控件
     */
    explicit StatusLight(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(14, 14);
        m_color = QColor("#00FF00"); // Default green (Idle) ; 默认绿色（空闲）
    }

    /**
     * Set the current state of the light.
     * 设置灯的当前状态。
     * 
     * @param state 0 = Idle (green), 1 = Working (cyan/blue), 2 = Error (red)
     */
    void setState(int state) { 
        m_state = state;
        update(); // Trigger a repaint ; 触发重绘
    }

    /**
     * Getter for the animated color (used by the property system).
     * 获取动画颜色（供属性系统使用）。
     */
    QColor color() const { return m_animColor; }

    /**
     * Setter for the animated color (used by the property system).
     * 设置动画颜色（供属性系统使用）。
     */
    void setColor(const QColor &c) { m_animColor = c; update(); }

protected:
    /**
     * Custom paint event to draw the status light.
     * 自定义绘制事件，绘制状态灯。
     * 
     * Draws a small circle with a soft outer glow. The fill color depends on the current state.
     * 绘制一个小圆，带有柔和的外发光。填充色取决于当前状态。
     * 
     * @param event Paint event (unused) ; 绘制事件（未使用）
     */
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QColor drawColor;
        if (m_state == 1) { // Working: use the animated color ; 工作中：使用动画颜色
             drawColor = m_animColor;
        } else if (m_state == 2) {
             drawColor = Qt::red;
        } else {
             drawColor = QColor("#4CAF50"); // Green ; 绿色
        }

        // Draw the core of the light ; 绘制灯芯
        p.setBrush(drawColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(2, 2, 10, 10);
        
        // Draw a faint outer glow (semi‑transparent larger circle) ; 绘制微弱的外发光（半透明大圆）
        drawColor.setAlpha(100);
        p.setBrush(drawColor);
        p.drawEllipse(0, 0, 14, 14);
    }

private:
    int m_state = 0;               ///< Current state (0:idle, 1:working, 2:error) ; 当前状态
    QColor m_color;                 ///< Static color (not used directly for working state) ; 静态颜色（工作状态不直接使用）
    QColor m_animColor = QColor("#00BFFF"); ///< Color used for the breathing animation ; 用于呼吸动画的颜色
};

/**
 * Main HUD (heads‑up display) window.
 * HUD（平视显示）主窗口。
 * 
 * This is a small, frameless, always‑on‑top window that shows the translation status,
 * token count, and provides a button to restore the main application window.
 * It supports dragging via mouse and has a custom translucent rounded background.
 * 这是一个小型、无边框、置顶窗口，显示翻译状态、令牌计数，并提供还原主应用程序窗口的按钮。
 * 支持鼠标拖拽，并具有自定义的半透明圆角背景。
 */
class HudWindow : public QWidget {
    Q_OBJECT

public:
    /**
     * Constructor. Sets up the UI, widgets, and the breathing animation.
     * 构造函数。设置UI、控件和呼吸动画。
     * 
     * @param parent Parent widget (usually nullptr) ; 父窗口（通常为nullptr）
     */
    explicit HudWindow(QWidget *parent = nullptr);

    /**
     * Update the displayed token count.
     * 更新显示的令牌计数。
     * 
     * @param total Total tokens used so far ; 到目前为止使用的总令牌数
     */
    void updateTokens(long long total);

    /**
     * Set the HUD status based on working state and error condition.
     * 根据工作状态和错误条件设置HUD状态。
     * 
     * @param isWorking Whether translation is in progress ; 是否正在翻译
     * @param isError   Whether an error has occurred ; 是否发生错误
     */
    void setStatus(bool isWorking, bool isError = false);

signals:
    /**
     * Signal emitted when the restore button is clicked.
     * Requests the main window to be shown/restored.
     * 当还原按钮被点击时发射的信号。请求主窗口显示/还原。
     */
    void requestRestore();

protected:
    /**
     * Handle mouse press to initiate window dragging.
     * 处理鼠标按下以开始窗口拖拽。
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * Handle mouse move to perform window dragging.
     * 处理鼠标移动以执行窗口拖拽。
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * Custom paint event for the window background.
     * 窗口背景的自定义绘制事件。
     */
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint m_dragPosition;               ///< Drag offset (mouse position relative to window top‑left) ; 拖拽偏移量（鼠标相对于窗口左上角的位置）
    StatusLight *m_light;                 ///< Status light widget ; 状态灯控件
    QLabel *m_lblTokens;                  ///< Label showing token count ; 显示令牌计数的标签
    QLabel *m_lblTitle;                   ///< Label showing title or status text ; 显示标题或状态文字的标签
    QPropertyAnimation *m_breathAnim;      ///< Animation for the breathing effect (working state) ; 呼吸效果动画（工作状态）
};