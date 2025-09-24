/**
 * @file sharedfilemanager.cpp
 */

#include "sharedfilemanager.h"
#include "../config/config.h"
#include <QJsonDocument>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDirIterator>

/**
 * @brief Constructs a new SharedFileManager.
 *
 * Initializes the file system watcher and scan timer for monitoring shared files.
 *
 * @param parent Parent QObject for memory management
 */
SharedFileManager::SharedFileManager(QObject *parent)
    : QObject(parent),
      fileWatcher(new QFileSystemWatcher(this)),
      scanTimer(new QTimer(this))
{
    // Set shared folder path - use safe default if config is invalid
    QString configPath = Config::getSharedFolderPath();
    if (configPath.isEmpty() || configPath == "?" || configPath.isNull())
    {
        configPath = "./Shared Files";
    }
    QString absoluteSharedPath = QDir::current().absoluteFilePath(configPath);
    setSharedFolderPath(absoluteSharedPath);

    // Setup scan timer (debounce rapid changes)
    scanTimer->setSingleShot(true);
    connect(scanTimer, &QTimer::timeout, this, &SharedFileManager::refreshFileList);

    // Connect file watcher signals
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &SharedFileManager::onDirectoryChanged);
    connect(fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &SharedFileManager::onFileChanged);
}

/**
 * @brief Destructor for SharedFileManager.
 *
 * Proper cleanup by stopping file system monitoring.
 */
SharedFileManager::~SharedFileManager()
{
    stopWatching();
}

/**
 * @brief Sets the shared folder path and updates monitoring.
 *
 * Changes the directory being monitored for shared files.
 *
 * @param path New absolute path to the shared files directory
 */
void SharedFileManager::setSharedFolderPath(const QString &path)
{
    if (sharedFolderPath == path)
        return;

    stopWatching();
    sharedFolderPath = path;

    // Create shared folder if it doesn't exist
    QDir().mkpath(sharedFolderPath);

    refreshFileList();
}

/**
 * @brief Starts monitoring the shared files directory for changes.
 *
 * Adds the shared folder and all its subdirectories to the file system watcher.
 */
void SharedFileManager::startWatching()
{
    if (sharedFolderPath.isEmpty())
        return;

    // Add shared folder to watcher
    if (!fileWatcher->directories().contains(sharedFolderPath))
    {
        fileWatcher->addPath(sharedFolderPath);
    }

    // Add all subdirectories to watcher
    QDirIterator it(sharedFolderPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString dirPath = it.next();
        if (!fileWatcher->directories().contains(dirPath))
        {
            fileWatcher->addPath(dirPath);
        }
    }

    refreshFileList();
}

/**
 * @brief Stops monitoring all directories and files.
 *
 * Removes all paths from the file system watcher to cease monitoring.
 */
void SharedFileManager::stopWatching()
{
    if (!fileWatcher->directories().isEmpty())
    {
        fileWatcher->removePaths(fileWatcher->directories());
    }
    if (!fileWatcher->files().isEmpty())
    {
        fileWatcher->removePaths(fileWatcher->files());
    }
}

/**
 * @brief Refreshes the shared file list and notifies listeners of changes.
 */
void SharedFileManager::refreshFileList()
{
    // qDebug() << "SharedFileManager: refreshFileList()";

    // Ensure we have a valid path
    QString safePath = sharedFolderPath;
    if (safePath.isEmpty() || safePath == "?" || safePath.isNull())
    {
        safePath = QDir::current().absoluteFilePath("./Shared Files");
    }

    // Ensure the directory exists
    if (!QDir().mkpath(safePath))
    {
        emit sharedFilesChanged();
        return;
    }

    QDir sharedDir(safePath);
    if (!sharedDir.exists())
    {
        emit sharedFilesChanged();
        return;
    }

    emit sharedFilesChanged();
}

/**
 * @brief Handles directory change notifications from the file system watcher.
 *
 * Called when the watched shared directory or its subdirectories are modified.
 *
 * @param path Path of the directory that changed.
 */
void SharedFileManager::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    scanDelayed();
}

/**
 * @brief Handles file change notifications from the file system watcher.
 *
 * Called when individual files within the watched directories are modified.
 *
 * @param path Path of the file that changed.
 */
void SharedFileManager::onFileChanged(const QString &path)
{
    Q_UNUSED(path)
    scanDelayed();
}

/**
 * @brief Initiates a delayed file system scan to debounce rapid changes.
 *
 * Restarts the scan timer to prevent excessive file list refreshes.
 */
void SharedFileManager::scanDelayed()
{
    scanTimer->start(SCAN_DELAY_MS);
}