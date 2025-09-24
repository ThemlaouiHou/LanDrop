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

TransferHistoryWidget::TransferHistoryWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10,10,10,10);
    mainLayout->setSpacing(8);

    // Titre
    QLabel *title = new QLabel("Transfer history", this);
    title->setStyleSheet("font-weight: bold; font-size: 16px;");
    mainLayout->addWidget(title);

    // Zone scrollable
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    container = new QWidget;
    itemsLayout = new QVBoxLayout(container);
    itemsLayout->setAlignment(Qt::AlignTop);
    container->setLayout(itemsLayout);
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea, /* stretch */ 1);

    // Bouton ouvrir dossier
    openFolderButton = new QPushButton("Open the received files folder", this);
    openFolderButton->setStyleSheet(Config::buttonStyleSheet);
    openFolderButton->setFixedHeight(30);
    mainLayout->addWidget(openFolderButton);

    connect(openFolderButton, &QPushButton::clicked, this, []() {
        const QString folder = Config::receivedFilesPath;      // corrected name
        QDir().mkpath(folder);                                 // ensures the existence
        QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
    });

}

void TransferHistoryWidget::addTransferItem(TransferItemWidget *item)
{
    int id = items.count();
    items[id] = item;
    itemsLayout->addWidget(item);
}

void TransferHistoryWidget::updateProgress(int id, int percent)
{
    if (items.contains(id))
        items[id]->updateProgress(percent);
}

void TransferHistoryWidget::setStatus(int id, TransferStatus status)
{
    if (items.contains(id))
        items[id]->setStatus(status);
}
