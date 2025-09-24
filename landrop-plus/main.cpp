/**
 * @file main.cpp
 * @brief Entry point for the LANDrop application
 */

#include <QApplication>
#include "ui/mainwindow.h"
#include "config/config.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <iostream>
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
#ifdef QT_DEBUG
    // Allocate console for debug output on Windows
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
#endif

    // Load configuration from persistent storage
    Config::readFromFile();

    // Create Qt application instance
    QApplication app(argc, argv);
    MainWindow w;
    w.show();

    // Set application icon
    app.setWindowIcon(QIcon(":/resources/image.ico"));

    // Start event loop
    return app.exec();
}