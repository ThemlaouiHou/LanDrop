// TransferHistoryWidget.h
#ifndef TRANSFERHISTORYWIDGET_H
#define TRANSFERHISTORYWIDGET_H

#include <QWidget>
#include <QMap>

class QScrollArea;
class QVBoxLayout;
class QPushButton;
class TransferItemWidget;

class TransferHistoryWidget : public QWidget {
    Q_OBJECT
public:
    enum TransferStatus { WAITING, IN_PROGRESS, FINISHED, CANCELLED, ERROR };

    explicit TransferHistoryWidget(QWidget *parent = nullptr);

    void addTransferItem(TransferItemWidget *item);
    void updateProgress(int id, int percent);
    void setStatus(int id, TransferStatus status);
    QMap<int, TransferItemWidget*> items;

private:
    QScrollArea      *scrollArea;
    QWidget          *container;
    QVBoxLayout      *itemsLayout;
    QPushButton      *openFolderButton;
};

#endif // TRANSFERHISTORYWIDGET_H
