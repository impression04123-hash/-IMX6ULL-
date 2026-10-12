#include "smarthomewindow.h"

#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SmartHomeWindow w;
    w.setCursor(Qt::BlankCursor); // 隐藏鼠标指针
    w.show();

    return a.exec();
}
