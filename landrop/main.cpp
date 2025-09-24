#include <QApplication>
#include "ui/mainwindow.h"

int main(int argc, char *argv[]) {
    Config::readFromFile();
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    app.setWindowIcon(QIcon(":/resources/image.ico"));
    return app.exec();
}
