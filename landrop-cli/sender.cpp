#include <QCoreApplication>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QFile>
#include <QFileInfo>
#include <iostream>

using namespace std;

const int port = 42424;
const int bufferSize = 65536;

QHostAddress IP;
QHostAddress rescipientIP;
QFile* file;

QTcpSocket *socket;
QByteArray filename;
QByteArray fileSize;

void sendFile(QHostAddress*, QFile*);
void readyRead();

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if(argc == 3){
        rescipientIP = QHostAddress(QString(argv[1]));
        if (rescipientIP.isNull()){
            cout << "\nError : Invalid IP" << argv[1];
            return 0;
        }

        file = new QFile(QString(argv[2]));
        if (!file->exists() || !QFileInfo(file->fileName()).isFile()){
            cout << "\nError : Invalid file";
            return 0;
        }

        sendFile(&rescipientIP, file);
        return a.exec();
    }else if(argc != 1){
        cout << "\nError : Wrong arguments\nUsage : " << std::filesystem::path(argv[0]).filename().string() << " [IP] [Path to file] OR without arguments";
        return 0;
    }

    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
            IP = address;
    }
    cout << "LANDrop sender test application\nMy IP : " << IP.toString().toStdString() << "\n\n";

    cout << "Enter reciever IP : ";
    do{
        string input;
        cin >> input;
        rescipientIP = QHostAddress(QString::fromStdString(input));
        if (!rescipientIP.isNull()) break;
        cout << "Please enter a valid reciever IP : ";
    } while(true);

    cout << "Enter file to send : ";
    do{
        string input;
        cin >> input;
        file = new QFile(QString::fromStdString(input));
        if (file->exists() && QFileInfo(file->fileName()).isFile()) break;
        cout << "Please enter a valid file to send : ";
    } while(true);

    cout << "\n\nFile : " << file->fileName().toStdString() << "\nIP : " << IP.toString().toStdString() << "\n\nProceed to file sending ? (y/N) : ";

    string input;
    cin >> input;
    if (input != "Y" && input != "y") return 0;

    sendFile(&rescipientIP, file);

    return a.exec();
}

void sendFile(QHostAddress* ip, QFile* file){
    socket = new QTcpSocket();
    socket->connectToHost(*ip, port);
    if (socket->waitForConnected(5000)){
        if (!file->open(QIODevice::ReadOnly)){
            cout << "\nError : Could not open the file";
            return;
        }

        filename = QFileInfo(file->fileName()).fileName().toLocal8Bit();
        fileSize  = QByteArray::number(file->size());

        QByteArray notification = QString(QString(filename) + "|" + ip->toString() + "|" + QString(fileSize) + "\n").toLocal8Bit();
        qDebug() << notification.toStdString();
        QObject::connect(socket, &QTcpSocket::readyRead, readyRead);
        socket->write(notification, qstrlen(notification));
        socket->flush();
    }else{
        cout << "\nError : " << socket->errorString().toStdString() << endl;
        socket->disconnectFromHost();
        delete socket;
    }
}

void readyRead(){
    QString data = QString(socket->readAll().trimmed());

    if(data == "KO"){
        cout << "Transfer refused, cancelling connection";
        socket->disconnectFromHost();
        delete socket;
        return;
    }

    qint64 writtenBytes = socket->write(filename, qstrlen(filename));
    socket->write("\n");
    socket->flush();
    socket->waitForBytesWritten();
    if (writtenBytes == -1){
        cout << "\nError : Could not send file name";
        socket->disconnectFromHost();
        delete socket;
        return;
    }
    cout << "Sent file name : " << filename.toStdString() << "\n";

    writtenBytes = socket->write(fileSize, qstrlen(fileSize));
    socket->write("\n");
    socket->flush();
    socket->waitForBytesWritten();
    if (writtenBytes == -1){
        cout << "\nError : Could not send file size";
        socket->disconnectFromHost();
        delete socket;
        return;
    }
    cout << "Sent file size : " << fileSize.toStdString() << "\n";

    qint64 totalWrittenBytes = 0;
    while (!file->atEnd()) {
        QByteArray data = file->read(bufferSize);
        writtenBytes = socket->write(data);
        socket->flush();
        socket->waitForBytesWritten();
        if(writtenBytes == -1){
            cout << "\nError : Could not write data";
        }
        else{
            cout << writtenBytes << " bytes written\n";
            totalWrittenBytes += writtenBytes;
        }
    }

    socket->waitForBytesWritten();
    cout << "\nTransfer complete, written " << totalWrittenBytes << " bytes in total\n";
    socket->disconnectFromHost();
    delete socket;
}
