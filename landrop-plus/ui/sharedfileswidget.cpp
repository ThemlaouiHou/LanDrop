/**
 * @file sharedfileswidget.cpp
 */

#include "sharedfileswidget.h"
#include "../services/sharedfilemanager.h"
#include "../config/config.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QApplication>
#include <QStyle>
#include <QJsonDocument>
#include <QDebug>

/**
 * @brief Constructs a new SharedFilesWidget.
 *
 * Initializes the widget and sets up the user interface components.
 *
 * @param parent Parent widget
 */
SharedFilesWidget::SharedFilesWidget(QWidget *parent)
    : QWidget(parent), sharedFileManager(nullptr)
{
    setupUI();
}

/**
 * @brief Sets up the user interface layout and components.
 *
 * Creates the tree widget for displaying shared files, configures headers,
 * sets up buttons for download and folder operations, and establishes
 * signal-slot connections for user interactions.
 */
void SharedFilesWidget::setupUI()
{
    setMinimumWidth(150);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Title
    QLabel *title = new QLabel("Network Shared Files", this);
    title->setStyleSheet("font-weight: bold; font-size: 16px;");
    mainLayout->addWidget(title);

    // Status label
    statusLabel = new QLabel("Discovering shared files...", this);
    statusLabel->setStyleSheet("color: gray; font-style: italic;");
    mainLayout->addWidget(statusLabel);

    // Tree widget for files - more compact
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderLabels({"Name", "Size", "Type"});
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setRootIsDecorated(true);
    treeWidget->setSortingEnabled(true);
    treeWidget->setUniformRowHeights(true);
    treeWidget->header()->setStretchLastSection(false);
    treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    treeWidget->header()->setDefaultSectionSize(60);

    connect(treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &SharedFilesWidget::onItemDoubleClicked);
    connect(treeWidget, &QTreeWidget::itemSelectionChanged,
            this, [this]()
            {
                QTreeWidgetItem *item = treeWidget->currentItem();
                downloadButton->setEnabled(item && item->data(0, IsDownloadableRole).toBool()); });

    mainLayout->addWidget(treeWidget, 1);

    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(4);

    downloadButton = new QPushButton("Download", this);
    downloadButton->setStyleSheet(Config::getButtonStyleSheet());
    downloadButton->setEnabled(false);
    downloadButton->setMaximumHeight(28);
    connect(downloadButton, &QPushButton::clicked, this, &SharedFilesWidget::onDownloadButtonClicked);

    openFolderButton = new QPushButton("Shared Folder", this);
    openFolderButton->setStyleSheet(Config::getButtonStyleSheet());
    openFolderButton->setMaximumHeight(28);
    connect(openFolderButton, &QPushButton::clicked, this, &SharedFilesWidget::onOpenSharedFolderClicked);

    refreshButton = new QPushButton("Refresh", this);
    refreshButton->setStyleSheet(Config::getButtonStyleSheet());
    refreshButton->setMaximumHeight(28);
    connect(refreshButton, &QPushButton::clicked, this, &SharedFilesWidget::onRefreshClicked);

    buttonLayout->addWidget(downloadButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(openFolderButton);
    buttonLayout->addWidget(refreshButton);

    mainLayout->addLayout(buttonLayout);
}

/**
 * @brief Sets the shared file manager instance.
 *
 * @param manager Pointer to the SharedFileManager instance
 */
void SharedFilesWidget::setSharedFileManager(SharedFileManager *manager)
{
    sharedFileManager = manager;
}

/**
 * @brief Handles updates to the list of discovered users.
 *
 * Called when the BroadcastDiscoveryService discovers new users or updates
 * existing user information.
 *
 * @param users List of currently discovered LANDrop users
 */
void SharedFilesWidget::onUserListUpdated(const QList<LANDropUser> &users)
{
    qDebug() << "SharedFilesWidget: Received user list with" << users.size() << "users";

    discoveredUsers = users;
    populateUserFiles();
}

/**
 * @brief Rebuilds the shared files tree with current user data.
 *
 * Clears the existing tree and repopulates it with shared files from all
 * discovered users. Preserves the current selection if the previously
 * selected file is still available after the refresh.
 */
void SharedFilesWidget::populateUserFiles()
{
    qDebug() << "SharedFilesWidget: Rebuilding shared files tree";

    // Save current selection
    QString selectedUserIP;
    QString selectedFilePath;

    QTreeWidgetItem *selectedItem = treeWidget->currentItem();
    if (selectedItem && selectedItem->data(0, IsDownloadableRole).toBool())
    {
        selectedUserIP = selectedItem->data(0, UserIPRole).toString();
        selectedFilePath = selectedItem->data(0, FilePathRole).toString();
        qDebug() << "SharedFilesWidget: Preserving selection:" << selectedUserIP << selectedFilePath;
    }

    treeWidget->clear();

    int totalFiles = 0;
    int usersWithFiles = 0;

    for (const LANDropUser &user : discoveredUsers)
    {
        if (user.hasSharedFiles())
        {
            addUserToTree(user);
            totalFiles += user.sharedFileCount();
            usersWithFiles++;
        }
    }

    // Update status
    QString currentSharedFolder = QDir::current().absoluteFilePath("./Shared Files");

    if (usersWithFiles == 0)
    {
        statusLabel->setText(QString("No shared files found on network\nYour shared folder: %1").arg(currentSharedFolder));
    }
    else
    {
        statusLabel->setText(QString("%1 files shared by %2 users\nYour shared folder: %3")
                                 .arg(totalFiles)
                                 .arg(usersWithFiles)
                                 .arg(currentSharedFolder));
    }

    // Expand all user nodes by default
    treeWidget->expandAll();

    // Try to restore previous selection
    if (!selectedUserIP.isEmpty() && !selectedFilePath.isEmpty())
    {
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *userItem = treeWidget->topLevelItem(i);
            for (int j = 0; j < userItem->childCount(); ++j)
            {
                QTreeWidgetItem *child = userItem->child(j);
                if (child->data(0, UserIPRole).toString() == selectedUserIP &&
                    child->data(0, FilePathRole).toString() == selectedFilePath)
                {
                    treeWidget->setCurrentItem(child);
                    treeWidget->scrollToItem(child);
                    qDebug() << "SharedFilesWidget: Restored selection";
                    return;
                }
            }
        }
        qDebug() << "SharedFilesWidget: Previous selection not found in refreshed tree.";
    }
}

