/**
 * @file filetransfermanager.cpp
 */

#include "filetransfermanager.h"
#include "../config/config.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QTcpSocket>

/**
 * @brief Constructs a new FileTransferManager.
 *
 * Initializes the receiver pointer, batch timer for grouping incoming transfers,
 * and session ID counter.
 *
 * @param parent Parent QObject for memory management
 */
FileTransferManager::FileTransferManager(QObject *parent)
    : QObject(parent),
      receiver(nullptr),
      batchTimer(new QTimer(this)),
      nextSessionId(1)
{
    batchTimer->setSingleShot(true);
    connect(batchTimer, &QTimer::timeout, this, [this]()
            {
        emit batchTransferRequested(pendingBatchFiles, pendingBatchSockets);
        pendingBatchFiles.clear();
        pendingBatchSockets.clear(); });
}

/**
 * @brief Destructor for FileTransferManager.
 *
 * Proper cleanup by disconnecting the receiver and cleaning up all
 * active sender objects and their sessions.
 */
FileTransferManager::~FileTransferManager()
{
    if (receiver)
    {
        receiver->disconnect();
    }

    for (auto it = senderToSession.begin(); it != senderToSession.end(); ++it)
    {
        Sender *sender = it.key();
        if (sender)
        {
            sender->disconnect();
            delete sender;
        }
    }
    senderToSession.clear();
    sessions.clear();
}

/**
 * @brief Sets up the receiver server for incoming file transfers.
 *
 * Creates and configures a new Receiver instance if one doesn't exist.
 * Starts the server on the configured port and connects all receiver signals
 * to the appropriate handler methods. Updates the global port configuration
 * with the actual port being used.
 */
void FileTransferManager::setupReceiver()
{
    if (receiver)
        return;

    receiver = new Receiver(this);

    if (!receiver->startServer())
    {
        // qCritical() << "FileTransferManager: Failed to start receiver server";
        delete receiver;
        receiver = nullptr;
        return;
    }

    // Update Config::getPort() with the actual port being used
    quint16 actualPort = receiver->getServerPort();
    if (Config::getPort() != actualPort)
    {
        Config::getPort() = actualPort;
    }

    connect(receiver, &Receiver::fileTransferRequested,
            this, &FileTransferManager::onReceiverFileTransferRequested);
    connect(receiver, &Receiver::transferProgressUpdated,
            this, &FileTransferManager::onReceiverProgressUpdated);
    connect(receiver, &Receiver::transferStatusUpdated,
            this, &FileTransferManager::onReceiverStatusUpdated);
    connect(receiver, &Receiver::fileReceivedSuccessfully,
            this, &FileTransferManager::onReceiverFileReceived);
}

/**
 * @brief Restarts the receiver server.
 */
void FileTransferManager::restartReceiver()
{
    if (receiver)
    {
        receiver->disconnect();
        delete receiver;
        receiver = nullptr;
    }

    // Re-setup the receiver
    setupReceiver();
}

/**
 * @brief Sends files to multiple recipients.
 *
 * Creates a new transfer session and Sender instance for each file-to-recipient
 * combination.
 *
 * @param filePaths List of file paths to send
 * @param recipients List of users to send files to
 */
void FileTransferManager::sendFilesToUsers(const QStringList &filePaths, const QList<LANDropUser> &recipients)
{
    for (const QString &filePath : filePaths)
    {
        QFileInfo fi(filePath);

        for (const LANDropUser &user : recipients)
        {
            int sessionId = createTransferSession(fi.fileName() + QString(" @%1").arg(user.ipAddress), user.ipAddress);
            TransferSession &session = sessions[sessionId];

            Sender *sender = new Sender(this);
            session.sender = sender;
            senderToSession[sender] = sessionId;

            // Connect all sender signals
            connect(sender, &Sender::transferAccepted, this, &FileTransferManager::onSenderTransferAccepted);
            connect(sender, &Sender::transferRefused, this, &FileTransferManager::onSenderTransferRefused);
            connect(sender, &Sender::progressUpdated, this, &FileTransferManager::onSenderProgressUpdated);
            connect(sender, &Sender::transferFinished, this, &FileTransferManager::onSenderTransferFinished);
            connect(sender, &Sender::transferError, this, &FileTransferManager::onSenderTransferError);

            sender->sendFile(filePath, user.ipAddress, user.transferPort);
        }
    }
}

/**
 * @brief Creates a new transfer session for tracking file transfers.
 *
 * Generates a unique session ID and creates a TransferSession with initial
 * status and progress values.
 *
 * @param fileName Name of the file being transferred
 * @param recipientIP IP address of the recipient
 * @return Unique session ID for tracking this transfer
 */
int FileTransferManager::createTransferSession(const QString &fileName, const QString &recipientIP)
{
    int sessionId = nextSessionId++;
    TransferSession session;
    session.id = sessionId;
    session.fileName = fileName;
    session.recipientIP = recipientIP;
    session.status = TransferStatus::WAITING;
    session.progress = 0;

    sessions[sessionId] = session;
    // Emit signal to notify UI components
    emit transferSessionCreated(sessionId, fileName, recipientIP);

    return sessionId;
}

