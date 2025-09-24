/**
 * @file mainwindow.cpp
 * @brief Main application window for LANDrop file transfer application
 */
#include "mainwindow.h"
#include "aboutdialog.h"
#include "batchrequestdialog.h"
#include "userlistwidget.h"
#include "sendfilewidget.h"
#include "transferhistorywidget.h"
#include "transferitemwidget.h"
#include "configdialog.h"
#include "../config/config.h"

#include <QProcess>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QDir>
#include <QFile>
#include <QTcpSocket>

/**
 * @brief Constructs the main application window
 * @param parent Parent widget (typically nullptr for main window)
 * 
 * Initializes the complete UI layout, sets up network components,
 * and establishes signal connections between widgets.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    batchTimer(new QTimer(this))
{
    // --- Base UI Layout Setup ---
    auto *central   = new QWidget(this);
    auto *split     = new QSplitter(Qt::Horizontal, this);
    auto *userList  = new UserListWidget(this);
    auto *history   = new TransferHistoryWidget(this);
    auto *sendFile  = new SendFileWidget(history, this);

    connect(userList, &UserListWidget::ipSelected,
            sendFile, &SendFileWidget::setRecipientAddress);

    split->addWidget(userList);
    split->addWidget(sendFile); 
    split->addWidget(history);   
    split->setSizes({200, 500, 250}); 

    auto *lay = new QVBoxLayout;
    lay->addWidget(split);
    central->setLayout(lay);
    setCentralWidget(central);

    setWindowTitle("LANDrop");
    resize(900, 500);

    // --- Status Bar Setup ---
    statusBar = new QStatusBar(this);
    ipLabel   = new QLabel(this);
    connectionStatusLabel = new QLabel("Status: Checking...", this);
    connectionStatusLabel->setStyleSheet("color: orange;");
    statusBar->addWidget(ipLabel);
    statusBar->addWidget(connectionStatusLabel);
    setStatusBar(statusBar);

    // --- Network Monitoring Setup ---
    updateIpAddress();
    auto *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &MainWindow::updateIpAddress);
    t->start(10000);
    
    createMenuBar();

    // network discovery
    connect(&getWifiIPProcess, &QProcess::finished,
            this, &MainWindow::returnGetWifiIP);
    connect(&getDynamicARPAddressesProcess, &QProcess::finished,
            this, &MainWindow::returnGetDynamicARPAddresses);

    // --- File Reception Server Setup ---
    receiver = new Receiver(this);
    receiver->startServer();

    // --- Configuring batchTimer 
    batchTimer->setSingleShot(true);
    connect(batchTimer, &QTimer::timeout, this, [this]() {
        auto files   = pendingFiles;
        auto sockets = sockMap;
        pendingFiles.clear();
        sockMap.clear();

        BatchRequestDialog dlg(files, this);
        if (dlg.exec() == QDialog::Accepted) {
            auto res = dlg.results();
            for (const QString &fn : files.keys()) {
                QTcpSocket *s = sockets.value(fn);
                if (res.value(fn)) {
                    s->write("OK\n"); s->flush();
                    QDir dir(Config::receivedFilesPath);
                    dir.mkpath(".");
                    QString fullPath = dir.filePath(fn);
                    QFile *file = new QFile(fullPath);
                    if (!file->open(QIODevice::WriteOnly)) {
                        delete file;
                        s->write("NO\n"); s->flush();
                        s->disconnectFromHost();
                    } else {
                        receiver->setFile(s, file);
                    }
                } else {
                    s->write("NO\n"); s->flush();
                    s->disconnectFromHost();
                }
            }
        } else {
            // Refuse all if we cancel
            for (const QString &fn : files.keys()) {
                QTcpSocket *s = sockets.value(fn);
                s->write("NO\n"); s->flush();
                s->disconnectFromHost();
            }
        }
    });

    // --- At each request: we accumulate then restart batchTimer ---
    connect(receiver, &Receiver::fileTransferRequested, this,
            [this, history ](const QString &fileName, const QString &fileSize, QTcpSocket *socket) {

                int newId = history->items.size();
                receivedFileIds[fileName] = newId;

                // Creates a new item and adds it to the history
                TransferItemWidget *item = new TransferItemWidget(fileName, TransferItemWidget::TransferDirection::RECEIVE, this);
                history->addTransferItem(item);
                pendingFiles.insert(fileName, fileSize.toLongLong());
                sockMap[fileName] = socket;
                batchTimer->start(200);
            });

    connect(receiver, &Receiver::transferProgressUpdated, this,
            [this,history](const QString &fileName, int percent) {
                if (receivedFileIds.contains(fileName)) {
                    int id = receivedFileIds[fileName];
                    history->updateProgress(id, percent);
                }
            });

    connect(receiver, &Receiver::transferStatusUpdated, this,
            [this, history](const QString &fileName, TransferHistoryWidget::TransferStatus status) {
                if (receivedFileIds.contains(fileName)) {
                    int id = receivedFileIds[fileName];
                    history->setStatus(id, status);
                }
            });
}

void MainWindow::updateIpAddress() {
    if (!getWifiIPProcess.atEnd() || !getDynamicARPAddressesProcess.atEnd()) return;
#ifdef Q_OS_WIN
    getWifiIPProcess.start("powershell", QStringList() << "-Command"
                                                       << "(Get-NetIPAddress | Where-Object { $_.InterfaceAlias -like '*Wi-Fi*' -and $_.AddressFamily -eq 'IPv4' }).IPAddress");
#else
    getWifiIPProcess.start("bash", QStringList() << "-c"
                                        << "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1");
#endif
}

void MainWindow::returnGetWifiIP(){
#ifdef Q_OS_WIN
    getDynamicARPAddressesProcess.start("cmd", QStringList() << "/c" << "arp -a");
#else
    getDynamicARPAddressesProcess.start("bash", QStringList() << "-c" << "arp -a");
#endif
}

QStringList MainWindow::returnGetDynamicARPAddresses(){
    ipAddresses.clear();
    QString getDynamicARPAddressesoutput = getDynamicARPAddressesProcess.readAllStandardOutput();

    static QRegularExpression regex(R"(\d+\.\d+\.\d+\.\d+)");
    QRegularExpressionMatchIterator i = regex.globalMatch(getDynamicARPAddressesoutput);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        ipAddresses.append(match.captured(0));
    }

    QStringList arpAddresses = ipAddresses;
    getWifiIPoutput = getWifiIPProcess.readAllStandardOutput().trimmed();
    QString wifiIP =  getWifiIPoutput.isEmpty() ? "Not detected" : getWifiIPoutput;

    if (arpAddresses.contains(wifiIP)) {

        ipLabel->setText("Wi-Fi IP: " + wifiIP);
        connectionStatusLabel->setText("Status: Connected");
        connectionStatusLabel->setStyleSheet("color: green;");
    } else {
        ipLabel->setText("Wi-Fi IP: NOT FOUND");
        connectionStatusLabel->setText("Status: DISCONNECTED");
        connectionStatusLabel->setStyleSheet("color: red;");
    }

    return arpAddresses;
}

void MainWindow::createMenuBar() {
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

void MainWindow::modifyConfig() {
    ConfigDialog configDialog(this);
    if (configDialog.exec() == QDialog::Accepted) {
        Config::receivedFilesPath = configDialog.getDownloadPath();
        Config::port = configDialog.getPort();
        Config::bufferSize = configDialog.getBufferSize();
    }
    Config::writeToFile();
}

void MainWindow::about() {
    (new AboutDialog(this))->exec();
}

MainWindow::~MainWindow() {
    receiver->disconnect();
    getWifiIPProcess.waitForFinished();
    getDynamicARPAddressesProcess.waitForFinished();
}
