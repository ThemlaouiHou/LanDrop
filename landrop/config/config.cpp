/**
 * @file config.cpp
 */

#include "config.h"
#include <QFile>

// Default configuration values
QString Config::receivedFilesPath = "./Received Files";
QString Config::settingsPath = "./settings.txt";
int Config::port = 5556;
int Config::bufferSize = 65536;

QString Config::buttonStyleSheet ="QPushButton {background-color: black; color: white; border: 1px solid #ffb300; padding: 5px; border-radius: 5px; font-weight: bold;} QPushButton:hover {background-color: #333333;} QPushButton:pressed {background-color: #666666;}";
QString Config::disabledButtonStyleSheet ="QPushButton {background-color: rgba(0, 0, 0, 40%); color: rgba(255, 255, 255, 40%); border: 1px solid rgba(255, 179, 0, 40%); padding: 5px; border-radius: 5px; font-weight: bold;}";
QString Config::bigButtonStyleSheet ="QPushButton {background-color: black; height: 30px; color: white; border: 1px solid #ffb300; padding: 5px; border-radius: 5px; font-weight: bold;} QPushButton:hover {background-color: #333333;} QPushButton:pressed {background-color: #666666;}";

/**
 * @brief Resets all configuration values to their factory defaults.
 */
void Config::reset(){
    receivedFilesPath = "./Received Files";
    settingsPath = "./settings.txt";
    port = 5556;
    bufferSize = 65536;
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
    QFile file(Config::settingsPath);
    if(file.open(QIODevice::WriteOnly|QFile::Truncate))
    {
        file.write(Config::receivedFilesPath.trimmed().toUtf8());
        file.write("\n");
        file.write(QString::number(Config::port).toUtf8());
        file.write("\n");
        file.write(QString::number(Config::bufferSize).toUtf8());
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
    QFile file(Config::settingsPath);
    if(file.exists()){
        if (file.open(QIODevice::ReadOnly))
        {
            try{
                Config::receivedFilesPath = QString(file.readLine().trimmed());
                Config::port = file.readLine().toInt();
                Config::bufferSize = file.readLine().toInt();
            }catch(...){
                Config::reset();
                writeToFile();
            }
        }
    }
    else{
        Config::reset();
        writeToFile();
    }
}
