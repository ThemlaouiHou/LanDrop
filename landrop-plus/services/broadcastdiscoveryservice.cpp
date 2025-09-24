/**
 * @file broadcastdiscoveryservice.cpp
 * @brief Implementation of the BroadcastDiscoveryService class for peer discovery
 */

#include "broadcastdiscoveryservice.h"
#include "sharedfilemanager.h"
#include "../config/config.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QOperatingSystemVersion>

const QString BroadcastDiscoveryService::PROTOCOL_VERSION = "V1";

/**
 * @brief Constructs a new BroadcastDiscoveryService instance.
 *
 * Initializes the discovery service with UDP socket, timers for broadcasting and cleanup,
 * and automatically starts the discovery process.
 *
 * @param parent The parent QObject for memory management
 */
BroadcastDiscoveryService::BroadcastDiscoveryService(QObject *parent)
    : QObject(parent),
      discoverySocket(nullptr),
      broadcastTimer(new QTimer(this)),
      cleanupTimer(new QTimer(this)),
      discovering(false),
      discoveryInterval(5000),
      myDiscoveryPort(0),
      sharedFileManager(nullptr),
      cachedSharedFilesJson("[]"),
      fileScanTimer(new QTimer(this))
{
    connect(broadcastTimer, &QTimer::timeout, this, &BroadcastDiscoveryService::performPeriodicBroadcast);
    connect(cleanupTimer, &QTimer::timeout, this, &BroadcastDiscoveryService::cleanupExpiredUsers);
    connect(fileScanTimer, &QTimer::timeout, this, &BroadcastDiscoveryService::scanSharedFilesDirectly);
    startDiscovery();
}

/**
 * @brief Destructor that ensures proper cleanup of the discovery service.
 */
BroadcastDiscoveryService::~BroadcastDiscoveryService()
{
    stopDiscovery();
}

/**
 * @brief Starts the network discovery process.
 *
 * Initializes UDP socket binding on the fixed discovery port, clears any existing
 * user data, and starts the periodic broadcast and cleanup timers.
 */
void BroadcastDiscoveryService::startDiscovery()
{
    if (discovering)
        return;

    if (!findAndBindAvailablePort())
    {
        // qCritical() << "BroadcastDiscoveryService: Unable to bind to discovery port.";
        return;
    }

    discovering = true;
    discoveredUsers.clear();
    lastSeenTimes.clear();

    broadcastTimer->start(discoveryInterval);
    cleanupTimer->start(10000);
    performPeriodicBroadcast();
    QTimer::singleShot(200, this, &BroadcastDiscoveryService::requestUserListUpdate);

    emit discoveryStarted();
}

/**
 * @brief Stops the network discovery process and cleans up resources.
 *
 * Stops all discovery operations by stopping timers, closing UDP socket,
 * and clearing all discovered user data.
 */
void BroadcastDiscoveryService::stopDiscovery()
{
    if (!discovering)
        return;

    discovering = false;
    broadcastTimer->stop();
    cleanupTimer->stop();

    if (discoverySocket)
    {
        discoverySocket->close();
        discoverySocket->deleteLater();
        discoverySocket = nullptr;
    }

    myDiscoveryPort = 0;
    discoveredUsers.clear();
    lastSeenTimes.clear();

    emit discoveryStopped();
}

/**
 * @brief Attempts to bind a UDP socket to the fixed discovery port.
 *
 * @return true if socket binding succeeds, false if port is unavailable
 */
bool BroadcastDiscoveryService::findAndBindAvailablePort()
{
    discoverySocket = new QUdpSocket(this);

    if (discoverySocket->bind(QHostAddress::Any, DISCOVERY_PORT, QUdpSocket::DontShareAddress))
    {
        myDiscoveryPort = DISCOVERY_PORT;
        connect(discoverySocket, &QUdpSocket::readyRead, this, &BroadcastDiscoveryService::handleDiscoveryMessage);
        // qDebug() << "BroadcastDiscoveryService: Successfully bound to port" << DISCOVERY_PORT;
        return true;
    }

    delete discoverySocket;
    discoverySocket = nullptr;
    return false;
}


/**
 * @brief Scans the shared files directory and caches the file list as JSON.
 *
 * Directly scans the configured shared folder path for files and creates a JSON
 * representation of available files.
 * Updates the cached JSON representation for use in discovery protocol messages.
 */
