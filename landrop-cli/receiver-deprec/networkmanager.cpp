#include "networkmanager.h"
#include "transferhandler.h"

#include <QHostAddress>
#include <QNetworkDatagram>
#include <QDebug>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent), udpSocket(new QUdpSocket(this)), tcpServer(new QTcpServer(this)) {

    udpSocket->bind(45454, QUdpSocket::ShareAddress);
    connect(udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onUdpReadyRead);

    tcpServer->listen(QHostAddress::Any, 42424);
    connect(tcpServer, &QTcpServer::newConnection, this, &NetworkManager::onNewConnection);

    qDebug() << "Receiver ready. Listening for incoming files...";
    broadcastPresence();
}

void NetworkManager::broadcastPresence() {
    QByteArray datagram = "LANDROP_DISCOVERY";
    udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, 45454);
}

void NetworkManager::onUdpReadyRead() {
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        qDebug() << "Discovered peer:" << datagram.senderAddress().toString();
        // Optionally connect back
    }
}

void NetworkManager::onNewConnection() {
    clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    qDebug() << "Connected to sender";
}

void NetworkManager::onReadyRead() {
    TransferHandler* handler = new TransferHandler(this);
    handler->receive(clientSocket);
}

void NetworkManager::discoverUsers() {
    // Already handled via broadcastPresence() and UDP
}

void NetworkManager::connectToPeer(const QString& ip) {
    // Not used for receiver
}

void NetworkManager::establishConnection(const QString& ip) {
    // Not used for receiver
}

void NetworkManager::receiveFile() {
    // Handled via incoming TCP connection
}
