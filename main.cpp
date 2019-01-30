#include "steganograph.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Steganograph w;
    w.show();

    return a.exec();
}
