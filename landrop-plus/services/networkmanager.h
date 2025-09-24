/**
 * @file networkmanager.h
 * @brief Network monitoring and status management for LANDrop
 */

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTimer>
#include <QNetworkInterface>
#include <QHostAddress>

/**
 * @class NetworkManager
 * @brief Monitors network connectivity and IP address changes for the application.
 * 
 * This service continuously monitors the local network interface to detect IP address
 * changes and connection status.
 */
class NetworkManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @enum ConnectionStatus
     * @brief Represents the current network connection state.
     */
    enum class ConnectionStatus {
        DISCONNECTED,  /**< No network connection available */
        CHECKING,      /**< Currently checking connection status */
        CONNECTED      /**< Active network connection detected */
    };

    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    QString getCurrentIP() const { return currentIP; }
    ConnectionStatus getConnectionStatus() const { return connectionStatus; }
    void startMonitoring(int intervalMs = 10000);
    void stopMonitoring();
    void checkConnection();

signals:
    /** @brief Emitted when the local IP address changes */
    void ipAddressChanged(const QString &newIP);
    
    /** @brief Emitted when network connection status changes */
    void connectionStatusChanged(ConnectionStatus status);

private:
    void updateConnectionStatus();
    QString getLocalIPAddress();

    /** Timer for periodic network status checks */
    QTimer *monitorTimer;
    
    /** Currently detected IP address */
    QString currentIP;
    
    /** Current network connection status */
    ConnectionStatus connectionStatus;
};

#endif // NETWORKMANAGER_H