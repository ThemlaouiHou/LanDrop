/**
 * @file networkmanager.cpp
 */

#include "networkmanager.h"
#include <QDebug>

/**
 * @brief Constructs a new NetworkManager.
 *
 * Initializes the monitoring timer and connection status.
 *
 * @param parent Parent QObject
 */
NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent),
      monitorTimer(new QTimer(this)),
      connectionStatus(ConnectionStatus::DISCONNECTED)
{
    connect(monitorTimer, &QTimer::timeout, this, &NetworkManager::checkConnection);
}

/**
 * @brief Destructor for NetworkManager.
 *
 * Proper cleanup by stopping network monitoring.
 */
NetworkManager::~NetworkManager()
{
    stopMonitoring();
}

/**
 * @brief Starts periodic network monitoring.
 *
 * @param intervalMs Monitoring interval in milliseconds
 */
void NetworkManager::startMonitoring(int intervalMs)
{
    checkConnection();
    monitorTimer->start(intervalMs);
}

/**
 * @brief Stops network monitoring.
 */
void NetworkManager::stopMonitoring()
{
    monitorTimer->stop();
}

/**
 * @brief Performs a network connectivity check.
 */
void NetworkManager::checkConnection()
{
    connectionStatus = ConnectionStatus::CHECKING;
    emit connectionStatusChanged(connectionStatus);

    // Get local IP using Qt network interfaces
    QString newIP = getLocalIPAddress();

    if (newIP != currentIP)
    {
        currentIP = newIP;
        emit ipAddressChanged(currentIP);
    }

    updateConnectionStatus();
}

/**
 * @brief Updates the connection status based on current IP availability.
 */
void NetworkManager::updateConnectionStatus()
{
    ConnectionStatus newStatus = currentIP.isEmpty() ? ConnectionStatus::DISCONNECTED : ConnectionStatus::CONNECTED;

    if (newStatus != connectionStatus)
    {
        connectionStatus = newStatus;
        emit connectionStatusChanged(connectionStatus);
    }
}

/**
 * @brief Retrieves the local IP address of the primary network interface.
 *
 * Scans all network interfaces to find the first valid IPv4 address that
 * is not a loopback, multicast, or virtual machine interface. Filters out
 * common virtualization interfaces (VirtualBox, VMware, Docker) to return
 * the actual physical network adapter's IP address.
 *
 * @return Local IP address as string, or empty string if no valid interface found
 */
QString NetworkManager::getLocalIPAddress()
{
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces)
    {
        if (!(interface.flags() & QNetworkInterface::IsUp) ||
            (interface.flags() & QNetworkInterface::IsLoopBack))
        {
            continue;
        }

        QString name = interface.name().toLower();
        if (name.contains("virtualbox") || name.contains("vmware") ||
            name.contains("docker") || name.contains("veth") ||
            name.contains("br-") || name.startsWith("vbox") ||
            name.startsWith("vmnet") || name.contains("host-only"))
        {
            continue;
        }

        for (const QNetworkAddressEntry &entry : interface.addressEntries())
        {
            QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                !ip.isLoopback() && !ip.isMulticast())
            {

                quint32 addr = ip.toIPv4Address();
                if ((addr & 0xFFFFFF00) == 0xC0A83800)
                    continue; // 192.168.56.x (VirtualBox)
                if ((addr & 0xFFFFFF00) == 0xAC110000)
                    continue; // 172.17.0.x (Docker default bridge)

                return ip.toString();
            }
        }
    }

    return QString();
}