/**
 * @file sender.cpp
 */

#include "sender.h"
#include <QFileInfo>
#include <QDebug>

/**
 * @brief Constructs a new Sender instance.
 *
 * Initializes the TCP socket pointer, file handle, timers, and sets up
 * timeout handlers for connection and response timeouts.
 *
 * @param parent Parent QObject
 */
Sender::Sender(QObject *parent)
    : QObject(parent), socket(nullptr), file(nullptr), bytesSent(0), port(Config::getPort()),
      connectionTimer(new QTimer(this)), responseTimer(new QTimer(this))
{
    connectionTimer->setSingleShot(true);
    responseTimer->setSingleShot(true);

    connect(connectionTimer, &QTimer::timeout, this, [this]()
            {
        qDebug() << "Sender: Connection timeout";
        emit transferError();
        reset(); });

    connect(responseTimer, &QTimer::timeout, this, [this]()
            {
        qDebug() << "Sender: Response timeout - no OK/NO received";
        emit transferError();
        reset(); });
}

/**
 * @brief Proper destructor for Sender.
 */
Sender::~Sender()
{
    reset();
}

/**
 * @brief Resets the sender to its initial state, cleaning up all resources.
 *
 * This method safely disconnects the TCP socket, closes the file handle,
 * stops all timers, and resets internal state variables.
 *
 * @note This method is called automatically by the destructor and before
 *       starting new transfers to ensure clean state.
 */
void Sender::reset()
{
    connectionTimer->stop();
    responseTimer->stop();

    if (socket)
    {
        socket->blockSignals(true);

        if (socket->state() != QAbstractSocket::UnconnectedState)
        {
            if (socket != nullptr)
            {
                socket->disconnectFromHost();
            }
        }

        socket->deleteLater();
        socket = nullptr;
    }
    if (file)
    {
        if (file->isOpen())
            file->close();
        delete file;
        file = nullptr;
    }
    bytesSent = 0;
}


/**
 * @brief Initiates a file transfer to a specific receiver on a custom port.
 *
 * This method performs the full file transfer process:
 * 1. Validates that the file exists
 * 2. Creates a TCP connection to the receiver
 * 3. Sends file metadata for user confirmation
 * 4. Transfers file data in chunks upon acceptance
 * 5. Provides progress updates throughout the transfer
 *
 * @param filePath Absolute path to the file to be sent.
 * @param receiverIP IP address of the receiver.
 * @param customPort TCP port number to connect to on the receiver.
 *
 * @note Returns silently if file doesn't exist; emits transferError() for connection/protocol failures.
 * @note Uses a 10-second connection timeout.
 */
void Sender::sendFile(const QString &filePath, const QString &receiverIP, quint16 customPort)
{
    reset();

    port = customPort; // Use the specified port

    file = new QFile(filePath);
    if (!file->exists())
    {
        return;
    }

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Sender::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &Sender::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Sender::reset);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, [this](QAbstractSocket::SocketError socketError)
            { emit transferError(); });

    socket->connectToHost(QHostAddress(receiverIP), port);
    connectionTimer->start(10000); // 10 second connection timeout
}

/**
 * @brief Handles successful TCP connection to the receiver.
 *
 * Called when the socket successfully connects to the receiver. This method
 * sends the file metadata (name, size) in the LANDrop protocol format
 * and starts a timer waiting for the receiver's acceptance response.
 *
 * @note Uses a 30-second timeout for receiver response.
 * @note File metadata is sent in format: "filename|filesize\n"
 */
void Sender::onConnected()
{
    connectionTimer->stop(); // Connection successful

    QByteArray filename = QFileInfo(file->fileName()).fileName().toLocal8Bit();
    QByteArray filesize = QByteArray::number(file->size());
    socket->write(filename + "|" + filesize + "\n");
    socket->flush();

    responseTimer->start(30000); // 30 second response timeout
}

/**
 * @brief Processes responses from the receiver and manages file data transmission.
 *
 * This method handles the LANDrop protocol responses:
 * - "OK": Receiver accepts the transfer, begin sending file data
 * - "NO": Receiver refuses the transfer
 * - Other: Errors
 *
 * For accepted transfers, this method sets up chunked file transmission
 * using the configured buffer size.
 *
 * @note Emits transferAccepted(), transferRefused(), or transferError() based on response.
 * @note Returns silently if file cannot be opened after acceptance.
 * @note Emits progressUpdated() signals during file transmission.
 * @note Emits transferFinished() when file transmission completes.
 */
void Sender::onReadyRead()
{
    if (!socket)
        return; // Safety check

    QByteArray response = socket->readLine().trimmed();

    if (response == "OK")
    {
        responseTimer->stop(); // Got response
        emit transferAccepted();

        // Regular file upload logic
        if (!file->open(QIODevice::ReadOnly))
        {
            reset();
            return;
        }

        bytesSent = 0;

        connect(socket, &QTcpSocket::bytesWritten, this, [this](qint64)
                {
            if (!file || !file->isOpen() || !socket) return;

            if (!file->atEnd()) {
                QByteArray chunk = file->read(Config::getBufferSize());
                bytesSent += chunk.size();
                emit progressUpdated(static_cast<int>(bytesSent * 100 / file->size()));
                socket->write(chunk);
            } else {
                emit transferFinished();
                file->close();
                if (socket && socket->state() != QAbstractSocket::UnconnectedState) {
                    socket->disconnectFromHost();
                }
            } });

        QByteArray chunk = file->read(Config::getBufferSize());
        bytesSent += chunk.size();
        if (socket->write(chunk) < 1)
        {
            emit transferError();
            return;
        }
    }
    else if (response == "NO")
    {
        responseTimer->stop(); // Got response
        if (socket && socket->state() != QAbstractSocket::UnconnectedState) {
            socket->disconnectFromHost();
        }
        emit transferRefused();
    }
    else
    {
        responseTimer->stop();
        if (socket && socket->state() != QAbstractSocket::UnconnectedState) {
            socket->disconnectFromHost();
        }
        emit transferError();
    }
}
