/**
 * @file sendfilewidget.cpp
 * @brief File selection and sending interface widget
 */
#include "sendfilewidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QLabel>
#include <QButtonGroup>
#include <QMessageBox>
#include <QFileInfo>
#include <QListWidget>
#include <QSpacerItem>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QWidget>
#include <QListWidgetItem>
#include "transferitemwidget.h"

/**
 * @brief Constructs the send file widget
 * @param historyWidget Reference to transfer history for progress tracking
 * @param parent Parent widget
 */
SendFileWidget::SendFileWidget(TransferHistoryWidget *historyWidget, QWidget *parent)
    : QWidget(parent), historyWidget(historyWidget)
{
    // --- Widget Layout Setup ---
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *title = new QLabel("Send file");
    title->setStyleSheet("font-weight: bold; font-size: 16px;");

    QPushButton *selectFileButton = new QPushButton("Select file");
    selectFileButton->setStyleSheet(Config::bigButtonStyleSheet);

    // list widget with scroll
    fileListWidget = new QListWidget();
    fileListWidget->setStyleSheet("font-size: 12px; color: gray;");
    fileListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    fileListWidget->setFixedHeight(300);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidget(fileListWidget);
    scrollArea->setWidgetResizable(true);

    // Recipient IP address input
    recipientInput = new QLineEdit;
    recipientInput->setPlaceholderText("Recipient address");
    recipientInput->setMinimumHeight(30);

    QPushButton *sendButton = new QPushButton("SEND");
    sendButton->setStyleSheet(Config::bigButtonStyleSheet);

    // --- File Selection Handler ---
    QObject::connect(selectFileButton, &QPushButton::clicked, this, [this, layout, scrollArea]() {
        QStringList filePaths = QFileDialog::getOpenFileNames(this, "Select Files", "", "All Files (*)");

        if (!filePaths.isEmpty()) {
            for (const QString &filePath : filePaths) {
                QFileInfo fileInfo(filePath);
                // Validate file exists and is not a directory
                if (!fileInfo.exists() || fileInfo.isDir()) {
                    QMessageBox::critical(this, "Error", "One of the selected files does not exist or is a directory.");
                    return;
                }

                // custom list item widget with filename and remove button
                QWidget *itemWidget = new QWidget();
                QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
                itemLayout->setContentsMargins(0, 0, 0, 0);

                QLabel *fileLabel = new QLabel(fileInfo.fileName());
                QPushButton *deleteButton = new QPushButton("X");
                deleteButton->setStyleSheet("background-color: red; color: white; border: none; padding: 0px;");
                deleteButton->setMinimumSize(15, 15);
                deleteButton->setMaximumSize(15, 15);

                itemLayout->addWidget(fileLabel);
                itemLayout->addWidget(deleteButton);

                // Add item to list widget
                QListWidgetItem *listItem = new QListWidgetItem();
                listItem->setSizeHint(itemWidget->sizeHint());
                listItem->setData(Qt::UserRole, filePath);  // Store full path for sending
                fileListWidget->addItem(listItem);
                fileListWidget->setItemWidget(listItem, itemWidget);

                // Connect remove button to delete specific item
                QObject::connect(deleteButton, &QPushButton::clicked, this, [this, itemWidget, listItem, scrollArea]() {
                    fileListWidget->takeItem(fileListWidget->row(listItem));
                    delete itemWidget;

                    fileListWidget->update();
                });
            }

            layout->addWidget(scrollArea);
        }
    });

    connect(sendButton, &QPushButton::clicked, this, [this, historyWidget]() {
        // 1) Verify there are files
        if (fileListWidget->count() < 1) {
            QMessageBox::warning(this, "LANDrop - error", "Choose a file before!");
            return;
        }

        // 2) Retrieve and parses IP
        QString text = recipientInput->text().trimmed();
        if (text.isEmpty()) {
            QMessageBox::warning(this, "LANDrop - error", "Missing recipient address");
            return;
        }
        QStringList tokens = text.split(',', Qt::SkipEmptyParts);
        QStringList ips;
        for (QString t : tokens) {
            QString ip = t.trimmed();
            if (ips.size() >= 10) break;            // max 10
            if (!QHostAddress(ip).toIPv4Address()) {
                QMessageBox::warning(this, "LANDrop - error", QString("Invalid IP address skipped: %1").arg(ip));
                continue;
            }
            ips << ip;
        }
        if (ips.isEmpty()) {
            QMessageBox::warning(this, "LANDrop - error", "No valid IP addresses provided");
            return;
        }

        // 3) Retrieves files
        QStringList paths;
        for (int i = 0; i < fileListWidget->count(); ++i) {
            paths << fileListWidget->item(i)->data(Qt::UserRole).toString();
        }
        fileListWidget->clear();

        // 4) For each file AND each IP, create an item + sender
        for (const QString &filePath : paths) {
            QFileInfo fi(filePath);
            for (const QString &ip : ips) {
                // a) History item
                auto *item = new TransferItemWidget(
                    fi.fileName() + QString(" @%1").arg(ip),
                    TransferItemWidget::TransferDirection::SEND,
                    this);
                historyWidget->addTransferItem(item);
                item->setStatus(TransferHistoryWidget::TransferStatus::WAITING);

                // b) Sender
                Sender *sender = new Sender(this);
                connect(sender, &Sender::transferAccepted, this, [item]() {
                    item->setStatus(TransferHistoryWidget::TransferStatus::IN_PROGRESS);
                });
                connect(sender, &Sender::transferRefused, this, [item]() {
                    item->setStatus(TransferHistoryWidget::TransferStatus::CANCELLED);
                });
                connect(sender, &Sender::progressUpdated, item, &TransferItemWidget::updateProgress);
                connect(sender, &Sender::transferFinished, this, [item]() {
                    item->setStatus(TransferHistoryWidget::TransferStatus::FINISHED);
                });
                connect(sender, &Sender::transferError, this, [item]() {
                    if (item->getStatus() != TransferHistoryWidget::TransferStatus::FINISHED &&
                        item->getStatus() != TransferHistoryWidget::TransferStatus::CANCELLED)
                    {
                        item->setStatus(TransferHistoryWidget::TransferStatus::ERROR);
                    }
                });

                // c) Launch the transfer
                sender->sendFile(filePath, ip);
            }
        }
    });


    layout->addWidget(title);
    layout->addWidget(selectFileButton);
    layout->addItem(new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Fixed));
    layout->addWidget(new QLabel("Recipient"));
    layout->addWidget(recipientInput);
    layout->addItem(new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Fixed));
    layout->addWidget(sendButton);
    layout->addItem(new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Fixed));
    layout->addWidget(new QLabel("Selected files"));
    layout->addWidget(scrollArea);

    setLayout(layout);
    setStyleSheet(R"(
        QListWidget, QTextEdit {
            background-color: white;
            border: 1px solid #64b5f6;
        }
    )");
}

void SendFileWidget::setRecipientAddress(const QString &address) {
    recipientInput->setText(address);
}

