/**
 * @file sharedfilemanager.h
 * @brief File system monitoring service for shared folder management
 */

#ifndef SHAREDFILEMANAGER_H
#define SHAREDFILEMANAGER_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>

/**
 * @class SharedFileManager
 * @brief Manages the shared files folder and monitors for changes.
 *
 * This service monitors the designated shared files directory using
 * QFileSystemWatcher and provides JSON representations of available files
 * for network discovery.
 */
class SharedFileManager : public QObject
{
    Q_OBJECT

public:
    explicit SharedFileManager(QObject *parent = nullptr);
    ~SharedFileManager();

    void setSharedFolderPath(const QString &path);
    QString getSharedFolderPath() const { return sharedFolderPath; }

public slots:
    void startWatching();
    void stopWatching();
    void refreshFileList();

signals:
    /** Signal emitted when shared files are added, removed, or modified. */
    void sharedFilesChanged();

private slots:
    void onDirectoryChanged(const QString &path);
    void onFileChanged(const QString &path);
    void scanDelayed();

private:
    /** File system watcher for monitoring the shared directory */
    QFileSystemWatcher *fileWatcher;

    /** Timer for delayed file system scanning to avoid excessive updates */
    QTimer *scanTimer;

    /** Path to the shared files directory */
    QString sharedFolderPath;

    /** Delay in milliseconds before scanning after file system events */
    static const int SCAN_DELAY_MS = 1000;
};

#endif // SHAREDFILEMANAGER_H