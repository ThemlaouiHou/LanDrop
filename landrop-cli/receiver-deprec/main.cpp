#include <QCoreApplication>
#include "networkmanager.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    NetworkManager manager;

    return app.exec();
}
