/**
 * @file sender.h
 * @brief TCP-based file sender implementation for LANDrop application
 */

#ifndef SENDER_H
#define SENDER_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QHostAddress>
#include <QTimer>

#include "../config/config.h"

/**
 * @class Sender
 * @brief TCP client class responsible for sending files to remote receivers.
 *
 * This class implements the sender side of LANDrop's file transfer protocol.
 * It establishes TCP connections to receivers, sends file metadata, waits for
 * acceptance confirmation, and then transfers the file data in chunks while
 * providing progress updates.
 */
class Sender : public QObject
{
    Q_OBJECT

    /** Number of bytes sent during current transfer.*/
    qint64 bytesSent = 0;

public:
    explicit Sender(QObject *parent = nullptr);
    ~Sender();

    void sendFile(const QString &filePath, const QString &receiverIP, quint16 port);

private slots:
    void onReadyRead();
    void onConnected();

signals:
    /**
     * @brief Signal emitted when transfer progress is updated.
     * @param percent Completion percentage.
     */
    void progressUpdated(int percent);

    /** @brief Signal emitted when receiver accepts the file transfer. */
    void transferAccepted();

    /** @brief Signal emitted when receiver refuses the file transfer. */
    void transferRefused();

    /** @brief Signal emitted when file transfer completes successfully. */
    void transferFinished();

    /** @brief Signal emitted when an error occurs during transfer. */
    void transferError();

private:
    /** TCP socket for connection to receiver. */
    QTcpSocket *socket;

    /** File object for reading data to send. */
    QFile *file;

    /** Port number for connection to receiver. */
    int port;

    /** Timer for connection timeout handling. */
    QTimer *connectionTimer;

    /** Timer for response timeout handling. */
    QTimer *responseTimer;

    void reset();
};

#endif // SENDER_H
