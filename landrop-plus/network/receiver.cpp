/**
 * @file receiver.cpp
 */

#include "receiver.h"
#include "sender.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QTimer>

/**
 * @brief Constructs a new Receiver instance.
 *
 * Initializes the TCP server and directory handler, then connects the
 * newConnection signal to handle incoming client connections.
 *
 * @param parent Parent QObject
 */
Receiver::Receiver(QObject *parent)
    : QObject(parent),
      server(new QTcpServer(this)),
      directory(new QDir())
{
    connect(server, &QTcpServer::newConnection, this, &Receiver::onNewConnection);
}

/**
 * @brief Destructor for Receiver, clean up  directory.
 */
Receiver::~Receiver()
{
    delete directory;
}

/**
 * @brief Starts the TCP server to listen for incoming file transfers.
 *
 * This method implements multi-instance support by automatically finding
 * an available port if the requested port is in use.
 * @param port The desired port number (0 means use Config::getPort()).
 * @return true if server started successfully, false otherwise.
 */
bool Receiver::startServer(quint16 port)
{
    quint16 targetPort = port;
    
    if (targetPort == 0) {
        targetPort = Config::getPort();
    }
    
    // Try to start server on target port
    if (!server->listen(QHostAddress::Any, targetPort)) {
        if (port != 0 && !server->listen(QHostAddress::Any, 0)) {
            return false;
        }
    }
    
    // Update config
    Config::getPort() = server->serverPort();
    return true;
}

/**
 * @brief Retrieves the actual port number the server is listening on.
 *
 * @return The port number currently used by the TCP server
 */
quint16 Receiver::getServerPort() const
{
    return server->serverPort();
}

/**
 * @brief Handles new TCP client connections.
 *
 * Called when a new client connects to the server. Sets up signal connections
 * for handling incoming data and client disconnections.
 */
void Receiver::onNewConnection()
{
    QTcpSocket *clientSocket = server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &Receiver::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Receiver::onDisconnected);
}

/**
 * @brief Processes incoming data from connected clients.
 *
 * This method handles the LANDrop protocol for both regular file transfers
 * and download requests.
 *
 * Protocol formats:
 * - Regular transfer: "filename|filesize\n"
 * - Download request: "DOWNLOAD_REQUEST|relativePath|fileName|clientPort\n"
 *
 * @note Emits fileTransferRequested() for new transfers requiring user approval
 * @note Emits transferProgressUpdated() during file reception
 * @note Disconnects clients that send invalid protocol messages
 */
void Receiver::onReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket) return;

    if (!pendingFiles.contains(clientSocket))
    {
        // Handle new connection - read metadata
        QByteArray line = clientSocket->readLine().trimmed();
        if (line.isEmpty() || !line.contains('|'))
        {
            clientSocket->disconnectFromHost();
            return;
        }
        
        QString message = QString::fromUtf8(line);

        // Check if this is a download request
        if (message.startsWith("DOWNLOAD_REQUEST|"))
        {
            QStringList parts = message.split('|');
            if (parts.size() >= 4)
            {
                QString relativePath = parts[1];
                QString fileName = parts[2];
                quint16 clientPort = parts[3].toUShort();
                QString clientIP = clientSocket->peerAddress().toString();
                handleDownloadRequest(clientIP, relativePath, fileName, clientPort);
            }
            clientSocket->disconnectFromHost();
            return;
        }

        // Regular file transfer - parse metadata
        QList<QByteArray> data = line.split('|');
        if (data.size() < 2)
        {
            clientSocket->disconnectFromHost();
            return;
        }

        QString fileName = data[0];
        qint64 fileSize = data[1].toLongLong();

        if (fileName.isEmpty() || fileSize < 0)
        {
            clientSocket->disconnectFromHost();
            return;
        }

        pendingFiles[clientSocket].name = fileName;
        pendingFiles[clientSocket].size = fileSize;
        pendingFiles[clientSocket].totalReceived = 0;
        pendingFiles[clientSocket].file = nullptr;
        emit fileTransferRequested(fileName, QString::number(fileSize), clientSocket);
    }
    else
    {
        // Handle file data transfer
        FileDefinition &fileInfo = pendingFiles[clientSocket];
        QFile *file = fileInfo.file;

        if (!file || !file->isOpen())
        {
            return;
        }

        QByteArray data = clientSocket->readAll();
        if (data.isEmpty()) return;

        qint64 written = file->write(data);
        if (written == -1)
        {
            emit transferStatusUpdated(fileInfo.name, TransferStatus::CANCELLED);
            clientSocket->disconnectFromHost();
            return;
        }

        fileInfo.totalReceived += data.size();
        float percentage = ((float)fileInfo.totalReceived / (float)fileInfo.size) * 100;
        file->flush();

        emit transferProgressUpdated(fileInfo.name, static_cast<int>(percentage));

        // Check if transfer complete
        if (fileInfo.totalReceived >= fileInfo.size)
        {
            clientSocket->disconnectFromHost();
        }
    }
}

