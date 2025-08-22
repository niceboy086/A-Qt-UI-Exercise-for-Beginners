#include "mywidget.h"
#include "./ui_mywidget.h"
#include <QMessageBox>

MyWidget::MyWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MyWidget)
{
    ui->setupUi(this);

    HoverButton *btn3 = findChild<HoverButton*>("pushButton_3"); // 查找名为myButton的按钮

    QPushButton *btn4 = findChild<QPushButton*>("pushButton_4"); // 查找名为myButton的按钮
    btn4->setStyleSheet(
        "QPushButton:hover {"
        "   background-color: #a0f050;"  // 稍深灰色背景
        "   border: 1px solid #0050f0;"  /* 边框不变 */
        "   box-shadow: none;"           /* 无阴影 */

        "}");

    connect(ui->pushButton_1, SIGNAL(clicked()), this, SLOT(Click_pushButton_1()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(Click_pushButton_2()));

}

MyWidget::~MyWidget()
{
    delete ui;
}

void MyWidget::Click_pushButton_1()
{
    int rst = QMessageBox::question(this,
                    "information",
                    "thank you for assignment check! \r\nnow you can: ok exit, cancel cancel exit.",
                    QMessageBox::Ok | QMessageBox::Cancel);

    if(rst == QMessageBox::Ok)
    {
        // 用户点击了OK按钮
        this->close();
    }
}

void MyWidget::Click_pushButton_2()
{
    static int count = 0;
    count++;

    QPushButton *senderBtn = qobject_cast<QPushButton*>(sender());
    if(senderBtn)
    {
        if(count%3 == 1)
            senderBtn->setStyleSheet("background: #f0b0b0;"
                                     "border: 1px solid #5090f0;"); // 给发送信号的按钮上色
        else if(count%3 == 2)
            senderBtn->setStyleSheet("background: #b0f0b0;"
                                     "border: 1px solid #5090f0;"); // 给发送信号的按钮上色
        else
            senderBtn->setStyleSheet(""); // 给发送信号的按钮上色

    }
}
