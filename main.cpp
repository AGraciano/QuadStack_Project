#include "quadstackapp.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QuadStackApp w;
    w.show();
	w.resize(1280, 720);
    return a.exec();
}
