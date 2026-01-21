#pragma once
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QMouseEvent>
#include <QPropertyAnimation>

// 自定义呼吸灯控件
class StatusLight : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor) // 用于动画属性

public:
    explicit StatusLight(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(14, 14);
        m_color = QColor("#00FF00"); // 默认绿色 (Idle)
    }

    void setState(int state) { 
        // 0: Idle (Green), 1: Working (Cyan/Blue), 2: Error (Red)
        m_state = state;
        update(); // 触发重绘
    }

    QColor color() const { return m_animColor; }
    void setColor(const QColor &c) { m_animColor = c; update(); }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QColor drawColor;
        if (m_state == 1) { // Working: 使用动画颜色
             drawColor = m_animColor;
        } else if (m_state == 2) {
             drawColor = Qt::red;
        } else {
             drawColor = QColor("#4CAF50"); // Green
        }

        // 绘制光晕
        p.setBrush(drawColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(2, 2, 10, 10);
        
        // 绘制微弱的外发光
        drawColor.setAlpha(100);
        p.setBrush(drawColor);
        p.drawEllipse(0, 0, 14, 14);
    }

private:
    int m_state = 0;
    QColor m_color;
    QColor m_animColor = QColor("#00BFFF"); // 初始动画色
};

// HUD 主窗口
class HudWindow : public QWidget {
    Q_OBJECT

public:
    explicit HudWindow(QWidget *parent = nullptr);

    // 更新显示数据
    void updateTokens(long long total);
    void setStatus(bool isWorking, bool isError = false);

signals:
    void requestRestore(); // 请求还原回主窗口

protected:
    // 实现无边框拖拽
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    // 自定义背景绘制
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint m_dragPosition;
    StatusLight *m_light;
    QLabel *m_lblTokens;
    QLabel *m_lblTitle;
    QPropertyAnimation *m_breathAnim; // 呼吸动画
};
