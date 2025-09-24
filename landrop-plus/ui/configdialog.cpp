/**
 * @file configdialog.cpp
 */

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QFileDialog>
#include <QMessageBox>
#include "../config/config.h"

#include "configdialog.h"

/**
 * @brief Constructs a new configuration dialog.
 *
 * Creates and initializes the settings dialog with current configuration values.
 *
 * @param parent Parent widget for the dialog
 */
ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    Config::readFromFile();

    mainLayout = new QVBoxLayout(this);
    formLayout = new QFormLayout();
    downloadPathLayout = new QHBoxLayout();
    buttonLayout = new QVBoxLayout();
    altButtonLayout = new QHBoxLayout();

    downloadPathEdit = new QLabel(this);
    downloadPathEdit->setText(Config::getReceivedFilesPath());
    downloadBrowseButton = new QPushButton(this);
    downloadBrowseButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    downloadBrowseButton->setFixedSize(30, 30);

    downloadPathLayout->addWidget(downloadPathEdit);
    downloadPathLayout->addWidget(downloadBrowseButton);

    connect(downloadBrowseButton, &QPushButton::clicked, this, &ConfigDialog::selectDownloadDirectory);

    portEdit = new QLineEdit(this);
    portEdit->setPlaceholderText("e.g. 5554");
    portEdit->setText(QString::number(Config::getPort()));

    bufferEdit = new QLineEdit(this);
    bufferEdit->setPlaceholderText("e.g. 1024");
    bufferEdit->setText(QString::number(Config::getBufferSize()));

    formLayout->addRow("Download path", downloadPathLayout);
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
    connect(resetButton, &QPushButton::clicked, this, [this]()
            {
        if (QMessageBox::question(this, "LANDrop - confirmation", "Reset settings to default values?\nThis cannot be undone", QMessageBox::Yes|QMessageBox::No)){
            Config::reset();
            Config::writeToFile();
            downloadPathEdit->setText(Config::getReceivedFilesPath());
            portEdit->setText(QString::number(Config::getPort()));
            bufferEdit->setText(QString::number(Config::getBufferSize()));
        } });

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

/**
 * @brief Opens a directory selection dialog for choosing the download path.
 *
 * Displays a native directory browser dialog and updates the download path.
 */
void ConfigDialog::selectDownloadDirectory()
{
    QString directory = QFileDialog::getExistingDirectory(this, "Select download directory");
    if (!directory.isEmpty())
    {
        downloadPathEdit->setText(directory);
    }
}
