/**
 * @file config.h
 * @brief Configuration management class for LANDrop application
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

/**
 * @class Config
 * @brief Centralized configuration management for LANDrop application settings.
 * 
 * This class provides static access to application-wide configuration settings
 * including file paths, network ports, and UI styling.
 */
class Config
{
public:
    /**
     * @brief Get path where received files are stored.
     */
    static QString& getReceivedFilesPath();
    
    /**
     * @brief Get path to the shared files folder.
     */
    static QString& getSharedFolderPath();
    
    /**
     * @brief Get path to the configuration settings file.
     */
    static QString& getSettingsPath();
    
    /**
     * @brief Get TCP port number for file transfer operations.
     */
    static int& getPort();
    
    /**
     * @brief Get buffer size in bytes for file transfer operations.
     */
    static int& getBufferSize();
    
    /**
     * @brief Get CSS stylesheet for enabled UI buttons.
     */
    static QString& getButtonStyleSheet();
    
    /**
     * @brief Get CSS stylesheet for disabled UI buttons.
     */
    static QString& getDisabledButtonStyleSheet();

    static void reset();
    static void readFromFile();
    static void writeToFile();
    static int findAvailablePort(int startPort = 5556, int maxAttempts = 50);
};


#endif // CONFIG_H
