#ifndef HOVERBUTTON_H
#define HOVERBUTTON_H

#include <QWidget>
#include <QPushButton>
#include <QEvent>
#include <QEnterEvent>
#include <QColor>
#include <QPainter>


class HoverButton : public QPushButton
{
    Q_OBJECT
public:
    explicit HoverButton(QWidget *parent = nullptr);


protected:
    // 鼠标进入按钮区域（hover开始）
    void enterEvent(QEnterEvent *event) override;

    // 鼠标离开按钮区域（hover结束）
    void leaveEvent(QEvent *event) override;
    // 重写绘制事件
    void paintEvent(QPaintEvent *event) override;
    // 重写鼠标按下事件
    void mousePressEvent(QMouseEvent *event) override;
    // 重写鼠标释放事件
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // 按钮状态
    enum ButtonState {
              Normal,
              Hover,
              Pressed
            };

    ButtonState m_currentState;  // 当前状态

    // 不同状态的颜色
    QColor m_normalColor;
    QColor m_hoverColor;
    QColor m_pressedColor;
    QColor m_textColor;

    // QT 继承自QPushButton 的自绘按钮
};

#endif // HOVERBUTTON_H
