/**
 * @file mainwindow.cpp
 */

#include "mainwindow.h"
#include "aboutdialog.h"
#include "batchrequestdialog.h"
#include "userlistwidget.h"
#include "sendfilewidget.h"
#include "transferhistorywidget.h"
#include "transferitemwidget.h"
#include "sharedfileswidget.h"
#include "configdialog.h"
#include "../config/config.h"
#include "../services/sharedfilemanager.h"
#include "../services/broadcastdiscoveryservice.h"

#include <QStatusBar>
#include <QVBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QDir>
#include <QFile>
#include <QTcpSocket>

/**
 * @brief Constructs the main application window.
 *
 * Initializes the LANDrop main window by setting up all core services first,
 * then building the user interface.
 *
 * @param parent Parent widget
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      networkManager(nullptr),
      transferManager(nullptr),
      sharedFileManager(nullptr),
      discoveryService(nullptr)
{
    setupServices();
    setupUI();
}

/**
 * @brief Initializes all core services and establishes their connections.
 */
void MainWindow::setupServices()
{
    networkManager = new NetworkManager(this);
    transferManager = new FileTransferManager(this);
    sharedFileManager = new SharedFileManager(this);
    discoveryService = new BroadcastDiscoveryService(this);
    
    // Connect SharedFileManager directly to DiscoveryService
    discoveryService->setSharedFileManager(sharedFileManager);

    connect(networkManager, &NetworkManager::ipAddressChanged,
            this, &MainWindow::onIPAddressChanged);
    connect(networkManager, &NetworkManager::connectionStatusChanged,
            this, &MainWindow::onNetworkStatusChanged);

    connect(transferManager, &FileTransferManager::batchTransferRequested,
            this, &MainWindow::onBatchTransferRequested);
    connect(transferManager, &FileTransferManager::transferSessionCreated,
            this, &MainWindow::onTransferSessionCreated);
    connect(transferManager, &FileTransferManager::transferProgressUpdated,
            this, &MainWindow::onTransferProgressUpdated);
    connect(transferManager, &FileTransferManager::transferStatusChanged,
            this, &MainWindow::onTransferStatusChanged);

    transferManager->setupReceiver();
    
    // Start watching shared folder
    sharedFileManager->startWatching();
}

/**
 * @brief Sets up the main user interface layout and components.
 */
void MainWindow::setupUI()
{
    // Create main UI components
    auto *central = new QWidget(this);
    auto *split = new QSplitter(Qt::Horizontal, this);
    auto *userList = new UserListWidget(discoveryService, this);
    transferHistoryWidget = new TransferHistoryWidget(this);
    sendFileWidget = new SendFileWidget(transferHistoryWidget, transferManager, this);
    sharedFilesWidget = new SharedFilesWidget(this);

    // Connect user selection to send file widget
    connect(userList, &UserListWidget::userSelected,
            sendFileWidget, &SendFileWidget::setRecipientUser);
    
    // Connect shared files widget directly to discovery service
    connect(discoveryService, &BroadcastDiscoveryService::userListUpdated,
            sharedFilesWidget, &SharedFilesWidget::onUserListUpdated);
    
    // Connect shared file download requests
    connect(sharedFilesWidget, &SharedFilesWidget::downloadRequested,
            this, &MainWindow::onSharedFileDownloadRequested);

    // Configure splitter layout
    split->addWidget(userList);
    split->addWidget(sendFileWidget);
    split->addWidget(sharedFilesWidget);
    split->addWidget(transferHistoryWidget);
    split->setSizes({180, 350, 250, 220});

    auto *lay = new QVBoxLayout;
    lay->addWidget(split);
    central->setLayout(lay);
    setCentralWidget(central);

    setWindowTitle("LANDrop");
    resize(900, 500);

    // Setup status bar with network information
    statusBar = new QStatusBar(this);
    ipLabel = new QLabel(this);
    connectionStatusLabel = new QLabel("Status: Checking...", this);
    connectionStatusLabel->setStyleSheet("color: orange;");
    statusBar->addWidget(ipLabel);
    statusBar->addWidget(connectionStatusLabel);
    setStatusBar(statusBar);

    // Connect widgets to services after both are created
    sendFileWidget->setTransferManager(transferManager);
    sharedFilesWidget->setSharedFileManager(sharedFileManager);

    createMenuBar();
    
    // Start network monitoring after UI is fully set up
    networkManager->startMonitoring();
}

