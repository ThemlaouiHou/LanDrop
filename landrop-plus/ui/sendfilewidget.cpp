/**
 * @file sendfilewidget.cpp
 */

#include "sendfilewidget.h"
#include "../config/config.h"
#include "transferitemwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QHostAddress>
#include <QSpacerItem>

/**
 * @brief Constructs a new SendFileWidget.
 *
 * Initializes the widget with references to the transfer history and file transfer manager,
 * then sets up the interface.
 *
 * @param historyWidget Reference to the transfer history widget for tracking
 * @param transferManager Manager for coordinating file transfer operations
 * @param parent Parent widget
 */
SendFileWidget::SendFileWidget(TransferHistoryWidget *historyWidget, FileTransferManager *transferManager, QWidget *parent)
    : QWidget(parent),
      historyWidget(historyWidget),
      transferManager(nullptr)
{
    if (transferManager) {
        this->transferManager = transferManager;
    }
    setupUI();
}

/**
 * @brief Sets up the user interface layout and components.
 */
void SendFileWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel("Send file");
    title->setStyleSheet("font-weight: bold; font-size: 16px;");

    QPushButton *selectFileButton = new QPushButton("Select file");
    selectFileButton->setStyleSheet(Config::getButtonStyleSheet());

    fileListWidget = new QListWidget();
    fileListWidget->setStyleSheet("font-size: 12px; color: gray;");
    fileListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    fileListWidget->setFixedHeight(300);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidget(fileListWidget);
    scrollArea->setWidgetResizable(true);

    recipientInput = new QLineEdit;
    recipientInput->setPlaceholderText("Recipient address");
    recipientInput->setMinimumHeight(30);

    QPushButton *sendButton = new QPushButton("SEND");
    sendButton->setStyleSheet(Config::getButtonStyleSheet());

    connect(selectFileButton, &QPushButton::clicked, this, &SendFileWidget::onSelectFilesClicked);
    connect(sendButton, &QPushButton::clicked, this, &SendFileWidget::onSendButtonClicked);

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

    setStyleSheet(R"(
        QListWidget, QTextEdit {
            background-color: white;
            border: 1px solid #64b5f6;
        }
    )");
}

/**
 * @brief Handles file selection button clicks.
 *
 * Opens a file dialog for multiple file selection, and adds them to the list widget
 * with individual delete buttons.
 * Only allows regular files (no directories) to be selected.
 */
void SendFileWidget::onSelectFilesClicked()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, "Select Files", "", "All Files (*)");

    if (filePaths.isEmpty())
        return;

    for (const QString &filePath : filePaths)
    {
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists() || fileInfo.isDir())
        {
            QMessageBox::critical(this, "Error", "One of the selected files does not exist or is a directory.");
            return;
        }

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

        QListWidgetItem *listItem = new QListWidgetItem();
        listItem->setSizeHint(itemWidget->sizeHint());
        listItem->setData(Qt::UserRole, filePath);
        fileListWidget->addItem(listItem);
        fileListWidget->setItemWidget(listItem, itemWidget);

        connect(deleteButton, &QPushButton::clicked, this, [this, listItem, itemWidget]()
                {
            fileListWidget->takeItem(fileListWidget->row(listItem));
            delete itemWidget;
            fileListWidget->update(); });
    }
}

/**
 * @brief Handles send button clicks to initiate file transfers.
 *
 * Validates that files are selected and recipients are specified, processes
 * recipient IP addresses, and initiates the file transfer operation through
 * the FileTransferManager. Supports both discovered users (with correct ports)
 * and manually entered IP addresses (using default port).
 */
void SendFileWidget::onSendButtonClicked()
{
    if (fileListWidget->count() < 1)
    {
        QMessageBox::warning(this, "LANDrop - error", "Choose a file before!");
        return;
    }

    QString text = recipientInput->text().trimmed();
    if (text.isEmpty())
    {
        QMessageBox::warning(this, "LANDrop - error", "Missing recipient address");
        return;
    }

    QStringList recipientIPs = validateRecipients(text);
    if (recipientIPs.isEmpty())
    {
        QMessageBox::warning(this, "LANDrop - error", "No valid IP addresses provided");
        return;
    }

    QStringList filePaths = getSelectedFilePaths();
    clearFileList();

    // Build user list with port information
    QList<LANDropUser> usersWithPorts;
    for (const QString &ip : recipientIPs)
    {
        if (selectedUsers.contains(ip))
        {
            // Use discovered user info with correct port
            usersWithPorts.append(selectedUsers[ip]);
        }
        else
        {
            // Fallback: create user with default port for manually entered IPs
            LANDropUser fallbackUser(ip, ip, Config::getPort(), "unknown");
            usersWithPorts.append(fallbackUser);
        }
    }

    transferManager->sendFilesToUsers(filePaths, usersWithPorts);
}

/**
 * @brief Validates and parses recipient IP addresses from user input.
 *
 * Parses comma-separated IP addresses, validates each one using QHostAddress,
 * and returns a list of valid IPv4 addresses.
 *
 * @param input Comma-separated string of IP addresses
 * @return List of valid IP addresses
 */
QStringList SendFileWidget::validateRecipients(const QString &input) const
{
    QStringList tokens = input.split(',', Qt::SkipEmptyParts);
    QStringList validIPs;

    for (const QString &token : tokens)
    {
        QString ip = token.trimmed();
        if (validIPs.size() >= 10)
            break; // max 10 recipients

        if (!QHostAddress(ip).toIPv4Address())
        {
            QMessageBox::warning(const_cast<SendFileWidget *>(this), "LANDrop - error",
                                 QString("Invalid IP address skipped: %1").arg(ip));
            continue;
        }
        validIPs << ip;
    }

    return validIPs;
}

/**
 * @brief Retrieves file paths of all selected files.
 *
 * Extracts the file paths stored in the UserRole data of each list widget item.
 *
 * @return List of absolute file paths for selected files
 */
QStringList SendFileWidget::getSelectedFilePaths() const
{
    QStringList paths;
    for (int i = 0; i < fileListWidget->count(); ++i)
    {
        paths << fileListWidget->item(i)->data(Qt::UserRole).toString();
    }
    return paths;
}

/**
 * @brief Clears all files from the file selection list.
 */
void SendFileWidget::clearFileList()
{
    fileListWidget->clear();
}

/**
 * @brief Sets the recipient user when a user is selected from the user list.
 *
 * Stores the complete user information for port lookup and sets the IP address
 * in the recipient input field.
 *
 * @param user Complete LANDropUser information including transfer port
 */
void SendFileWidget::setRecipientUser(const LANDropUser &user)
{
    // Store user info for port lookup
    selectedUsers[user.ipAddress] = user;

    // Set the IP in the input field
    recipientInput->setText(user.ipAddress);
}

/**
 * @brief Sets or updates the file transfer manager reference.
 *
 * @param manager Pointer to the FileTransferManager instance
 */
void SendFileWidget::setTransferManager(FileTransferManager *manager)
{
    if (manager) {
        transferManager = manager;
    }
}