/**
 * @file filetransfermanager.h
 * @brief High-level file transfer coordination service for LANDrop
 */

#ifndef FILETRANSFERMANAGER_H
#define FILETRANSFERMANAGER_H

#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QFile>
#include <QMap>
#include "../network/sender.h"
#include "../network/receiver.h"
#include "../core/transferstatus.h"
#include "broadcastdiscoveryservice.h"

/**
 * @brief Structure representing an incoming file transfer request.
 *
 * Contains the metadata for a file transfer request that requires
 * user confirmation before acceptance.
 */
struct TransferRequest
{
    /** Name of the file being offered */
    QString fileName;

    /** Size of the file in bytes */
    qint64 fileSize;

    /** TCP socket connection for this transfer */
    QTcpSocket *socket;
};

/**
 * @brief Structure representing an active file transfer session.
 *
 * Tracks the complete state of an outgoing file transfer.
 */
struct TransferSession
{
    /** Unique session identifier */
    int id;

    /** Name of the file being transferred */
    QString fileName;

    /** IP address of the recipient */
    QString recipientIP;

    /** Current transfer status */
    TransferStatus status;

    /** Transfer progress percentage */
    int progress;

    /** Sender object handling this transfer */
    Sender *sender;

    TransferSession() : id(-1), status(TransferStatus::WAITING),
                        progress(0), sender(nullptr) {}
};

/**
 * @class FileTransferManager
 * @brief Central coordination service for all file transfer operations.
 *
 * This class manages both sending and receiving file transfers, providing
 * a unified interface for the application. It coordinates between Sender
 * and Receiver objects, manages transfer sessions, handles batch operations,
 * and provides progress tracking for the UI.
 */
class FileTransferManager : public QObject
{
    Q_OBJECT

public:
    explicit FileTransferManager(QObject *parent = nullptr);
    ~FileTransferManager();

    void setupReceiver();
    void restartReceiver();
    void sendFilesToUsers(const QStringList &filePaths, const QList<LANDropUser> &recipients);
    void downloadSharedFile(const QString &userIP, quint16 userPort, const QString &relativePath, const QString &fileName);
    Receiver *getReceiver() const { return receiver; }

signals:
    /**
     * @brief Signal emitted when a new transfer session is created.
     * @param sessionId Unique identifier for the session.
     * @param fileName Name of the file being transferred.
     * @param recipient IP address of the recipient.
     */
    void transferSessionCreated(int sessionId, const QString &fileName, const QString &recipient);

    /**
     * @brief Signal emitted when transfer progress is updated.
     * @param sessionId Session identifier.
     * @param progress Transfer progress percentage.
     */
    void transferProgressUpdated(int sessionId, int progress);

    /**
     * @brief Signal emitted when transfer status changes.
     * @param sessionId Session identifier.
     * @param status New transfer status.
     */
    void transferStatusChanged(int sessionId, TransferStatus status);

    /**
     * @brief Signal emitted when multiple file transfers are requested.
     * @param files Map of file names to file sizes.
     * @param sockets Map of file names to TCP sockets.
     */
    void batchTransferRequested(const QMap<QString, qint64> &files,
                                const QMap<QString, QTcpSocket *> &sockets);

private slots:
    void onSenderTransferAccepted();
    void onSenderTransferRefused();
    void onSenderProgressUpdated(int progress);
    void onSenderTransferFinished();
    void onSenderTransferError();
    void onReceiverFileTransferRequested(const QString &fileName, const QString &fileSize, QTcpSocket *socket);
    void onReceiverProgressUpdated(const QString &fileName, int progress);
    void onReceiverStatusUpdated(const QString &fileName, TransferStatus status);
    void onReceiverFileReceived(const QString &fileName);

private:
    int createTransferSession(const QString &fileName, const QString &recipientIP);
    void updateSessionStatus(int sessionId, TransferStatus status);
    void updateSessionProgress(int sessionId, int progress);

    /** Receiver object for handling incoming transfers */
    Receiver *receiver;

    /** Map of active transfer sessions indexed by session ID */
    QMap<int, TransferSession> sessions;

    /** Map linking sender objects to their session IDs */
    QMap<Sender *, int> senderToSession;

    /** Map linking received file names to session IDs */
    QMap<QString, int> receivedFileToSession;

    /** Timer for batching multiple transfer requests */
    QTimer *batchTimer;

    /** Temporary storage for pending batch files */
    QMap<QString, qint64> pendingBatchFiles;

    /** Temporary storage for pending batch sockets */
    QMap<QString, QTcpSocket *> pendingBatchSockets;

    /** Counter for generating unique session IDs */
    int nextSessionId;
};

#endif // FILETRANSFERMANAGER_H