/**
 * @brief Handles IP address changes from the network manager.
 *
 * Updates the IP address display in the status bar when a change is detected in the local IP address.
 *
 * @param newIP The new IP address, or empty string if no connection
 */
void MainWindow::onIPAddressChanged(const QString &newIP)
{
    if (!ipLabel)
        return;
    QString displayText = newIP.isEmpty() ? "NOT FOUND" : newIP;
    ipLabel->setText("IP Wi-Fi: " + displayText);
}

/**
 * @brief Handles network connection status changes.
 *
 * Updates the connection status label based on the current network connectivity state.
 *
 * @param status Current network connection status
 */
void MainWindow::onNetworkStatusChanged(NetworkManager::ConnectionStatus status)
{
    if (!connectionStatusLabel)
        return;

    switch (status)
    {
    case NetworkManager::ConnectionStatus::CONNECTED:
        connectionStatusLabel->setText("Status: Connected");
        connectionStatusLabel->setStyleSheet("color: green;");
        break;
    case NetworkManager::ConnectionStatus::CHECKING:
        connectionStatusLabel->setText("Status: Checking...");
        connectionStatusLabel->setStyleSheet("color: orange;");
        break;
    case NetworkManager::ConnectionStatus::DISCONNECTED:
        connectionStatusLabel->setText("Status: DISCONNECTED");
        connectionStatusLabel->setStyleSheet("color: red;");
        break;
    }
}

/**
 * @brief Handles batch file transfer requests from remote users.
 *
 * Shows a dialog for the user to approve or reject multiple incoming file transfers.
 * For each accepted file, prepares the destination path and sets up the receiver.
 * For rejected files or if the dialog is cancelled, sends rejection responses.
 *
 * @param files Map of file names to their sizes for the batch request
 * @param sockets Map of file names to their corresponding TCP sockets
 */
void MainWindow::onBatchTransferRequested(const QMap<QString, qint64> &files, const QMap<QString, QTcpSocket *> &sockets)
{
    BatchRequestDialog dlg(files, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        auto results = dlg.results();
        for (const QString &fileName : files.keys())
        {
            QTcpSocket *socket = sockets.value(fileName);
            if (results.value(fileName))
            {
                socket->write("OK\n");
                socket->flush();
                QDir dir(Config::getReceivedFilesPath());
                dir.mkpath(".");
                QString fullPath = dir.filePath(fileName);
                QFile *file = new QFile(fullPath);
                if (!file->open(QIODevice::WriteOnly))
                {
                    delete file;
                    socket->write("NO\n");
                    socket->flush();
                    socket->disconnectFromHost();
                }
                else
                {
                    transferManager->getReceiver()->setFile(socket, file);
                }
            }
            else
            {
                socket->write("NO\n");
                socket->flush();
                socket->disconnectFromHost();
            }
        }
    }
    else
    {
        for (const QString &fileName : files.keys())
        {
            QTcpSocket *socket = sockets.value(fileName);
            socket->write("NO\n");
            socket->flush();
            socket->disconnectFromHost();
        }
    }
}

/**
 * @brief Handles creation of new file transfer sessions.
 *
 * Creates a new transfer item widget and adds it to the transfer history
 * when a new file transfer session is initiated.
 *
 * @param sessionId Unique identifier for the transfer session
 * @param fileName Name of the file being transferred
 * @param recipient Recipient information
 */
