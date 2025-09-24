#include "sharedfileswidget.h"
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>

SharedFilesWidget::SharedFilesWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout;
    QLabel *title = new QLabel("Shared Files");
    title->setStyleSheet("font-weight: bold; font-size: 16px;");

    QListWidget *listWidget = new QListWidget(this);
    listWidget->addItems({"file.txt", "file.txt", "file.txt", "file.txt", "file.txt", "file.txt"});

    layout->addWidget(title);
    layout->addWidget(listWidget);
    setLayout(layout);
}
