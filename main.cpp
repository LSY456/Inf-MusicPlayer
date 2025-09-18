#include "player.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFont>

int main(int argc, char *argv[])
{


    QApplication a(argc, argv);


    Player w;
    w.show();
    return a.exec();
} 
