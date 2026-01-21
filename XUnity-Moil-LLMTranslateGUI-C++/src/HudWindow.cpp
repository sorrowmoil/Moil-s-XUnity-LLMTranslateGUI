#include "HudWindow.h"
#include <QPushButton>

HudWindow::HudWindow(QWidget *parent) : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool) {
    // 设置半透明背景尺寸和属性
    setAttribute(Qt::WA_TranslucentBackground);
    resize(260, 40);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 5, 15, 5);
    layout->setSpacing(10);

    // 1. 呼吸灯
    m_light = new StatusLight(this);
    layout->addWidget(m_light);

    // 2. 标题/状态文字
    m_lblTitle = new QLabel("XUnity Translator", this);
    m_lblTitle->setStyleSheet("color: white; font-weight: bold; font-family: Segoe UI;");
    layout->addWidget(m_lblTitle);

    layout->addStretch();

    // 3. Token 计数
    m_lblTokens = new QLabel("TK: 0", this);
    m_lblTokens->setStyleSheet("color: #FFD700; font-size: 11px;");
    layout->addWidget(m_lblTokens);

    // 4. 还原按钮
    QPushButton *btnRestore = new QPushButton("❐", this);
    btnRestore->setFixedSize(24, 24);
    btnRestore->setToolTip("Restore Window / 还原窗口");
    btnRestore->setCursor(Qt::PointingHandCursor);
    btnRestore->setStyleSheet(
        "QPushButton { background: transparent; color: #CCCCCC; border: none; font-size: 14px; }"
        "QPushButton:hover { color: white; }"
    );
    connect(btnRestore, &QPushButton::clicked, this, &HudWindow::requestRestore);
    layout->addWidget(btnRestore);

    // 初始化呼吸动画
    m_breathAnim = new QPropertyAnimation(m_light, "color", this);
    m_breathAnim->setDuration(800);
    m_breathAnim->setStartValue(QColor("#00BFFF")); // 深天蓝
    m_breathAnim->setEndValue(QColor("#E0FFFF"));   // 淡青色
    m_breathAnim->setLoopCount(-1); // 无限循环
    m_breathAnim->setEasingCurve(QEasingCurve::InOutQuad);
}

void HudWindow::updateTokens(long long total) {
    m_lblTokens->setText(QString("TK: %1").arg(total));
}

void HudWindow::setStatus(bool isWorking, bool isError) {
    if (isWorking) {
        // 如果正在工作，强制转为蓝色呼吸状态
        m_light->setState(1);
        m_lblTitle->setText("Translating...");
        if (m_breathAnim->state() == QAbstractAnimation::Stopped) {
            m_breathAnim->start();
        }
    } else {
        m_breathAnim->stop();
        if (isError) {
            // 错误状态：红色
            m_light->setState(2); 
            m_lblTitle->setText("Error / Retry");
            // 即使是 Error，也保持停止动画，红灯常亮比较醒目
        } else {
            // 空闲状态：绿色
            m_light->setState(0); 
            m_lblTitle->setText("Ready");
        }
    }
    update(); // 强制刷新界面
}

// --- 拖拽逻辑 ---
void HudWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void HudWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

// --- 背景绘制 (圆角深色磨砂感) ---
void HudWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // 背景色：深灰，带透明度
    QColor bgColor(45, 45, 48, 240); 
    p.setBrush(bgColor);
    p.setPen(QPen(QColor(80, 80, 80), 1)); // 细边框
    
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 10, 10);
}
