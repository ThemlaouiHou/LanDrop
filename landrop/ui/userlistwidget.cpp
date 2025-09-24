#include "userlistwidget.h"
#include <QProcess>
#include <QRegularExpression>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QDebug>
#include <QHostInfo>
#include <QTcpSocket>
#include "../config/config.h"

// Utility function that checks whether an IP address is in the same subnetwork than one of the machine's active network interfaces
bool UserListWidget::isInLocalSubnet(const QHostAddress &ip)
{
    if (ip.protocol() != QAbstractSocket::IPv4Protocol) return false;

    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        // Ignore inactive or loopback interfaces
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            (iface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                quint32 mask   = entry.netmask().toIPv4Address();
                quint32 local  = entry.ip().toIPv4Address();
                quint32 remote = ip.toIPv4Address();

                // If (local & mask) == (remote & mask), then the ip is in the same subnet
                if ((local & mask) == (remote & mask))
                    return true;
            }
        }
    }
    return false;
}

UserListWidget::UserListWidget(QWidget *parent)
    : QWidget(parent), pendingPings(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *title = new QLabel("User list", this);
    title->setStyleSheet("font-weight: bold; font-size: 16px;");
    listWidget = new QListWidget(this);
    refreshButton = new QPushButton("Refresh", this);
    refreshButton->setStyleSheet(Config::buttonStyleSheet);

    layout->addWidget(title);
    scanStatusLabel = new QLabel("Scan in progress...", this);
    scanStatusLabel->setAlignment(Qt::AlignCenter);
    scanStatusLabel->setStyleSheet("color: orange; font-style: italic;");
    scanStatusLabel->setVisible(false);
    scanStatusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(scanStatusLabel);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setVisible(false);
    layout->addWidget(progressBar);

    layout->addWidget(listWidget);
    layout->addWidget(refreshButton);

    connect(listWidget, &QListWidget::itemClicked, this, &UserListWidget::onItemClicked);
    connect(refreshButton, &QPushButton::clicked, this, &UserListWidget::scanNetwork);

    connect(&getWifiIPProcess, &QProcess::finished, this, &UserListWidget::returnGetWifiIP);
    connect(&arpProcess, &QProcess::finished, this, &UserListWidget::returnScanNetwork);

    // First scan on starting
    scanNetwork();
}

QString localIp;

void UserListWidget::scanNetwork()
{
    listWidget->clear();
    progressBar->setValue(0);
    setState(STATE::PROGRESS_BAR);

#ifdef Q_OS_WIN
    getWifiIPProcess.start("powershell", QStringList() << "-Command"
                                                       << "(Get-NetIPAddress | Where-Object { $_.InterfaceAlias -like '*Wi-Fi*' -and $_.AddressFamily -eq 'IPv4' }).IPAddress");
#else
    getWifiIPProcess.start("bash", QStringList() << "-c"
                                                 << "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1");
#endif
}

void UserListWidget::returnGetWifiIP(){
    progressBar->setValue(25);
    QString output = getWifiIPProcess.readAllStandardOutput().trimmed();
    localIp = output.isEmpty() ? "Non détectée" : output;

    if (localIp.isEmpty()) {
        setState(STATE::IDLE);
        return;
    }

    // Read the table ARPQStringList ipsToScan;
    arpProcess.start("arp", QStringList() << "-a");
}

void UserListWidget::returnScanNetwork(){
    ipsToScan.clear();
    progressBar->setValue(50);
    QString output = arpProcess.readAllStandardOutput();

    static QRegularExpression ipRegex(R"((\d+\.\d+\.\d+\.\d+))");
    QRegularExpressionMatchIterator matchIt = ipRegex.globalMatch(output);

    QString ipPrefixAB = localIp.section('.', 0, 1) + "." ;
    while (matchIt.hasNext()) {
        QRegularExpressionMatch match = matchIt.next();
        QString ipStr = match.captured(1);

        if (ipStr != localIp &&
            ipStr.startsWith(ipPrefixAB) &&
            isInLocalSubnet(QHostAddress(ipStr))) {
            ipsToScan << ipStr;
        }
    }

    pendingPings = ipsToScan.size();
    int totalIPs = pendingPings;

    if (pendingPings == 0) {
        setState(STATE::IDLE);
        return;
    }

    for (const QString &ipStr : ipsToScan) {
        QHostInfo::lookupHost(ipStr, this, [=](const QHostInfo &info) {
            QString hostname = info.hostName().toLower();

            listWidget->addItem(ipStr + " [" + hostname + "]");

            pendingPings--;
            progressBar->setValue(50 + ((100 * (totalIPs - pendingPings)) / totalIPs) / 2);
            if (pendingPings == 0) {
                setState(STATE::IDLE);
            }
        });
    }
}

void UserListWidget::onItemClicked(QListWidgetItem *item)
{
    emit ipSelected(item->text().split('[')[0].trimmed());
}

void UserListWidget::setState(STATE state){
    if(state == STATE::IDLE){
        progressBar->setVisible(false);
        scanStatusLabel->setVisible(false);
        listWidget->setVisible(true);
        refreshButton->setEnabled(true);
        refreshButton->setStyleSheet(Config::buttonStyleSheet);
    }else{
        refreshButton->setEnabled(false);
        refreshButton->setStyleSheet(Config::disabledButtonStyleSheet);
        listWidget->setVisible(false);
        scanStatusLabel->setVisible(true);
        progressBar->setVisible(true);
    }
}

UserListWidget::~UserListWidget(){
    getWifiIPProcess.waitForFinished();
    arpProcess.waitForFinished();
}