/**
 * @brief Updates the status of a transfer session.
 *
 * @param sessionId ID of the session to update
 * @param status New transfer status
 */
void FileTransferManager::updateSessionStatus(int sessionId, TransferStatus status)
{
    if (sessions.contains(sessionId))
    {
        sessions[sessionId].status = status;
        emit transferStatusChanged(sessionId, status);
    }
}

/**
 * @brief Updates the progress of a transfer session.
 *
 * @param sessionId ID of the session to update
 * @param progress Transfer progress percentage (0-100)
 */
void FileTransferManager::updateSessionProgress(int sessionId, int progress)
{
    if (sessions.contains(sessionId))
    {
        sessions[sessionId].progress = progress;
        emit transferProgressUpdated(sessionId, progress);
    }
}

/**
 * @brief Handles sender transfer acceptance notification.
 *
 * Updates the corresponding session status to IN_PROGRESS when the
 * recipient accepts the file transfer.
 */
void FileTransferManager::onSenderTransferAccepted()
{
    Sender *sender = qobject_cast<Sender *>(this->sender());
    if (sender && senderToSession.contains(sender))
    {
        int sessionId = senderToSession[sender];
        updateSessionStatus(sessionId, TransferStatus::IN_PROGRESS);
    }
}

/**
 * @brief Handles sender transfer refusal notification.
 *
 * Updates the session status to CANCELLED when the recipient refuses
 * the file transfer.
 */
void FileTransferManager::onSenderTransferRefused()
{
    Sender *sender = qobject_cast<Sender *>(this->sender());
    if (!sender || !senderToSession.contains(sender))
    {
        return;
    }

    int sessionId = senderToSession[sender];
    updateSessionStatus(sessionId, TransferStatus::CANCELLED);

    // Disconnect all signals to prevent double-deletion
    sender->disconnect();

    // Use QPointer for safe deletion
    QPointer<Sender> senderPtr(sender);

    // Delay cleanup to ensure UI updates are processed
    QTimer::singleShot(500, this, [this, senderPtr, sessionId]()
                       {
        if (senderPtr) {
            senderToSession.remove(senderPtr);
            sessions.remove(sessionId);
            delete senderPtr;
        } else {
            // Object was already deleted, just clean up the maps
            sessions.remove(sessionId);
            for (auto it = senderToSession.begin(); it != senderToSession.end(); ++it) {
                if (it.value() == sessionId) {
                    senderToSession.erase(it);
                    break;
                }
            }
        } });
}

/**
 * @brief Handles sender progress update notifications.
 *
 * Updates the corresponding session progress when the sender reports
 * transfer progress changes.
 *
 * @param progress Transfer progress percentage
 */
void FileTransferManager::onSenderProgressUpdated(int progress)
{
    Sender *sender = qobject_cast<Sender *>(this->sender());
    if (sender && senderToSession.contains(sender))
    {
        int sessionId = senderToSession[sender];
        updateSessionProgress(sessionId, progress);
    }
}

/**
 * @brief Handles sender transfer completion notification.
 *
 * Updates the session status to FINISHED when the file transfer completes
 * successfully.
 */
void FileTransferManager::onSenderTransferFinished()
{
    Sender *sender = qobject_cast<Sender *>(this->sender());
    if (!sender || !senderToSession.contains(sender))
    {
        return;
    }

    int sessionId = senderToSession[sender];

    // Verify if the session is still active
    if (sessions.contains(sessionId))
    {
        // Mark as finished for completed transfers
        updateSessionStatus(sessionId, TransferStatus::FINISHED);
    }

    // Disconnect all signals first
    sender->disconnect();

    // Use QPointer for safe deletion
    QPointer<Sender> senderPtr(sender);

    // Delay cleanup to ensure UI updates are processed
    QTimer::singleShot(100, this, [this, senderPtr, sessionId]()
                       {
        if (senderPtr) {
            senderToSession.remove(senderPtr);
            sessions.remove(sessionId);
            delete senderPtr;
        } else {
            // Object was already deleted, just clean up the maps
            sessions.remove(sessionId);
            for (auto it = senderToSession.begin(); it != senderToSession.end(); ++it) {
                if (it.value() == sessionId) {
                    senderToSession.erase(it);
                    break;
                }
            }
        } });
}

/**
 * @brief Handles sender transfer error notification.
 *
 * Updates the session status to ERROR when a file transfer encounters
 * an error, but only if the transfer isn't already finished or cancelled.
 * Safely cleans up the sender object and session data.
 */
