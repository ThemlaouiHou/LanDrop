#include "transferitemwidget.h"
#include <QHBoxLayout>
#include <QPalette>

TransferItemWidget::TransferItemWidget(const QString &fileName, TransferDirection dir, QWidget *parent)
    : QWidget(parent), fileName(fileName), direction(dir)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(10, 5, 10, 5);

    fileNameLabel = new QLabel(fileName, this);
    fileNameLabel->setStyleSheet("font-weight: bold;");

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet(R"(
        QProgressBar {
            border: 1px solid gray;
            border-radius: 5px;
            text-align: center;
            height: 20px;
        }
        QProgressBar::chunk {
            background-color: #66aaff;
            width: 20px;
        }
    )");

    statusLabel = new QLabel("Status : Waiting", this);
    statusLabel->setStyleSheet("color: gray; font-weight: bold;");

    if (direction == TransferDirection::SEND) {
        statusLabel->setText("Status : Waiting     Type : Sending");
    } else {
        statusLabel->setText("Status : Waiting      Type : Reception");
    }

    mainLayout->addWidget(fileNameLabel);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(statusLabel);
    setLayout(mainLayout);
}

void TransferItemWidget::updateProgress(int percent)
{
    progressBar->setValue(percent);

    if (percent == 100) {
        setStatus(TransferHistoryWidget::TransferStatus::FINISHED);
    } else {
        setStatus(TransferHistoryWidget::TransferStatus::IN_PROGRESS);
    }
}

void TransferItemWidget::setStatus(const TransferHistoryWidget::TransferStatus status)
{
    this->status = status;
    if (status == TransferHistoryWidget::TransferStatus::FINISHED) {
        statusLabel->setText("Status : Finished");
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
        if (direction == TransferDirection::SEND) {
            statusLabel->setText("Status : Finished           Type : Sending");
        } else {
            statusLabel->setText("Status : Finished           Type  : Reception ");
        }
        progressBar->setValue(100);
        progressBar->setStyleSheet(R"(
            QProgressBar {
                border: 1px solid gray;
                border-radius: 5px;
                text-align: center;
                height: 20px;
            }
            QProgressBar::chunk {
                background-color: #4CAF50;
                width: 20px;
            }
        )");
    }
    else if (status == TransferHistoryWidget::TransferStatus::IN_PROGRESS) {
        statusLabel->setText("Status : In progress");
        statusLabel->setStyleSheet("color: blue; font-weight: bold;");
    }
    else if (status == TransferHistoryWidget::TransferStatus::CANCELLED) {
        statusLabel->setText("Status : Cancelled");
        statusLabel->setStyleSheet("color: red; font-weight: bold;");
        if (direction == TransferDirection::SEND) {
            statusLabel->setText("Status : Cancelled         Type: Sending");
        } else {
            statusLabel->setText("Status : Cancelled         Type : Reception");
        }
        progressBar->setStyleSheet(R"(
            QProgressBar {
                border: 1px solid gray;
                border-radius: 5px;
                text-align: center;
                height: 20px;
                background-color: red;
            }
            QProgressBar::chunk {
                width: 20px;
                background-color: red;
            }
        )");
    }
    else if (status == TransferHistoryWidget::TransferStatus::ERROR) {
        statusLabel->setText("Status : Error");
        statusLabel->setStyleSheet("color: red; font-weight: bold;");
        if (direction == TransferDirection::SEND) {
            statusLabel->setText("Status : Error           Type : Sending");
        } else {
            statusLabel->setText("Status : Error           Type : Reception");
        }
        progressBar->setStyleSheet(R"(
            QProgressBar {
                border: 1px solid gray;
                border-radius: 5px;
                text-align: center;
                height: 20px;
                background-color: red;
            }
            QProgressBar::chunk {
                width: 20px;
                background-color: red;
            }
        )");
    }
    else {
        statusLabel->setText("Status : Waiting");
        statusLabel->setStyleSheet("color: gray; font-weight: bold;");
    }
}

QString TransferItemWidget::getFileName() const
{
    return fileName;
}

TransferHistoryWidget::TransferStatus TransferItemWidget::getStatus() const
{
    return status;
}
