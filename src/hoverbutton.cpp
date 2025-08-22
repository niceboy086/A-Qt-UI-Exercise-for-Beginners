#include "hoverbutton.h"
#include <QFont>
#include <QPalette>

HoverButton::HoverButton(QWidget *parent)
    : QPushButton(parent),
      m_currentState(Normal),
      m_normalColor(QColor(70, 130, 180)),      // 钢蓝色
      m_hoverColor(QColor("#e1e1e1")),      // CornflowerBlue
      m_pressedColor(QColor(25, 25, 112)),      // 午夜蓝
      m_textColor(QColor(20, 170, 20))
{
    //setStyleSheet(
    //    "QPushButton:hover {"
    //    "   font-weight: bold;"
    //    "   font-size: 13px;"
    //    "   color: #00f000;"
    //    "   border: 1px solid #adadad;"  /* 边框不变 */
    //    "   box-shadow: none;"           /* 无阴影 */
    //    "   background-color: #f0f0f0;"  // 稍深灰色背景
    //
    //    "}");
    // 设置鼠标跟踪，以便检测悬停状态
    setMouseTracking(true);

}

void HoverButton::enterEvent(QEnterEvent *event)
{
    m_currentState = Hover;
    update();  // 触发重绘
    QPushButton::enterEvent(event); // 调用父类事件处理
    return;

    // 更改样式
    QFont currentFont = font();
    currentFont.setBold(true);
    currentFont.setPointSize(14);
    setFont(currentFont);
    // setStyleSheet("color: #00f000; background: #f0f0f0; ");
    QPalette curpalette = palette();

    // 设置字体颜色（WindowText角色控制文本颜色）
    curpalette.setColor(QPalette::WindowText, Qt::red); // 红色（Qt预定义颜色）
    // 也可以用RGB值：palette.setColor(QPalette::WindowText, QColor(255, 0, 0));

    // 应用修改后的调色板
    setPalette(curpalette);
    // 可以添加其他逻辑，如播放动画、发送信号等
}

// 鼠标离开按钮区域（hover结束）
void HoverButton::leaveEvent(QEvent *event)
{
    m_currentState = Normal;
    update();  // 触发重绘
    // 恢复样式
    // 可以添加其他逻辑，如停止动画等
    QPushButton::leaveEvent(event); // 调用父类事件处理
}
void HoverButton::mousePressEvent(QMouseEvent *event)
{
    //if (event->button() == Qt::LeftButton) {
        m_currentState = Pressed;
        update();  // 触发重绘
    //}
    QPushButton::mousePressEvent(event);
}

void HoverButton::mouseReleaseEvent(QMouseEvent *event)
{
    m_currentState = Hover;  // 释放后回到悬停状态（如果鼠标还在按钮上）
    update();  // 触发重绘
    QPushButton::mouseReleaseEvent(event);
}

void HoverButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if((m_currentState != Hover) && (m_currentState != Pressed))
    {
        QPushButton::paintEvent(event);
        return;
    }

    QPainter painter(this);
    //painter.setRenderHint(QPainter::Antialiasing, true);  // 抗锯齿

    // 根据当前状态选择颜色
    QColor bgColor = m_hoverColor;

    // 绘制按钮背景
    //painter.setBrush(Qt::NoBrush);
    //painter.setPen(QPen(QColor(247,247,247), 1, Qt::SolidLine));  // 无边框
    QRect rc =rect();
    //painter.drawRect(rc);

    painter.setBrush(bgColor);
    painter.setPen(QPen(QColor(170,174,170), 1, Qt::SolidLine));
    rc.adjust(1, 1, -2, -2);
    painter.drawRect(rc);

    QFont curfont = painter.font();
    curfont.setBold(false);
    curfont.setPointSize(10);
    painter.setFont(curfont);
    // 绘制按钮文本
    painter.setPen(m_textColor);
    painter.drawText(rc, Qt::AlignCenter, text());
}

