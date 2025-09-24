#ifndef SENDFILEWIDGET_H
#define SENDFILEWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QFileInfo>
#include <QLineEdit>
#include <QProgressBar>
#include <QScrollArea>
#include <QVBoxLayout>
#include "../network/sender.h"
#include "transferhistorywidget.h"

class TransferHistoryWidget;
class TransferItemWidget;

class SendFileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SendFileWidget(TransferHistoryWidget *historyWidget, QWidget *parent = nullptr);

public slots:
    void setRecipientAddress(const QString &address);

private:
    QListWidget *fileListWidget;
    QFileInfo *fileInfo;
    QLineEdit *recipientInput;
    QScrollArea *scrollArea;
    TransferHistoryWidget *historyWidget;
};

#endif // SENDFILEWIDGET_H
