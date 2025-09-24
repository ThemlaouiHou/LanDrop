#ifndef RECEIVER_H
#define RECEIVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QMap>
#include "../ui/transferhistorywidget.h"
#include "../config/config.h"

typedef struct FileDefintion
{
    QFile* file;
    QString name;
    qint64 size;
    qint64 totalRecieved;
} FileDefintion;

class Receiver : public QObject
{
    Q_OBJECT

public:
    explicit Receiver(QObject *parent = nullptr);
    ~Receiver();

    bool startServer(quint16 port = Config::port);
    void setFile(QTcpSocket *s, QFile *f);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

signals:
    void fileTransferRequested(const QString &fileName, const QString &fileSize, QTcpSocket *socket);
    void fileReceivedSuccessfully(const QString &fileName);
    void transferProgressUpdated(const QString &fileName, int percent);
    void transferStatusUpdated(const QString &fileName, TransferHistoryWidget::TransferStatus status);

private:
    QTcpServer *server;
    QDir *directory;
    bool nameReceived;
    QString pendingFileName;
    QMap<QTcpSocket*, FileDefintion> pendingFiles;
};

#endif // RECEIVER_H
