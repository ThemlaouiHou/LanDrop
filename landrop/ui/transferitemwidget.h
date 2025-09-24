#ifndef TRANSFERITEMWIDGET_H
#define TRANSFERITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QString>
#include "transferhistorywidget.h"

class TransferItemWidget : public QWidget
{
    Q_OBJECT

public:
    enum class TransferDirection { SEND, RECEIVE };

    explicit TransferItemWidget(const QString &fileName, TransferDirection dir, QWidget *parent = nullptr);

    void updateProgress(int percent);
    void setStatus(const TransferHistoryWidget::TransferStatus status);
    TransferHistoryWidget::TransferStatus getStatus() const;

    QString getFileName() const;

private:
    QString fileName;
    QLabel *fileNameLabel;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    TransferHistoryWidget::TransferStatus status = TransferHistoryWidget::TransferStatus::WAITING;
    TransferDirection direction;
    QLabel *directionLabel;
};

#endif // TRANSFERITEMWIDGET_H
