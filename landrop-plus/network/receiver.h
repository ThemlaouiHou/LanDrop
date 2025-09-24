/**
 * @file receiver.h
 * @brief TCP-based file receiver implementation for LANDrop application
 */

#ifndef RECEIVER_H
#define RECEIVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QMap>
#include "../core/transferstatus.h"
#include "../config/config.h"

/**
 * @brief Structure containing file transfer metadata and state.
 * 
 * This structure tracks the information needed for managing an active
 * file transfer.
 */
typedef struct FileDefinition
{
    /** @brief File handle for writing received data. */
    QFile* file = nullptr;
    
    /** @brief Original name of the file being transferred. */
    QString name;
    
    /** @brief Total size of the file in bytes. */
    qint64 size = 0;
    
    /** @brief Number of bytes received so far. */
    qint64 totalReceived = 0;
} FileDefinition;

/**
 * @class Receiver
 * @brief TCP server class responsible for receiving files from remote senders.
 * 
 * This class implements the receiver side of LANDrop's file transfer protocol.
 * It listens for incoming TCP connections, processes file transfer requests,
 * manages user confirmation dialogs, and handles concurrent file downloads
 * from multiple senders.
 */
class Receiver : public QObject
{
    Q_OBJECT

public:
    explicit Receiver(QObject *parent = nullptr);
    ~Receiver();

    bool startServer(quint16 port = 0);
    void setFile(QTcpSocket *s, QFile *f);
    quint16 getServerPort() const;

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

signals:
    /**
     * @brief Signal emitted when a file transfer is requested by a sender.
     * @param fileName Name of the file being offered for transfer.
     * @param fileSize Size of the file in human-readable format.
     * @param socket TCP socket connection for this transfer.
     */
    void fileTransferRequested(const QString &fileName, const QString &fileSize, QTcpSocket *socket);
    
    /**
     * @brief Signal emitted when a file has been successfully received.
     * @param fileName Name of the completed file.
     */
    void fileReceivedSuccessfully(const QString &fileName);
    
    /**
     * @brief Signal emitted when file transfer progress is updated.
     * @param fileName Name of the file being transferred.
     * @param percent Completion percentage (0-100).
     */
    void transferProgressUpdated(const QString &fileName, int percent);
    
    /**
     * @brief Signal emitted when transfer status changes.
     * @param fileName Name of the file being transferred.
     * @param status New transfer status.
     */
    void transferStatusUpdated(const QString &fileName, TransferStatus status);

private:
    void handleDownloadRequest(const QString &clientIP, const QString &relativePath, const QString &fileName, quint16 clientPort);

    /** TCP server for listening to incoming connections. */
    QTcpServer *server;
    
    /** Directory object for received files path. */
    QDir *directory;
     
    /** Map of active file transfers indexed by socket. */
    QMap<QTcpSocket*, FileDefinition> pendingFiles;
};

#endif // RECEIVER_H
