#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("Robin Gingras");
    QApplication::setOrganizationDomain("robingingras.com");
    QApplication::setApplicationName("Battery Pack Analyzer");

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
