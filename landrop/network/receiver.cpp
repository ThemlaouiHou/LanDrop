/**
 * @file receiver.cpp
 */
#include "receiver.h"
#include "../ui/mainwindow.h"
#include <QDebug>

/**
 * @brief Constructs a new Receiver instance
 * @param parent Parent QObject
 * 
 * Initializes the TCP server and sets up the new connection handler.
 */
Receiver::Receiver(QObject *parent)
    : QObject(parent),
    server(new QTcpServer(this)),
    directory(new QDir())
{
    connect(server, &QTcpServer::newConnection, this, &Receiver::onNewConnection);
}

/**
 * @brief Destructor that cleans up the QDir instance
 */
Receiver::~Receiver() {
    delete directory;
}

/**
 * @brief Starts the TCP server on the specified port
 * @param port Port number to bind to (0 for automatic assignment)
 * @return true if server started successfully, false otherwise
 */
bool Receiver::startServer(quint16 port) {
    return server->listen(QHostAddress::Any, port);
}


/**
 * @brief Handles new TCP client connections.
 * Sets up signal connections.
 */
void Receiver::onNewConnection() {
    QTcpSocket *clientSocket = server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &Receiver::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Receiver::onDisconnected);
}

/**
 * @brief Handles incoming data from connected clients
 * 
 * This method implements a two-phase protocol:
 * Phase 1 - Metadata Reception.
 * Phase 2 - File Data Reception.
 */
void Receiver::onReadyRead() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    // Phase 1: Parse metadata (filename|filesize format)
    if (!pendingFiles.contains(clientSocket)) {
        QList<QByteArray> data = clientSocket->readLine().trimmed().split('|');
        QString fileName = data[0];
        qint64 fileSize = data[1].toLongLong();
        pendingFiles[clientSocket].name = fileName;
        pendingFiles[clientSocket].size = fileSize;
        emit fileTransferRequested(fileName, QString::number(fileSize), clientSocket);
    } 
    // Phase 2: Receive and write file data
    else {
        QFile *file = pendingFiles[clientSocket].file;
        QByteArray data = clientSocket->readAll();
        pendingFiles[clientSocket].totalRecieved += data.size();
        
        // Calculate transfer progress percentage
        float percentage = ((float)pendingFiles[clientSocket].totalRecieved / (float)pendingFiles[clientSocket].size) * 100;
        
        // Write data to target file if available
        if (file && file->isOpen()) {
            file->write(data);
            file->flush();
        }

        emit transferProgressUpdated(pendingFiles[clientSocket].name, static_cast<int>(percentage));
    }
}

/**
 * @brief Handles client disconnection and transfer completion/cleanup
 * 
 * Transfer Status Logic:
 * - If totalReceived < fileSize: Transfer was CANCELLED/interrupted
 * - If totalReceived >= fileSize: Transfer FINISHED successfully
 */
void Receiver::onDisconnected() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    if (pendingFiles.contains(clientSocket)) {
        QFile *file = pendingFiles[clientSocket].file;
        QString fileName = pendingFiles[clientSocket].name;
        
        // Close target file if open
        if (file && file->isOpen()) file->close();
        
        // Determine transfer outcome and emit signals
        if (pendingFiles[clientSocket].totalRecieved < pendingFiles[clientSocket].size) {
            emit transferStatusUpdated(fileName, TransferHistoryWidget::TransferStatus::CANCELLED);
        } else {
            emit fileReceivedSuccessfully(QFileInfo(fileName).fileName());
            emit transferStatusUpdated(fileName, TransferHistoryWidget::TransferStatus::FINISHED);
        }
        
        // Clean up resources
        delete file;
        pendingFiles.remove(clientSocket);
    }

    // Schedule socket for deletion
    clientSocket->deleteLater();
}

/**
 * @brief Associates a target file with an incoming file transfer
 * @param socket The client socket requesting the file transfer
 * @param file The target file where received data will be written
 */
void Receiver::setFile(QTcpSocket *socket, QFile *file) {
    pendingFiles[socket].file = file;
}
