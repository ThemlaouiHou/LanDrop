#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QStatusBar>
#include <QProcess>
#include "../network/receiver.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QMap<QString, int> receivedFileIds;

private slots:
    void updateIpAddress();
    void modifyConfig();
    void about();

private:
    void createMenuBar();
    void returnGetWifiIP();
    QStringList returnGetDynamicARPAddresses();

    QStringList ipAddresses;
    QString getWifiIPoutput;
    QProcess getWifiIPProcess;
    QProcess getDynamicARPAddressesProcess;
    QLabel *ipLabel;
    QLabel *connectionStatusLabel;
    QStatusBar *statusBar;

    QMap<QString, qint64>       pendingFiles;
    QMap<QString, QTcpSocket*>  sockMap;
    QTimer                     *batchTimer;

    Receiver *receiver;
};

#endif // MAINWINDOW_H
