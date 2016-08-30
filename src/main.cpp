#include "sbmainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SBMainWindow w;
    w.show();

    return a.exec();
}
