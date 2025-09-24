/**
 * @file config.cpp
 */

#include "config.h"
#include <QFile>
#include <QTcpServer>
#include <QCoreApplication>

// Thread-safe static accessors with guaranteed initialization order
QString& Config::getReceivedFilesPath() {
    static QString receivedFilesPath = "./Received Files";
    return receivedFilesPath;
}

QString& Config::getSharedFolderPath() {
    static QString sharedFolderPath = "./Shared Files";
    return sharedFolderPath;
}

QString& Config::getSettingsPath() {
    static QString settingsPath = "./settings.txt";
    return settingsPath;
}

int& Config::getPort() {
    static int port = 5556;
    return port;
}

int& Config::getBufferSize() {
    static int bufferSize = 65536;
    return bufferSize;
}

QString& Config::getButtonStyleSheet() {
    static QString buttonStyleSheet = "QPushButton {background-color: black; height: 30px; color: white; border: 1px solid #ffb300; padding: 5px; border-radius: 5px; font-weight: bold;} QPushButton:hover {background-color: #333333;} QPushButton:pressed {background-color: #666666;}";
    return buttonStyleSheet;
}

QString& Config::getDisabledButtonStyleSheet() {
    static QString disabledButtonStyleSheet = "QPushButton {background-color: rgba(0, 0, 0, 40%); color: rgba(255, 255, 255, 40%); border: 1px solid rgba(255, 179, 0, 40%); padding: 5px; border-radius: 5px; font-weight: bold;}";
    return disabledButtonStyleSheet;
}

/**
 * @brief Resets all configuration values to their factory defaults.
 */
void Config::reset(){
    getReceivedFilesPath() = "./Received Files";
    getSharedFolderPath() = "./Shared Files";
    getSettingsPath() = "./settings.txt";
    getPort() = 5556;
    getBufferSize() = 65536;
}

/**
 * @brief Writes current configuration settings to the persistent settings file.
 *
 * Saves the current configuration (received files path, port, and buffer size)
 * to the settings file in a simple text format.
 *
 * @note File format: line 1 = receivedFilesPath, line 2 = port, line 3 = bufferSize
 */
void Config::writeToFile(){
    QFile file(Config::getSettingsPath());
    if(file.open(QIODevice::WriteOnly|QFile::Truncate))
    {
        file.write(Config::getReceivedFilesPath().trimmed().toUtf8());
        file.write("\n");
        file.write(QString::number(Config::getPort()).toUtf8());
        file.write("\n");
        file.write(QString::number(Config::getBufferSize()).toUtf8());
        file.resize(file.pos());
    }
    file.close();
}

/**
 * @brief Loads configuration settings from the persistent settings file.
 *
 * Reads configuration from the settings file if it exists, otherwise creates
 * a new settings file with default values.
 */
void Config::readFromFile(){
    QFile file(Config::getSettingsPath());
    if(file.exists()){
        if (file.open(QIODevice::ReadOnly))
        {
            try{
                QByteArray pathData = file.readLine();
                QByteArray portData = file.readLine();
                QByteArray bufferData = file.readLine();
                
                if(!pathData.isEmpty() && !portData.isEmpty() && !bufferData.isEmpty()){
                    Config::getReceivedFilesPath() = QString(pathData.trimmed());
                    bool portOk = false, bufferOk = false;
                    int newPort = portData.trimmed().toInt(&portOk);
                    int newBuffer = bufferData.trimmed().toInt(&bufferOk);
                    
                    if(portOk && bufferOk && newPort > 0 && newPort < 65536 && newBuffer > 0){
                        Config::getPort() = newPort;
                        Config::getBufferSize() = newBuffer;
                    } else {
                        Config::reset();
                        writeToFile();
                    }
                } else {
                    Config::reset();
                    writeToFile();
                }
            }catch(...){
                Config::reset();
                writeToFile();
            }
            file.close();
        }
    }
    else{
        Config::reset();
        writeToFile();
    }
}

/**
 * @brief Finds the first available TCP port starting from a given port number.
 *
 * @param startPort The first port number to test for availability
 * @param maxAttempts Maximum number of consecutive ports to test
 * @return The first available port number, or -1 if none found within the range
 *
 * @note Used by Receiver class to find available ports for TCP server
 */
int Config::findAvailablePort(int startPort, int maxAttempts)
{
    QTcpServer testServer;
    
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        int testPort = startPort + attempt;
        
        if (testServer.listen(QHostAddress::Any, testPort)) {
            testServer.close();
            return testPort;
        }
    }
    
    return -1;
}
