#include "player.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFont>

int main(int argc, char *argv[])
{

<<<<<<< HEAD:main.cpp

    QApplication a(argc, argv);

=======
    // 针对统信UOS设置输入法环境变量
    qputenv("QT_IM_MODULE", QByteArray("fcitx"));
    qputenv("GTK_IM_MODULE", QByteArray("fcitx"));
    qputenv("XMODIFIERS", QByteArray("@im=fcitx"));
    QApplication a(argc, argv);

    // 确保系统字体支持中文
    QFontDatabase::addApplicationFont(":/fonts/wqy-microhei.ttc"); // 如果有内置字体
    QFont font;
    font.setFamily("Noto Sans CJK SC"); // 或者使用系统默认中文字体
    a.setFont(font);
>>>>>>> 992db0b9b17aef6f100512fc6081f1cc5bcbdabd:Sources/main.cpp

    Player w;
    w.show();
    return a.exec();
} 