void FileTransferManager::onSenderTransferError()
{
    Sender *sender = qobject_cast<Sender *>(this->sender());
    if (!sender || !senderToSession.contains(sender))
    {
        return;
    }

    int sessionId = senderToSession[sender];
    if (sessions.contains(sessionId))
    {
        TransferSession &session = sessions[sessionId];
        if (session.status != TransferStatus::FINISHED &&
            session.status != TransferStatus::CANCELLED)
        {
            updateSessionStatus(sessionId, TransferStatus::ERROR);
        }
    }

    // Disconnect all signals first to prevent double-deletion
    sender->disconnect();

    // Use QPointer for safe deletion
    QPointer<Sender> senderPtr(sender);

    // Delay cleanup to ensure UI updates are processed
    QTimer::singleShot(100, this, [this, senderPtr, sessionId]()
                       {
        if (senderPtr) {
            senderToSession.remove(senderPtr);
            sessions.remove(sessionId);
            delete senderPtr;
        } else {
            // Object was already deleted, just clean up the maps
            sessions.remove(sessionId);
            for (auto it = senderToSession.begin(); it != senderToSession.end(); ++it) {
                if (it.value() == sessionId) {
                    senderToSession.erase(it);
                    break;
                }
            }
        } });
}

/**
 * @brief Handles incoming file transfer requests from the receiver.
 *
 * Adds the file to the pending batch for user approval and creates a new
 * transfer session to track the incoming transfer progress.
 *
 * @param fileName Name of the incoming file
 * @param fileSize Size of the incoming file as string
 * @param socket TCP socket for the file transfer
 */
void FileTransferManager::onReceiverFileTransferRequested(const QString &fileName, const QString &fileSize, QTcpSocket *socket)
{
    pendingBatchFiles.insert(fileName, fileSize.toLongLong());
    pendingBatchSockets.insert(fileName, socket);
    batchTimer->start(200);

    // Create a new session for the incoming transfer
    int sessionId = createTransferSession(fileName, "Incoming");

    receivedFileToSession[fileName] = sessionId;
    updateSessionStatus(sessionId, TransferStatus::WAITING);
}

/**
 * @brief Handles progress updates for incoming file transfers.
 *
 * @param fileName Name of the file being received
 * @param progress Transfer progress percentage (0-100)
 */
void FileTransferManager::onReceiverProgressUpdated(const QString &fileName, int progress)
{
    if (receivedFileToSession.contains(fileName))
    {
        int sessionId = receivedFileToSession[fileName];
        updateSessionProgress(sessionId, progress);
    }
}

/**
 * @brief Handles status updates for incoming file transfers.
 *
 * @param fileName Name of the file being received
 * @param status New transfer status
 */
void FileTransferManager::onReceiverStatusUpdated(const QString &fileName, TransferStatus status)
{
    if (receivedFileToSession.contains(fileName))
    {
        int sessionId = receivedFileToSession[fileName];
        updateSessionStatus(sessionId, status);
    }
}

/**
 * @brief Handles successful completion of incoming file transfers.
 *
 * Marks the session as finished and cleans up the tracking data for
 * the completed file transfer.
 *
 * @param fileName Name of the successfully received file
 */
void FileTransferManager::onReceiverFileReceived(const QString &fileName)
{
    if (receivedFileToSession.contains(fileName))
    {
        int sessionId = receivedFileToSession[fileName];
        updateSessionStatus(sessionId, TransferStatus::FINISHED);
        receivedFileToSession.remove(fileName);
    }
}

/**
 * @brief Initiates a download request for a shared file from another user.
 *
 * Connects to the remote user's server and sends a download request for the
 * specified file. Sets up the local receiver to accept the incoming file
 * transfer and handles the download request protocol.
 *
 * @param userIP IP address of the user sharing the file
 * @param userPort Port number of the user's file server
 * @param relativePath Relative path of the file on the remote system
 * @param fileName Name of the file to download
 */
void FileTransferManager::downloadSharedFile(const QString &userIP, quint16 userPort, const QString &relativePath, const QString &fileName)
{
    // qDebug() << "FileTransferManager: Starting download request for" << fileName << "from" << userIP << ":" << userPort;

    setupReceiver();
    quint16 ourPort = receiver->getServerPort(); // Use receiver port

    QTcpSocket *downloadSocket = new QTcpSocket(this);

    // Use QPointer for safe deletion
    QPointer<QTcpSocket> socketPtr(downloadSocket);

    connect(downloadSocket, &QTcpSocket::connected, this, [this, socketPtr, relativePath, fileName, ourPort]()
            {
        if (!socketPtr) return;

        QString downloadRequest = QString("DOWNLOAD_REQUEST|%1|%2|%3")
            .arg(relativePath, fileName)
            .arg(QString::number(ourPort));

        socketPtr->write(downloadRequest.toUtf8());
        socketPtr->flush();

        QTimer::singleShot(1000, this, [socketPtr]() {
            if (socketPtr && socketPtr->state() == QAbstractSocket::ConnectedState) {
                socketPtr->disconnectFromHost();
            }
        }); });

    connect(downloadSocket, &QTcpSocket::disconnected, this, [socketPtr]()
            {
        if (socketPtr) {
            socketPtr->deleteLater();
        } });

    connect(downloadSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, [socketPtr](QAbstractSocket::SocketError error)
            {
        if (socketPtr) {
            socketPtr->deleteLater();
        } });

    // qDebug() << "FileTransferManager: Connecting to" << userIP << ":" << userPort << "for download request";
    downloadSocket->connectToHost(userIP, userPort);
}
