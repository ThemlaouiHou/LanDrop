/**
 * @file transferitemwidget.cpp
 */

#include "transferitemwidget.h"
#include <QHBoxLayout>
#include <QPalette>

/**
 * @brief Constructs a new transfer item widget.
 *
 * Creates the UI layout with file name label, progress bar, and status label.
 * Applies appropriate styling and sets initial status based on transfer direction.
 *
 * @param fileName Name of the file being transferred
 * @param dir Transfer direction (SEND or RECEIVE)
 * @param parent Parent widget
 */
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

/**
 * @brief Updates the transfer progress and automatically sets status.
 *
 * Updates the progress bar value and automatically changes status.
 *
 * @param percent Progress percentage.
 */
void TransferItemWidget::updateProgress(int percent)
{
    progressBar->setValue(percent);

    if (percent == 100) {
        setStatus(TransferStatus::FINISHED);
    } else {
        setStatus(TransferStatus::IN_PROGRESS);
    }
}

/**
 * @brief Sets the transfer status and updates UI styling accordingly.
 *
 * @param status New transfer status to set
 */
void TransferItemWidget::setStatus(const TransferStatus status)
{
    this->status = status;
    if (status == TransferStatus::FINISHED) {
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
    else if (status == TransferStatus::IN_PROGRESS) {
        statusLabel->setText("Status : In progress");
        statusLabel->setStyleSheet("color: blue; font-weight: bold;");
    }
    else if (status == TransferStatus::CANCELLED) {
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
    else if (status == TransferStatus::ERROR) {
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

/**
 * @brief Returns the name of the file being transferred.
 *
 * @return File name as QString
 */
QString TransferItemWidget::getFileName() const
{
    return fileName;
}

/**
 * @brief Returns the current transfer status.
 *
 * @return Current transfer status
 */
TransferStatus TransferItemWidget::getStatus() const
{
    return status;
}
