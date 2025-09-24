#include "transferhandler.h"
#include <QDebug>

TransferHandler::TransferHandler(QObject* parent)
    : QObject(parent), bytesTransferred(0), transferComplete(false) {
        file = new QFile();
    }

void TransferHandler::receive(QTcpSocket* sock) {
    socket = sock;

    bool readyReadConnected = connect(socket, &QTcpSocket::readyRead, this, &TransferHandler::onReadyRead);
    bool disconnectedConnected = connect(socket, &QTcpSocket::disconnected, this, &TransferHandler::onDisconnected);

    if (!readyReadConnected || !disconnectedConnected) {
        qWarning() << "Failed to connect signals to slots.";
    } else {
        qDebug() << "Signals connected successfully.";
    }

    qDebug() << "Socket connected. Waiting for data...";
}

void TransferHandler::onReadyRead() {
    // Read the file name first
    if (!file->isOpen()) {
        QByteArray fileNameData = socket->readLine().trimmed(); // Read the first line as the file name
        if (fileNameData.isEmpty()) {
            qWarning() << "Failed to receive file name.";
            socket->disconnectFromHost(); // Disconnect only on error
            return;
        }

        QString fileName = QString::fromUtf8(fileNameData); // Convert to QString
        qDebug() << "File name received:" << fileName;

        file->setFileName(fileName);
        if (!file->open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to open file for writing. Check file permissions or path.";
            socket->disconnectFromHost(); // Disconnect only on error
            return;
        }

        qDebug() << "File opened successfully for writing.";
    }

    // Read the file content
    QByteArray data = socket->readAll();
    if (data.isEmpty()) {
        qWarning() << "No data received from the sender.";
        return; // Do not disconnect, just wait for more data
    } else {
        bytesTransferred += data.size();
        if (file->write(QByteArray::fromHex(data)) == -1) { // Write the data to the file
            qWarning() << "Failed to write data to the file.";
            socket->disconnectFromHost(); // Disconnect on write error
            return;
        }

        qDebug() << "Data received. Size:" << data.size();
        qDebug() << "Total bytes received:" << bytesTransferred;

        // Check if the transfer is complete (optional logic, depending on protocol)
        if (socket->bytesAvailable() == 0) {
            qDebug() << "All data received. Closing connection.";
            socket->disconnectFromHost(); // Explicitly disconnect after transfer is complete
        }
    }
}

void TransferHandler::onDisconnected() {
    if (file->isOpen()) {
        file->close();
    }

    if (bytesTransferred > 0) {
        transferComplete = true;
        qDebug() << "Transfer complete. Total bytes received:" << bytesTransferred;
        socket->delete(); // Clean up the socket
    } else {
        qWarning() << "Transfer failed or no data received.";
        socket->deleteLater(); 
    }
    socket->deleteLater(); 
    
}
