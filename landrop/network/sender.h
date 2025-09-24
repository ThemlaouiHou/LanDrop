#ifndef SENDER_H
#define SENDER_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QHostAddress>

#include "../config/config.h"

class Sender : public QObject
{
    Q_OBJECT
    qint64 bytesSent = 0;

public:
    explicit Sender(QObject *parent = nullptr);
    ~Sender();

    void sendFile(const QString &filePath, const QString &receiverIP);

private slots:
    void onReadyRead();
    void onConnected();

signals:
    void progressUpdated(int percent);
    void transferAccepted();
    void transferRefused();
    void transferFinished();
    void transferError();

private:
    QTcpSocket *socket;
    QFile *file;
    const int port = Config::port;
    const int bufferSize = Config::bufferSize;

    void reset();
};

#endif // SENDER_H