void BroadcastDiscoveryService::scanSharedFilesDirectly()
{
    QString sharedPath = Config::getSharedFolderPath();
    if (sharedPath.isEmpty()) {
        // Fallback if Config not set
        QString exeDir = QCoreApplication::applicationDirPath();
        sharedPath = QDir(exeDir).absoluteFilePath("Shared Files");
    }
    QDir sharedDir(sharedPath);
    
    if (!sharedDir.exists()) {
        QDir().mkpath(sharedPath);
    }
    
    QFileInfoList files = sharedDir.entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    
    if (files.isEmpty()) {
        cachedSharedFilesJson = "[]";
        return;
    }
    
    QJsonArray filesArray;
    for (const QFileInfo &fileInfo : files) {
        QJsonObject obj;
        obj["name"] = fileInfo.fileName();
        obj["path"] = fileInfo.fileName(); // files are in root of shared folder
        obj["size"] = QString::number(fileInfo.size());
        obj["type"] = "file";
        filesArray.append(obj);
    }
    
    QJsonDocument doc(filesArray);
    cachedSharedFilesJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    // qDebug() << "BroadcastDiscoveryService: Found" << files.size() << "files:" << cachedSharedFilesJson;
}



/**
 * @brief Processes incoming UDP discovery messages from other LANDrop instances.
 *
 * This slot is triggered when the discovery socket receives data. 
 * Ensures proper parsing and validation before processing discovery protocol messages.
 */