/**
 * @brief Adds a user and their shared files to the tree widget.
 *
 * Creates a top-level item for the user with styling and adds all their
 * shared files as child items.
 *
 * @param user LANDropUser containing shared file information
 */
void SharedFilesWidget::addUserToTree(const LANDropUser &user)
{
    // Create user root item
    QTreeWidgetItem *userItem = new QTreeWidgetItem(treeWidget);
    userItem->setText(0, QString("📁 %1 (%2)").arg(user.hostname, user.ipAddress));
    userItem->setText(1, "");
    userItem->setText(2, "User");
    userItem->setData(0, UserIPRole, user.ipAddress);
    userItem->setData(0, IsDownloadableRole, false);

    // Style user item
    QFont font = userItem->font(0);
    font.setBold(true);
    userItem->setFont(0, font);
    userItem->setBackground(0, QColor(240, 248, 255));

    // Add shared files
    for (const QJsonValue &fileValue : user.sharedFiles)
    {
        if (fileValue.isObject())
        {
            addFileToUser(userItem, fileValue.toObject(), user.ipAddress, user.transferPort);
        }
    }

    // Set file count in user item
    userItem->setText(1, QString("(%1 files)").arg(user.sharedFileCount()));
}

/**
 * @brief Adds a file item as a child of a user item.
 *
 * @param userItem Parent user item in the tree
 * @param fileInfo JSON object containing file metadata
 * @param userIP IP address of the file owner
 * @param userPort Port number for file transfer
 */
