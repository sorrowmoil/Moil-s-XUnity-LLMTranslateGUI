#pragma once
#include <QWidget>
#include <QPainter>
#include <QTimer>

class LoadingOverlay : public QWidget {
    Q_OBJECT
public:
    explicit LoadingOverlay(QWidget *parent = nullptr) : QWidget(parent) {
        // 设置透明背景，不干扰父窗口绘图
        setAttribute(Qt::WA_TransparentForMouseEvents, false); // 设为 false 以拦截点击，防止重复操作
        hide();
        
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, [this]() {
            m_angle = (m_angle + 30) % 360;
            update();
        });
    }

    void start() {
        if (parentWidget()) resize(parentWidget()->size());
        m_angle = 0;
        show();
        m_timer->start(50); // 约 20 帧/秒，既流畅又省资源
    }

    void stop() {
        m_timer->stop();
        hide();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // 1. 绘制半透明遮罩背景（稍微压暗/亮按钮文字）
        QColor bgColor = palette().color(QPalette::Window);
        bgColor.setAlpha(180);
        p.setBrush(bgColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 4, 4); // 4 是按钮圆角，可以根据 UI 调整

        // 2. 绘制旋转圆圈
        p.translate(width() / 2, height() / 2); // 移动画布中心
        p.rotate(m_angle);

        QColor spinnerColor = palette().color(QPalette::Highlight); // 跟随主题高亮色
        QPen pen(spinnerColor, 2);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);

        // 绘制 8 根线条组成的小圆圈
        for (int i = 0; i < 8; ++i) {
            p.rotate(45);
            p.setOpacity(0.125 * (i + 1)); // 渐变透明度营造旋转感
            p.drawLine(0, -6, 0, -10); // 线条长度和间距
        }
    }

private:
    QTimer *m_timer;
    int m_angle = 0;
};