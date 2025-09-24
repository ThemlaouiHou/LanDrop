/**
 * @file sender.cpp
 */
#include "sender.h"
#include <QFileInfo>
#include <QDebug>
#include <QTimer>

/**
 * @brief Constructs a new Sender instance
 * @param parent Parent QObject
 */
Sender::Sender(QObject *parent)
    : QObject(parent), socket(nullptr), file(nullptr), bytesSent(0)
{}

/**
 * @brief Destructor with proper cleanup
 */
Sender::~Sender()
{
    reset();
}

/**
 * @brief Resets the sender state and cleans up resources
 */
void Sender::reset()
{
    if (socket) {
        socket->disconnectFromHost();
        socket->deleteLater();
        socket = nullptr;
    }
    if (file) {
        if (file->isOpen()) file->close();
        delete file;
        file = nullptr;
    }
}

/**
 * @brief Initiates a file transfer to the specified receiver
 * @param filePath Path to the file to send
 * @param receiverIP IP address of the receiving LANDrop instance
 * 
 * If the file doesn't exist, the method returns early without
 * emitting any signals.
 */
void Sender::sendFile(const QString &filePath, const QString &receiverIP)
{
    reset();

    file = new QFile(filePath);
    if (!file->exists()) {
        return;
    }

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Sender::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &Sender::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Sender::reset);

    socket->connectToHost(QHostAddress(receiverIP), port);
}


/**
 * @brief Handles successful connection to receiver
 * Sends the LANDrop protocol metadata in the format:
 * "filename|filesize\n"
 */
void Sender::onConnected() {
    QByteArray filename = QFileInfo(file->fileName()).fileName().toLocal8Bit();
    QByteArray filesize = QByteArray::number(file->size());
    socket->write(filename + "|" + filesize + "\n");
    socket->flush();
}

/**
 * @brief Handles receiver's response and manages file transfer
 * 
 * Protocol Response Handling:
 * - "OK": Receiver accepts transfer → Start sending file data
 * - "NO": Receiver refuses transfer → Emit transferRefused signal
 * - Other: Invalid response → Emit transferError signal
 */
void Sender::onReadyRead()
{
    QByteArray response = socket->readLine().trimmed();

    if (response == "OK") {
        emit transferAccepted();

        // Open file for reading
        if (!file->open(QIODevice::ReadOnly)) {
            reset();
            return;
        }

        bytesSent = 0;

        // Set up chunked transfer continuation
        connect(socket, &QTcpSocket::bytesWritten, this, [=](qint64) {
            if (!file || !file->isOpen()) return;

            if (!file->atEnd()) {
                // Send next chunk and update progress
                QByteArray chunk = file->read(bufferSize);
                bytesSent += chunk.size();
                emit progressUpdated(static_cast<int>(bytesSent * 100 / file->size()));
                socket->write(chunk);
            } else {
                // Transfer complete
                emit transferFinished();
                file->close();
                socket->disconnectFromHost();
            }
        });

        // Set up error handler for socket errors
        connect(socket, &QAbstractSocket::errorOccurred, this, [=](){
            emit transferError();
        });

        // Send first chunk to initiate transfer
        QByteArray chunk = file->read(bufferSize);
        bytesSent += chunk.size();
        if (socket->write(chunk) < 1){
            emit transferError();
            return;
        }
    } else if (response == "NO") {
        // Receiver refused the transfer
        socket->disconnectFromHost();
        emit transferRefused();
    }
    else {
        // Invalid response or protocol error
        socket->disconnectFromHost();
        emit transferError();
    }
}