void SharedFilesWidget::addFileToUser(QTreeWidgetItem *userItem, const QJsonObject &fileInfo, const QString &userIP, quint16 userPort)
{
    QTreeWidgetItem *fileItem = createFileItem(fileInfo, userIP, userPort);
    userItem->addChild(fileItem);
}

/**
 * @brief Creates a tree widget item for a shared file.
 *
 * Parses file information from JSON and creates a configured
 * tree item with appropriate icons, metadata, and styling.
 *
 * @param fileInfo JSON object containing file metadata
 * @param userIP IP address of the file owner
 * @param userPort Port number for file transfer
 * @return Configured QTreeWidgetItem for the file
 */
QTreeWidgetItem *SharedFilesWidget::createFileItem(const QJsonObject &fileInfo, const QString &userIP, quint16 userPort)
{
    QTreeWidgetItem *item = new QTreeWidgetItem();

    QString name = fileInfo["name"].toString();
    QString type = fileInfo["type"].toString();
    qint64 size = fileInfo["size"].toString().toLongLong();
    QString relativePath = fileInfo["path"].toString();

    // Set item data
    item->setText(0, (type == "directory" ? "📁 " : "📄 ") + name);
    item->setText(1, type == "directory" ? "" : formatFileSize(size));
    item->setText(2, type == "directory" ? "Folder" : "File");

    // Store metadata
    item->setData(0, UserIPRole, userIP);
    item->setData(0, UserPortRole, userPort);
    item->setData(0, FilePathRole, relativePath);
    item->setData(0, FileTypeRole, type);
    item->setData(0, IsDownloadableRole, true);

    return item;
}

/**
 * @brief Handles double-click events on tree items.
 *
 * Initiates download for downloadable items when double-clicked.
 *
 * @param item The tree item that was double-clicked
 * @param column The column that was double-clicked (unused)
 */
void SharedFilesWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    if (item && item->data(0, IsDownloadableRole).toBool())
    {
        onDownloadButtonClicked();
    }
}

/**
 * @brief Handles download button clicks and double-click downloads.
 *
 * Extracts file information from the selected tree item and emits a
 * download request signal with the necessary transfer details.
 */
void SharedFilesWidget::onDownloadButtonClicked()
{
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (!item || !item->data(0, IsDownloadableRole).toBool())
    {
        return;
    }

    QString userIP = item->data(0, UserIPRole).toString();
    quint16 userPort = item->data(0, UserPortRole).toUInt();
    QString relativePath = item->data(0, FilePathRole).toString();
    QString fileName = item->text(0).mid(2).trimmed(); // Remove emoji prefix and trim spaces

    /**
    qDebug() << "SharedFilesWidget: Download requested:";
    qDebug() << "  - IP:" << userIP;
    qDebug() << "  - Port:" << userPort;
    qDebug() << "  - relativePath:" << relativePath;
    qDebug() << "  - fileName:" << fileName;
    qDebug() << "  - original text:" << item->text(0);
    */

    emit downloadRequested(userIP, userPort, relativePath, fileName);
}

/**
 * @brief Opens the local shared files folder in the system file manager.
 */
void SharedFilesWidget::onOpenSharedFolderClicked()
{
    QString folderPath = QDir::current().absoluteFilePath("./Shared Files");
    QDir().mkpath(folderPath); // Ensure folder exists
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

/**
 * @brief Handles refresh button clicks.
 *
 * Manually triggers a refresh of the shared files display by rebuilding
 * the tree with current user data.
 */
void SharedFilesWidget::onRefreshClicked()
{
    populateUserFiles();
}

/**
 * @brief Formats file size in bytes to readable format.
 *
 * @param bytes File size in bytes
 * @return Formatted size string with appropriate unit
 */
QString SharedFilesWidget::formatFileSize(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB)
    {
        return QString::number(bytes / GB) + " GB";
    }
    else if (bytes >= MB)
    {
        return QString::number(bytes / MB) + " MB";
    }
    else if (bytes >= KB)
    {
        return QString::number(bytes / KB) + " KB";
    }
    else
    {
        return QString::number(bytes) + " B";
    }
}
