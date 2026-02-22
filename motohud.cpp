// #include <QApplication>
// #include <QLabel>
// #include <QWidget>
// #include <QVBoxLayout>
// #include <QFont>

// int main(int argc, char *argv[])
// {
//     QApplication app(argc, argv);

//     QWidget window;
//     window.setWindowTitle("Moto HUD");
//     window.resize(800, 480);   // typical 5" HDMI display resolution

//     auto *layout = new QVBoxLayout(&window);

//     auto *label = new QLabel("0");
//     label->setAlignment(Qt::AlignCenter);

//     QFont font;
//     font.setPointSize(96);
//     font.setBold(true);
//     label->setFont(font);

//     layout->addWidget(label);

//     window.show();
//     return app.exec();
// }

#include <QApplication>
#include "main_window.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    MainWindow w;
    // w.showFullScreen();   
    w.resize(800,480);
    w.show();

    return app.exec();
}