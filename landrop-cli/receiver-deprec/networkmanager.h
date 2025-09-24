#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    void discoverUsers();
    void broadcastPresence();
    void connectToPeer(const QString& ip);
    void establishConnection(const QString& ip);
    void receiveFile();

private slots:
    void onUdpReadyRead();
    void onNewConnection();
    void onReadyRead();

private:
    QUdpSocket* udpSocket;
    QTcpServer* tcpServer;
    QTcpSocket* clientSocket;
};
