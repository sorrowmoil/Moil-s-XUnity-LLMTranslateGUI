#include "HudWindow.h"
#include <QPushButton>

/**
 * Constructor for the HUD window.
 * HUD窗口的构造函数。
 * 
 * Initializes a frameless, always‑on‑top window with a translucent background.
 * Sets up the layout, child widgets (status light, title, token counter, restore button),
 * and the breathing animation for the status light.
 * 初始化一个无边框、置顶、半透明背景的窗口。
 * 设置布局、子控件（状态灯、标题、令牌计数器、还原按钮），以及状态灯的呼吸动画。
 * 
 * @param parent Parent widget (usually nullptr) ; 父窗口（通常为nullptr）
 */
HudWindow::HudWindow(QWidget *parent) : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool) {
    // Enable translucent background for custom rounded corners and transparency
    // 启用半透明背景以实现自定义圆角和透明度
    setAttribute(Qt::WA_TranslucentBackground);
    // Set fixed size for the HUD bar ; 设置HUD栏的固定尺寸
    resize(260, 40);

    // Create a horizontal layout with margins and spacing
    // 创建水平布局，设置边距和间距
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 5, 15, 5);
    layout->setSpacing(10);

    // 1. Status light (breathing / colored indicator)
    // 1. 状态灯（呼吸/彩色指示器）
    m_light = new StatusLight(this);
    layout->addWidget(m_light);

    // 2. Title / status text label
    // 2. 标题/状态文字标签
    m_lblTitle = new QLabel("XUnity Translator", this);
    m_lblTitle->setStyleSheet("color: white; font-weight: bold; font-family: Segoe UI;");
    layout->addWidget(m_lblTitle);

    // Spacer to push following widgets to the right
    // 弹簧将后续控件推到右边
    layout->addStretch();

    // 3. Token counter label
    // 3. 令牌计数器标签
    m_lblTokens = new QLabel("TK: 0", this);
    m_lblTokens->setStyleSheet("color: #FFD700; font-size: 11px;");
    layout->addWidget(m_lblTokens);

    // 4. Restore button to bring back the main window
    // 4. 还原按钮，用于恢复主窗口
    QPushButton *btnRestore = new QPushButton("❐", this);
    btnRestore->setFixedSize(24, 24);
    btnRestore->setToolTip("Restore Window / 还原窗口");
    btnRestore->setCursor(Qt::PointingHandCursor);
    btnRestore->setStyleSheet(
        "QPushButton { background: transparent; color: #CCCCCC; border: none; font-size: 14px; }"
        "QPushButton:hover { color: white; }"
    );
    // Connect the button's click to the signal that requests main window restoration
    // 连接按钮点击到请求还原主窗口的信号
    connect(btnRestore, &QPushButton::clicked, this, &HudWindow::requestRestore);
    layout->addWidget(btnRestore);

    // Initialize the breathing animation for the status light
    // 初始化状态灯的呼吸动画
    m_breathAnim = new QPropertyAnimation(m_light, "color", this);
    m_breathAnim->setDuration(800);
    m_breathAnim->setStartValue(QColor("#00BFFF")); // Deep sky blue ; 深天蓝
    m_breathAnim->setEndValue(QColor("#E0FFFF"));   // Light cyan ; 淡青色
    m_breathAnim->setLoopCount(-1); // Infinite loop ; 无限循环
    m_breathAnim->setEasingCurve(QEasingCurve::InOutQuad);
}

/**
 * Update the token count displayed in the HUD.
 * 更新HUD中显示的令牌计数。
 * 
 * @param total Total tokens used so far ; 到目前为止使用的总令牌数
 */
void HudWindow::updateTokens(long long total) {
    m_lblTokens->setText(QString("TK: %1").arg(total));
}

/**
 * Set the HUD status based on working state and error condition.
 * 根据工作状态和错误条件设置HUD状态。
 * 
 * When working, the status light breathes blue and the title shows "Translating...".
 * On error (not working but error true), the light turns red (static) and the title shows "Error / Retry".
 * When idle (not working, no error), the light turns green (static) and the title shows "Ready".
 * 工作时，状态灯呼吸蓝色，标题显示“Translating...”。
 * 出错时（不工作但错误为真），状态灯变为红色（静态），标题显示“Error / Retry”。
 * 空闲时（不工作且无错误），状态灯变为绿色（静态），标题显示“Ready”。
 * 
 * @param isWorking Whether translation is in progress ; 是否正在翻译
 * @param isError   Whether an error has occurred ; 是否发生错误
 */
void HudWindow::setStatus(bool isWorking, bool isError) {
    if (isWorking) {
        // Working: force blue breathing state ; 工作中：强制蓝色呼吸状态
        m_light->setState(1);
        m_lblTitle->setText("Translating...");
        if (m_breathAnim->state() == QAbstractAnimation::Stopped) {
            m_breathAnim->start();
        }
    } else {
        // Stop animation when not working ; 不工作时停止动画
        m_breathAnim->stop();
        if (isError) {
            // Error state: red static light ; 错误状态：红色常亮
            m_light->setState(2); 
            m_lblTitle->setText("Error / Retry");
        } else {
            // Idle state: green static light ; 空闲状态：绿色常亮
            m_light->setState(0); 
            m_lblTitle->setText("Ready");
        }
    }
    // Force a repaint ; 强制重绘
    update();
}

// --- Drag logic for the frameless window ---
// --- 无边框窗口的拖拽逻辑 ---

/**
 * Handle mouse press events to initiate window dragging.
 * 处理鼠标按下事件以开始窗口拖拽。
 * 
 * Records the offset between the mouse position and the window top‑left corner.
 * 记录鼠标位置与窗口左上角的偏移量。
 * 
 * @param event Mouse event ; 鼠标事件
 */
void HudWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

/**
 * Handle mouse move events to perform window dragging.
 * 处理鼠标移动事件以执行窗口拖拽。
 * 
 * Moves the window to keep the offset constant while the left button is held.
 * 当左键按下时移动窗口，保持偏移量恒定。
 * 
 * @param event Mouse event ; 鼠标事件
 */
void HudWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

// --- Custom background painting (rounded dark translucent) ---
// --- 自定义背景绘制（圆角深色半透明） ---

/**
 * Paint event for custom window background.
 * 自定义窗口背景的绘制事件。
 * 
 * Draws a rounded rectangle with a dark translucent fill and a thin border.
 * Antialiasing is enabled for smooth edges.
 * 绘制一个带有深色半透明填充和细边框的圆角矩形。
 * 启用抗锯齿以获得平滑边缘。
 * 
 * @param event Paint event (unused) ; 绘制事件（未使用）
 */
void HudWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Background color: dark gray with slight transparency ; 背景色：深灰色，略带透明度
    QColor bgColor(45, 45, 48, 240); 
    p.setBrush(bgColor);
    // Border: thin line ; 边框：细线
    p.setPen(QPen(QColor(80, 80, 80), 1));
    
    // Draw rounded rectangle with 10px corner radius ; 绘制圆角矩形，圆角半径10像素
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 10, 10);
}