/**
 * @brief Handles client disconnection and transfer cleanup.
 *
 * Called when a client disconnects, either normally after transfer completion
 * or abnormally due to network issues.
 */
void Receiver::onDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket) return;

    if (pendingFiles.contains(clientSocket))
    {
        FileDefinition &fileInfo = pendingFiles[clientSocket];
        QFile *file = fileInfo.file;
        QString fileName = fileInfo.name;

        if (file)
        {
            if (file->isOpen()) file->close();
            delete file;
        }

        if (fileInfo.totalReceived < fileInfo.size)
        {
            emit transferStatusUpdated(fileName, TransferStatus::CANCELLED);
        }
        else
        {
            emit fileReceivedSuccessfully(QFileInfo(fileName).fileName());
            emit transferStatusUpdated(fileName, TransferStatus::FINISHED);
        }

        pendingFiles.remove(clientSocket);
    }

    clientSocket->deleteLater();
}

/**
 * @brief Associates a file handle with a client socket for file reception.
 *
 * Called by the UI layer after user accepts a file transfer. Sets up the
 * file handle for writing incoming data and cleans up any existing file
 * associated with the socket.
 *
 * @param socket The client socket requesting file transfer
 * @param file The file handle to write received data to
 */
void Receiver::setFile(QTcpSocket *socket, QFile *file)
{
    if (!socket || !file || !pendingFiles.contains(socket))
    {
        delete file; // Clean up passed file if invalid parameters
        return;
    }

    // Clean up existing file if any
    FileDefinition &fileInfo = pendingFiles[socket];
    if (fileInfo.file)
    {
        if (fileInfo.file->isOpen()) fileInfo.file->close();
        delete fileInfo.file;
    }

    fileInfo.file = file;
}

/**
 * @brief Processes download requests for shared files.
 *
 * When a client requests a shared file, this method validates the file exists
 * in the shared folder and initiates a reverse file transfer using a Sender
 * instance.
 *
 * @param clientIP IP address of the requesting client
 * @param relativePath Relative path of the requested file within shared folder
 * @param fileName Name of the requested file
 * @param clientPort Port number where the client expects to receive the file
 */
void Receiver::handleDownloadRequest(const QString &clientIP, const QString &relativePath, const QString &fileName, quint16 clientPort)
{
    // qDebug() << "Receiver: Handling download request for" << fileName << "to" << clientIP << ":" << clientPort;

    // Build full path to the shared file
    QString sharedDir = Config::getSharedFolderPath();
    QString fullPath = QDir(sharedDir).absoluteFilePath(relativePath);

    // Check if file exists
    QFileInfo fileInfo(fullPath);
    if (!fileInfo.exists() || !fileInfo.isFile())
    {
        // qDebug() << "Receiver: Requested file does not exist:" << fullPath;
        return;
    }

    // qDebug() << "Receiver: File found, sending to" << clientIP << ":" << clientPort;

    // Use the same logic as normal file transfers - create a Sender
    Sender *downloadSender = new Sender(this);

    // Connect Sender signals for proper cleanup (but don't connect to FileTransferManager)
    connect(downloadSender, &Sender::transferFinished, downloadSender, &Sender::deleteLater);
    connect(downloadSender, &Sender::transferError, downloadSender, &Sender::deleteLater);

    // Use normal Sender::sendFile - this will follow the exact same protocol as regular transfers
    downloadSender->sendFile(fullPath, clientIP, clientPort);
}
