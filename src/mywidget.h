#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>
#include "hoverbutton.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MyWidget;
}
QT_END_NAMESPACE

class MyWidget : public QWidget
{
    Q_OBJECT

public:
    MyWidget(QWidget *parent = nullptr);
    ~MyWidget();

public slots:
    void Click_pushButton_1();
    void Click_pushButton_2();

private:
    Ui::MyWidget *ui;
};
#endif // MYWIDGET_H
