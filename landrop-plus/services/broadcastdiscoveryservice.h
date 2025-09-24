/**
 * @file broadcastdiscoveryservice.h
 * @brief UDP-based network discovery service for LANDrop peer detection
 */

#ifndef BROADCASTDISCOVERYSERVICE_H
#define BROADCASTDISCOVERYSERVICE_H 

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QSysInfo>
#include <QMap>
#include <QJsonArray>
#include <QString>
#include <QList>
#include <QDebug>

// Forward declaration
class SharedFileManager;

/**
 * @struct LANDropUser
 * @brief Represents a discovered LANDrop user on the local network.
 * 
 * Contains all necessary information to identify and connect to another
 * LANDrop instance.
 */
struct LANDropUser {
    /** IP address of the user's machine */
    QString ipAddress;
    
    /** Hostname of the user's machine */
    QString hostname;
    
    /** TCP port for file transfer connections */
    quint16 transferPort;
    
    /** Protocol version string */
    QString version;
    
    /** JSON array of files shared by this user */
    QJsonArray sharedFiles;
    
    LANDropUser() = default;
    LANDropUser(const QString &ip, const QString &host, quint16 port, const QString &ver) 
        : ipAddress(ip), hostname(host), transferPort(port), version(ver) {}
    
    /** Check if user has any shared files available */
    bool hasSharedFiles() const { return !sharedFiles.isEmpty(); }
    
    /** Get the number of shared files */
    int sharedFileCount() const { return sharedFiles.size(); }
};

Q_DECLARE_METATYPE(LANDropUser)

/**
 * @class BroadcastDiscoveryService
 * @brief UDP broadcast-based service for discovering LANDrop users on the local network.
 * 
 * This service handles peer discovery by broadcasting UDP packets on a fixed port
 * and listening for responses from other LANDrop instances. It maintains a list
 * of discovered users and their shared files, with automatic cleanup of expired entries.
 */
class BroadcastDiscoveryService : public QObject
{
    Q_OBJECT

public:
    explicit BroadcastDiscoveryService(QObject *parent = nullptr);
    ~BroadcastDiscoveryService();

    void startDiscovery();
    void stopDiscovery();
    void requestUserListUpdate();
    void setSharedFileManager(SharedFileManager *manager);
    

signals:
    /** @brief Emitted when the complete user list is updated */
    void userListUpdated(const QList<LANDropUser> &users);
    
    /** @brief Emitted when discovery service starts */
    void discoveryStarted();
    
    /** @brief Emitted when discovery service stops */
    void discoveryStopped();

private slots:
    void handleDiscoveryMessage();
    void performPeriodicBroadcast();
    void cleanupExpiredUsers();

private:
    bool findAndBindAvailablePort();
    void sendDiscoveryBroadcast();
    void handleDiscoveryRequest(const QString &message, const QHostAddress &sender, const QString &senderIP);
    void handleDiscoveryResponse(const QString &message, const QString &senderIP);
    QString getLocalHostname() const;
    QString getLocalIPAddress() const;
    quint16 getTransferPort() const;
    void updatePeerWithSharedFiles(const LANDropUser &user);
    void scanSharedFilesDirectly();
    bool isSelfMessage(const QString &senderIP, const QString &hostname) const;

    /** UDP socket for discovery communications */
    QUdpSocket *discoverySocket;
    
    /** Timer for periodic broadcast transmissions */
    QTimer *broadcastTimer;
    
    /** Timer for cleaning up expired user entries */
    QTimer *cleanupTimer;
    
    /** List of currently discovered users */
    QList<LANDropUser> discoveredUsers;
    
    /** Map tracking last seen times for each user */
    QMap<QString, qint64> lastSeenTimes;
    
    /** Current discovery state */
    bool discovering;
    
    /** Interval between discovery broadcasts in milliseconds */
    int discoveryInterval;
    
    /** Our assigned discovery port */
    quint16 myDiscoveryPort;
    
    /** Shared file manager for broadcasting file lists */
    SharedFileManager *sharedFileManager;
    
    /** Cached shared files JSON to avoid frequent calls */
    QString cachedSharedFilesJson;
    
    /** Timer for periodic file rescanning */
    QTimer *fileScanTimer;
    
    
    /** Fixed UDP port for discovery communications */
    static const quint16 DISCOVERY_PORT = 12346;
    
    /** Protocol version identifier */
    static const QString PROTOCOL_VERSION;
    
    /** Timeout for removing inactive users */
    static const int USER_TIMEOUT_MS = 15000;
};


#endif // BROADCASTDISCOVERYSERVICE_H