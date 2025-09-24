#include <QCoreApplication>
#include <QTcpSocket>
#include <QTcpServer>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <iostream>

using namespace std;

enum STATE {IDLE, NOTIFICATION, NAME_RECIEVED, SIZE_RECIEVED};

const int port = 42424;
bool nameRecieved = false;
bool sizeRecieved = false;
STATE state = STATE::IDLE;
QTcpServer *Server;
QTcpSocket *socket;
QFile *file;
QDir *directory = new QDir();
qint64 fileSize = 0;
qint64 totalBytesRecieved = 0;

void readyRead(){
    switch(state){
        case STATE::IDLE:{
            QList<QByteArray> data = socket->readLine().trimmed().split('|');

            if (data.size() != 3) {
                cout << "Malformed header received\n";
                socket->disconnectFromHost();
                return;
            }

            try{
                QByteArray name = data[0].trimmed();
                QByteArray ip = data[1].trimmed();
                QByteArray size = data[2].trimmed();
                cout << "INCOMING FILE\nSender IP: " << ip.toStdString() << "\nFile name: " << name.toStdString() << "\nFile size: " << size.toStdString() << " bytes\nAccept transfer? (y/N): ";
            }catch(std::exception){
                cout << "Something went wrong\n";
                socket->disconnectFromHost();
                return;
            }

            string input;
            cin >> input;
            QByteArray message;
            if (input != "Y" && input != "y"){
                message = "KO";
            }
            else{
                message = "OK";
                state = STATE::NOTIFICATION;
            }

            socket->write(message, qstrlen(message));
            break;
        }
        case STATE::NOTIFICATION:{
            state = STATE::NAME_RECIEVED;
            QByteArray data = socket->readLine().trimmed();

            if(!directory->exists("./Recieved Files")){
                if (directory->mkdir("./Recieved Files")){
                    cout << "Created files directory\n";
                }else{
                    cout << "Could not create files directory\n";
                }
            }

            file = new QFile("./Recieved Files/"+data);

            cout << "Received file name:" << data.toStdString() << "\n";
            break;
        }
        case STATE::NAME_RECIEVED:{
            state = STATE::SIZE_RECIEVED;
            QByteArray data = socket->readLine().trimmed();
            fileSize = data.toLongLong();
            cout << "Received file size: " << data.toStdString() << "bytes\n";
            break;
        }
        default:{
            if (!file->isOpen()) {
                file->open(QIODevice::WriteOnly);
            }

            QByteArray data = socket->readAll();
            file->write(data);
            file->flush();
            totalBytesRecieved += data.size();
            QString percentage = fileSize == 0 ? "0" : QString::number(100 * ((float)totalBytesRecieved / (float)fileSize), 'f', 1);
            cout << "[ " << percentage.toStdString() << "% ] Received chunk of size: " << data.size() << "\n";
            break;
        }
    }
}

void ReceiveData() {
    socket = Server->nextPendingConnection();
    cout << "Client connected\n";

    QObject::connect(socket, &QTcpSocket::readyRead, readyRead);
    QObject::connect(socket, &QTcpSocket::disconnected, []() {
        cout << "Client disconnected, closing file\n";
        if(file && file->isOpen()) file->close();
        state = STATE::IDLE;
        totalBytesRecieved = 0;
    });
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Server = new QTcpServer();

    if(!Server->listen(QHostAddress::Any, port)) {
        return 1;
    }

    QObject::connect(Server, &QTcpServer::newConnection, ReceiveData);

    return a.exec();
}
