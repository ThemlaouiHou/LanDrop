#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QFileDialog>
#include <QMessageBox>
#include "../config/config.h"

#include "configdialog.h"

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    Config::readFromFile();

    mainLayout = new QVBoxLayout(this);
    formLayout = new QFormLayout();
    pathLayout = new QHBoxLayout();
    buttonLayout = new QVBoxLayout();
    altButtonLayout = new QHBoxLayout();

    downloadPathEdit = new QLabel(this);
    downloadPathEdit->setText(Config::receivedFilesPath);
    browseButton = new QPushButton(this);
    browseButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    browseButton->setFixedSize(30, 30);

    pathLayout->addWidget(downloadPathEdit);
    pathLayout->addWidget(browseButton);

    connect(browseButton, &QPushButton::clicked, this, &ConfigDialog::selectDirectory);

    portEdit = new QLineEdit(this);
    portEdit->setPlaceholderText("e.g. 5554");
    portEdit->setText(QString::number(Config::port));

    bufferEdit = new QLineEdit(this);
    bufferEdit->setPlaceholderText("e.g. 1024");
    bufferEdit->setText(QString::number(Config::bufferSize));

    formLayout->addRow("Download path", pathLayout);
    formLayout->addRow("Port number", portEdit);
    formLayout->addRow("Buffer size", bufferEdit);

    saveButton = new QPushButton("Save", this);
    cancelButton = new QPushButton("Cancel", this);
    resetButton = new QPushButton("Reset to defaults", this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addLayout(altButtonLayout);
    altButtonLayout->addWidget(cancelButton);
    altButtonLayout->addWidget(resetButton);

    connect(saveButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(resetButton, &QPushButton::clicked, this, [this](){
        if (QMessageBox::question(this, "LANDrop - confirmation", "Reset settings to default values?\nThis cannot be undone", QMessageBox::Yes|QMessageBox::No)){
            Config::reset();
            Config::writeToFile();
            downloadPathEdit->setText(Config::receivedFilesPath);
            portEdit->setText(QString::number(Config::port));
            bufferEdit->setText(QString::number(Config::bufferSize));
        }
    });

    setWindowTitle("LANDrop - settings");
}

QString ConfigDialog::getDownloadPath() const
{
    return downloadPathEdit->text();
}

int ConfigDialog::getPort() const
{
    return portEdit->text().toInt();
}

int ConfigDialog::getBufferSize() const
{
    return bufferEdit->text().toInt();
}

void ConfigDialog::selectDirectory() {
    QString directory = QFileDialog::getExistingDirectory(this, "Select a directory");
    if (!directory.isEmpty()) {
        downloadPathEdit->setText(directory);
    }
}
