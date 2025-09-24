#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
    static QString receivedFilesPath;
    static QString settingsPath;
    static int port;
    static int bufferSize;
    static QString buttonStyleSheet;
    static QString bigButtonStyleSheet;
    static QString disabledButtonStyleSheet;

    static void reset();
    static void readFromFile();
    static void writeToFile();
};

#endif // CONFIG_H