void BroadcastDiscoveryService::handleDiscoveryMessage()
{
    if (!discoverySocket || !discovering)
    {
        // qDebug() << "BroadcastDiscoveryService: Aborting - socket or discovery state invalid";
        return;
    }

    while (discoverySocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(discoverySocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        qint64 bytesRead = discoverySocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        if (bytesRead == -1)
        {
            continue;
        }

        datagram.resize(bytesRead);
        QString message = QString::fromUtf8(datagram);
        QString senderIP = sender.toString();

        // Clean IPv6-mapped addresses immediately to prevent connection issues
        if (senderIP.startsWith("::ffff:"))
        {
            senderIP = senderIP.mid(7);
        }

        // qDebug() << "BroadcastDiscoveryService: Received from" << senderIP << ":" << message;

        // Check if this is a response to our probe (indicates port is occupied)
        if (message.startsWith(QString("LANDROP_DISCOVERY_%1|").arg(PROTOCOL_VERSION)))
        {
            QString localHostname = getLocalHostname();
            QString localIP = getLocalIPAddress();
            handleDiscoveryRequest(message, sender, senderIP);
        }
        else if (message.startsWith(QString("LANDROP_RESPONSE_%1|").arg(PROTOCOL_VERSION)))
        {
            QString localIP = getLocalIPAddress();
            handleDiscoveryResponse(message, senderIP);
        }
    }
}

/**
 * @brief Handles incoming discovery request messages.
 *
 * Parses the received message, extracts peer information (hostname, ports, shared files),
 * responds with our shared file list, and updates internal state to reflect the presence
 * of this peer.
 *
 * @param message The raw discovery message string received via UDP
 * @param sender The QHostAddress of the sender
 * @param senderIP The IP address string of the sender (cleaned from IPv6-mapped format, the ::ffff:)
 */
void BroadcastDiscoveryService::handleDiscoveryRequest(const QString &message, const QHostAddress &sender, const QString &senderIP)
{
    QStringList parts = message.split('|');
    if (parts.size() >= 4)
    {
        quint16 senderDiscoveryPort = parts[1].toUShort();
        quint16 transferPort = parts[2].toUShort();
        QString hostname = parts[3];

        QJsonArray sharedFiles;
        if (parts.size() >= 5)
        {
            try
            {
                QString sharedFilesJson = parts[4];
                QJsonParseError error;
                QJsonDocument doc = QJsonDocument::fromJson(sharedFilesJson.toUtf8(), &error);
                if (error.error == QJsonParseError::NoError && doc.isArray())
                {
                    sharedFiles = doc.array();
                }
            }
            catch (...)
            {
            }
        }

        // Check if this is a self-message
        if (isSelfMessage(senderIP, hostname))
        {
            return;
        }

        // Send response with our shared files
        QString ourSharedFilesJson = cachedSharedFilesJson;

        QString responseMessage = QString("LANDROP_RESPONSE_%1|%2|%3|%4|%5")
                                      .arg(PROTOCOL_VERSION)
                                      .arg(myDiscoveryPort)
                                      .arg(getTransferPort())
                                      .arg(getLocalHostname())
                                      .arg(ourSharedFilesJson);

        qint64 result = discoverySocket->writeDatagram(responseMessage.toUtf8(), sender, senderDiscoveryPort);
        
        /* 
        if (result > 0)
        {
            qDebug() << "BroadcastDiscoveryService: Sent response to:" << senderIP << "port:" << senderDiscoveryPort;
        }
        else
        {
            qDebug() << "BroadcastDiscoveryService: Failed to send response to:" << senderIP << "port:" << senderDiscoveryPort;
        } 
        */

        // Add this user to our list
        LANDropUser user(senderIP, hostname, transferPort, "1.0");
        user.sharedFiles = sharedFiles;
        updatePeerWithSharedFiles(user);
    }
}

/**
 * @brief Handles a discovery response from another LANDrop instance.
 *
 * Parses and validates the incoming response, extracts peer info and shared file list,
 * and updates the internal discoveredUsers list accordingly.
 *
 * @param message The received LANDROP_RESPONSE message string
 * @param senderIP The IP address of the sender as a string
 */
void BroadcastDiscoveryService::handleDiscoveryResponse(const QString &message, const QString &senderIP)
{
    QStringList parts = message.split('|');
    if (parts.size() >= 4)
    {
        quint16 discoveryPort = parts[1].toUShort();
        quint16 transferPort = parts[2].toUShort();
        QString hostname = parts[3];

        QJsonArray sharedFiles;
        if (parts.size() >= 5)
        {
            try
            {
                QString sharedFilesJson = parts[4];
                QJsonParseError error;
                QJsonDocument doc = QJsonDocument::fromJson(sharedFilesJson.toUtf8(), &error);
                if (error.error == QJsonParseError::NoError && doc.isArray())
                {
                    sharedFiles = doc.array();
                }
            }
            catch (...)
            {
            }
        }

        // Check if this is a self-message
        if (isSelfMessage(senderIP, hostname))
        {
            return;
        }


        LANDropUser user(senderIP, hostname, transferPort, "1.0");
        user.sharedFiles = sharedFiles;
        updatePeerWithSharedFiles(user);
    }
}

/**
 * @brief Performs periodic broadcast when the broadcast timer triggers.
 */
void BroadcastDiscoveryService::performPeriodicBroadcast()
{
    if (!discovering || !discoverySocket)
        return;

    sendDiscoveryBroadcast();
}

/**
 * @brief Sends a broadcast message to discover peers on the local network.
 *
 * Constructs a discovery message containing hostname, ports, and shared files,
 * then broadcasts it using UDP. On Windows 11+, uses interface-specific
 * broadcast addresses for better compatibility, otherwise uses global broadcast.
 */
void BroadcastDiscoveryService::sendDiscoveryBroadcast()
{
    // Always rebuild message from current state to avoid stale data crashes
    QString hostname = getLocalHostname();
    quint16 transferPort = getTransferPort();

    QString sharedFilesJson = cachedSharedFilesJson;

    QString message = QString("LANDROP_DISCOVERY_%1|%2|%3|%4|%5")
                          .arg(PROTOCOL_VERSION)
                          .arg(myDiscoveryPort)
                          .arg(transferPort)
                          .arg(hostname)
                          .arg(sharedFilesJson);

#ifdef Q_OS_WIN
    QOperatingSystemVersion version = QOperatingSystemVersion::current();
    if (version >= QOperatingSystemVersion::Windows11)
    {

        bool sentAny = false;
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        for (const QNetworkInterface &iface : interfaces)
        {
            if (!(iface.flags() & QNetworkInterface::IsUp) ||
                !(iface.flags() & QNetworkInterface::IsRunning) ||
                (iface.flags() & QNetworkInterface::IsLoopBack))
            {
                continue;
            }

            for (const QNetworkAddressEntry &entry : iface.addressEntries())
            {
                if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
                    continue;

                QHostAddress bcast = entry.broadcast();
                if (!bcast.isNull())
                {
                    qint64 result = discoverySocket->writeDatagram(message.toUtf8(), bcast, DISCOVERY_PORT);
                    if (result > 0)
                        sentAny = true;
                }
            }
        }

        // Fallback to global broadcast if no interface-specific broadcasts worked
        if (!sentAny)
        {
            QHostAddress broadcastAddress("255.255.255.255");
            qint64 result = discoverySocket->writeDatagram(message.toUtf8(), broadcastAddress, DISCOVERY_PORT);
        }
    }
    else
#endif
    {
        // behavior for Win10 and maybe other OSes
        QHostAddress broadcastAddress("255.255.255.255");
        qint64 result = discoverySocket->writeDatagram(message.toUtf8(), broadcastAddress, DISCOVERY_PORT);
    }
}


/**
 * @brief Updates or adds a discovered user with complete shared file information.
 * 
 * @param user Complete LANDropUser object with IP, hostname, ports, and shared files
 */
void BroadcastDiscoveryService::updatePeerWithSharedFiles(const LANDropUser &user)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    lastSeenTimes[user.ipAddress] = currentTime;

    bool found = false;
    for (int i = 0; i < discoveredUsers.size(); ++i)
    {
        if (discoveredUsers[i].ipAddress == user.ipAddress || discoveredUsers[i].hostname == user.hostname)
        {
            discoveredUsers[i] = user;
            found = true;
            break;
        }
    }

    if (!found)
    {
        discoveredUsers.append(user);
    }

    emit userListUpdated(discoveredUsers);
}

