/**
 * @file transferhistorywidget.cpp
 */

#include "transferhistorywidget.h"
#include "transferitemwidget.h"
#include "../config/config.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QLabel>
#include <QDebug>

/**
 * @brief Construct the transfer history widget
 *
 * @param parent
 */
TransferHistoryWidget::TransferHistoryWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Title
    QLabel *title = new QLabel("Transfer history", this);
    title->setStyleSheet("font-weight: bold; font-size: 16px;");
    mainLayout->addWidget(title);

    // Scrollable area
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    container = new QWidget;
    itemsLayout = new QVBoxLayout(container);
    itemsLayout->setAlignment(Qt::AlignTop);
    container->setLayout(itemsLayout);
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea, 1);

    // Reception folder button
    openFolderButton = new QPushButton("Open reception folder", this);
    openFolderButton->setStyleSheet(Config::getButtonStyleSheet());
    openFolderButton->setFixedHeight(30);
    mainLayout->addWidget(openFolderButton);

    connect(openFolderButton, &QPushButton::clicked, this, []()
            {
        const QString folder = Config::getReceivedFilesPath();      
        QDir().mkpath(folder);                                 
        QDesktopServices::openUrl(QUrl::fromLocalFile(folder)); });
}

/**
 * @brief Adds a new transfer item to the history widget
 *
 * @param sessionId Unique identifier for the transfer session
 * @param item Transfer item widget to be added
 */
void TransferHistoryWidget::addTransferItem(int sessionId, TransferItemWidget *item)
{
    items[sessionId] = item;
    itemsLayout->insertWidget(0, item);
}

/**
 * @brief Updates the progress of a specific transfer item
 *
 * @param id Unique identifier for the transfer session
 * @param percent Progress percentage (0-100)
 */
void TransferHistoryWidget::updateProgress(int id, int percent)
{
    if (items.contains(id))
        items[id]->updateProgress(percent);
}

/**
 * @brief Sets the status of a specific transfer item
 *
 * @param id Unique identifier for the transfer session
 * @param status New status to set for the transfer item
 */
void TransferHistoryWidget::setStatus(int id, TransferStatus status)
{
    if (items.contains(id))
        items[id]->setStatus(status);
}
