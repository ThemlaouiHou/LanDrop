/**
 * @file mainwindow.h
 * @brief Main application window for LANDrop
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QStatusBar>
#include "../services/networkmanager.h"
#include "../services/filetransfermanager.h"
#include "../core/transferstatus.h"

/**
 * @class MainWindow
 * @brief Central application window that coordinates all LANDrop functionality.
 *
 * The MainWindow is the primary interface for LANDrop, unifying all other widgets into a unified interface.
 * It manages the application's menu system, status display, and coordinates communication
 * between different service components.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void modifyConfig();
    void about();

private slots:
    void onNetworkStatusChanged(NetworkManager::ConnectionStatus status);
    void onIPAddressChanged(const QString &newIP);
    void onBatchTransferRequested(const QMap<QString, qint64> &files, const QMap<QString, QTcpSocket *> &sockets);
    void onTransferSessionCreated(int sessionId, const QString &fileName, const QString &recipient);
    void onTransferProgressUpdated(int sessionId, int progress);
    void onTransferStatusChanged(int sessionId, TransferStatus status);
    void onPortChanged(int newPort);
    void onSharedFileDownloadRequested(const QString &userIP, quint16 userPort, const QString &relativePath, const QString &fileName);

private:
    void createMenuBar();
    void setupServices();
    void setupUI();

    /** Status bar label showing current IP address */
    QLabel *ipLabel;

    /** Status bar label showing network connection status */
    QLabel *connectionStatusLabel;

    /** Main window status bar */
    QStatusBar *statusBar;

    /** Network management service for TCP operations */
    NetworkManager *networkManager;

    /** File transfer coordination service */
    FileTransferManager *transferManager;

    /** Service for managing shared file directory */
    class SharedFileManager *sharedFileManager;

    /** Service for discovering other LANDrop users */
    class BroadcastDiscoveryService *discoveryService;

    /** Widget for selecting and sending files */
    class SendFileWidget *sendFileWidget;

    /** Widget displaying transfer history and progress */
    class TransferHistoryWidget *transferHistoryWidget;

    /** Widget for managing and browsing shared files */
    class SharedFilesWidget *sharedFilesWidget;
};

#endif // MAINWINDOW_H
