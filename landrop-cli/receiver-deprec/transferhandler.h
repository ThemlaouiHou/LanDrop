#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QFile>

class TransferHandler : public QObject {
    Q_OBJECT

public:
    explicit TransferHandler(QObject* parent = nullptr);
    void receive(QTcpSocket* socket);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QFile* file;
    qint64 fileSize = 0;
    qint64 bytesTransferred = 0;
    qint64 chunkSize = 4096;
    bool isEncrypted = false;
    bool transferComplete = false;
    QTcpSocket* socket = nullptr;
};