/**
 * @brief Removes expired and invalid users from the discovered users list.
 * 
 */
void BroadcastDiscoveryService::cleanupExpiredUsers()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    bool anyRemoved = false;
    QString localIP = getLocalIPAddress();
    QString localHostname = getLocalHostname();

    for (int i = discoveredUsers.size() - 1; i >= 0; --i)
    {
        QString ip = discoveredUsers[i].ipAddress;
        QString hostname = discoveredUsers[i].hostname;
        qint64 lastSeen = lastSeenTimes.value(ip, 0);
        qint64 timeSinceLastSeen = currentTime - lastSeen;

        // Remove self-entries if they exist
        if (ip == localIP || hostname == localHostname)
        {
            discoveredUsers.removeAt(i);
            lastSeenTimes.remove(ip);
            anyRemoved = true;
            continue;
        }

        // Remove expired users
        if (timeSinceLastSeen > USER_TIMEOUT_MS)
        {
            int secondsSinceLastPing = timeSinceLastSeen / 1000;
            discoveredUsers.removeAt(i);
            lastSeenTimes.remove(ip);
            anyRemoved = true;
        }
    }

    if (anyRemoved)
    {
        emit userListUpdated(discoveredUsers);
    }
}

/**
 * @brief Triggers an immediate user list update signal.
 *
 */
void BroadcastDiscoveryService::requestUserListUpdate()
{
    emit userListUpdated(discoveredUsers);
}

/**
 * @brief Sets the shared file manager and initializes periodic file scanning.
 *
 * Associates a SharedFileManager instance with this discovery service and
 * starts periodic scanning of shared files.
 *
 * @param manager Pointer to the SharedFileManager instance
 */
void BroadcastDiscoveryService::setSharedFileManager(SharedFileManager *manager)
{
    sharedFileManager = manager; 
    // Ignore the SharedFileManager and scan files directly
    scanSharedFilesDirectly();
    
    // Start periodic rescanning every 10 seconds
    fileScanTimer->start(10000);
}

/**
 * @brief Retrieves the local machine's hostname.
 *
 * @return The local machine's hostname as reported by the operating system
 */
QString BroadcastDiscoveryService::getLocalHostname() const
{
    return QSysInfo::machineHostName();
}

/**
 * @brief Retrieves the local machine's primary IP address.
 *
 * @return Local IP address as string, or empty string if no valid interface found
 */
QString BroadcastDiscoveryService::getLocalIPAddress() const
{
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces)
    {
        if (!(interface.flags() & QNetworkInterface::IsUp) ||
            (interface.flags() & QNetworkInterface::IsLoopBack))
        {
            continue;
        }

        // Only use ethernet or wifi interfaces for local IP
        QString name = interface.name().toLower();
        QNetworkInterface::InterfaceType type = interface.type();
        
        if (type != QNetworkInterface::Ethernet && 
            type != QNetworkInterface::Wifi &&
            !name.contains("ethernet") &&
            !name.contains("wifi") &&
            !name.contains("wlan") &&
            !name.contains("eth"))
        {
            continue;
        }

        for (const QNetworkAddressEntry &entry : interface.addressEntries())
        {
            QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                !ip.isLoopback() && !ip.isMulticast())
            {
                return ip.toString();
            }
        }
    }

    return QString();
}

/**
 * @brief Retrieves the configured file transfer port.
 *
 * @return Port number used for file transfers as configured in Config::getPort()
 */
quint16 BroadcastDiscoveryService::getTransferPort() const
{
    return Config::getPort();
}

/**
 * @brief Checks if a discovery message is from the local machine.
 *
 * @param senderIP IP address of the message sender
 * @param hostname Hostname of the message sender
 * @return true if the message is from this machine, false otherwise
 */
bool BroadcastDiscoveryService::isSelfMessage(const QString &senderIP, const QString &hostname) const
{
    QString localIP = getLocalIPAddress();
    QString localHostname = getLocalHostname();
    // qDebug() << "BroadcastDiscoveryService: Checking self-detection - senderIP:" << senderIP << "localIP:" << localIP << "hostname:" << hostname << "localHostname:" << localHostname;
    if ((senderIP == localIP) || (hostname == localHostname))
    {
        return true;
    }
    return false;
}