void MainWindow::onTransferSessionCreated(int sessionId, const QString &fileName, const QString &recipient)
{
    TransferItemWidget::TransferDirection direction =
        (recipient == "Incoming") ? TransferItemWidget::TransferDirection::RECEIVE
                                  : TransferItemWidget::TransferDirection::SEND;

    TransferItemWidget *item = new TransferItemWidget(fileName, direction);
    transferHistoryWidget->addTransferItem(sessionId, item);
}

/**
 * @brief Handles transfer progress updates.
 *
 * Updates the progress display for a specific transfer session.
 *
 * @param sessionId Session identifier for the transfer
 * @param progress Progress percentage
 */
void MainWindow::onTransferProgressUpdated(int sessionId, int progress)
{
    transferHistoryWidget->updateProgress(sessionId, progress);
}

/**
 * @brief Handles transfer status changes.
 *
 * Updates the status display for a specific transfer session.
 *
 * @param sessionId Session identifier for the transfer
 * @param status New transfer status
 */
void MainWindow::onTransferStatusChanged(int sessionId, TransferStatus status)
{
    transferHistoryWidget->setStatus(sessionId, status);
}

/**
 * @brief Creates and configures the application menu bar.
 */
void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);

    QMenu *configMenu = menuBar->addMenu("&Menu");
    QAction *modifyConfigAction = new QAction("&Settings", this);
    connect(modifyConfigAction, &QAction::triggered, this, &MainWindow::modifyConfig);
    QAction *aboutAction = new QAction("&About LANDrop", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
    configMenu->addAction(modifyConfigAction);
    configMenu->addAction(aboutAction);

    setMenuBar(menuBar);
}

/**
 * @brief Opens the configuration dialog and applies any changes.
 *
 * Shows the configuration dialog to allow users to modify application settings
 */
void MainWindow::modifyConfig()
{
    ConfigDialog configDialog(this);
    if (configDialog.exec() == QDialog::Accepted)
    {
        // Save config settings directly
        int oldPort = Config::getPort();
        Config::getReceivedFilesPath() = configDialog.getDownloadPath();
        Config::getPort() = configDialog.getPort();
        Config::getBufferSize() = configDialog.getBufferSize();
        Config::writeToFile();
        
        // Handle port change if needed
        if (oldPort != Config::getPort())
        {
            onPortChanged(Config::getPort());
        }
    }
}

/**
 * @brief Shows the About dialog with application information.
 */
void MainWindow::about()
{
    (new AboutDialog(this))->exec();
}

/**
 * @brief Handles port configuration changes.
 *
 * Restarts the file transfer receiver with the new port and shows a
 * status notification to inform the user of the change.
 *
 * @param newPort The new port number to use for file transfers
 */
void MainWindow::onPortChanged(int newPort)
{
    if (transferManager)
    {
        transferManager->restartReceiver();
    }

    // Show notification to user
    if (statusBar)
    {
        statusBar->showMessage(QString("Port changed to %1 - services restarted").arg(newPort), 5000);
    }
}

/**
 * @brief Handles shared file download requests from the shared files widget.
 *
 * Initiates a download of a shared file from a remote user and shows a
 * status message to inform the user of the download request.
 *
 * @param userIP IP address of the user sharing the file
 * @param userPort Port number for the file transfer
 * @param relativePath Relative path of the file on the remote system
 * @param fileName Name of the file to download
 */
void MainWindow::onSharedFileDownloadRequested(const QString &userIP, quint16 userPort, const QString &relativePath, const QString &fileName)
{
    if (transferManager)
    {
        transferManager->downloadSharedFile(userIP, userPort, relativePath, fileName);
        if (statusBar)
        {
            statusBar->showMessage(QString("Requesting download of %1 from %2...").arg(fileName, userIP), 3000);
        }
    }
}

/**
 * @brief Destructor for MainWindow.
 */
MainWindow::~MainWindow()
{
    if (networkManager)
    {
        networkManager->stopMonitoring();
    }
}