#include "widget.h"
#include <QApplication>
#include <QStyleFactory>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("fusion"));
    Widget w;
    //w.setWindowIcon(QIcon(":/image/icon.png"));
    w.setWindowTitle(QObject::tr("串口助手V1.7"));
    w.show();

    return a.exec();
}
