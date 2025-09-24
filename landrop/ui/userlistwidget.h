#ifndef USERLISTWIDGET_H
#define USERLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QNetworkInterface>
#include <QProcess>
#include <QHostAddress>
#include <QProcess>
#include <QHostAddress>
#include <QProgressBar>
#include <QLabel>

class UserListWidget : public QWidget {
    Q_OBJECT

public:
    enum STATE {IDLE, LOADING, PROGRESS_BAR};
    explicit UserListWidget(QWidget *parent = nullptr);
    QString getWifiIP() ;
    void setState(STATE);

private slots:
    void scanNetwork();  // Method to get the IPs

signals:
    void ipSelected(const QString &ip);

private:
    QListWidget *listWidget;
    QPushButton *refreshButton;
    int pendingPings; //Counts the pings in progress
    QProgressBar *progressBar;
    QLabel *scanStatusLabel ;
    QProcess getWifiIPProcess;
    QProcess arpProcess;
    QStringList ipsToScan;
    ~UserListWidget();

    // Verifys if the IP address is in the same subnet than an active interface
    bool isInLocalSubnet(const QHostAddress &ip);
    void onItemClicked(QListWidgetItem *item);
    void returnGetWifiIP();
    void returnScanNetwork();
};

#endif // USERLISTWIDGET_H
