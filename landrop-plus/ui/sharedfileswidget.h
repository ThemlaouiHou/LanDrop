/**
 * @file sharedfileswidget.h
 * @brief Widget for displaying and managing shared files from discovered users
 */

#ifndef SHAREDFILESWIDGET_H
#define SHAREDFILESWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QJsonObject>
#include "../services/broadcastdiscoveryservice.h"

class SharedFileManager;

/**
 * @class SharedFilesWidget
 * @brief Widget that displays shared files from discovered LANDrop users in a hierarchical tree view.
 *
 * This widget provides a user interface for browsing and downloading files shared by other
 * LANDrop users on the network. It displays users and their shared files in a tree structure.
 */
class SharedFilesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SharedFilesWidget(QWidget *parent = nullptr);

    void setSharedFileManager(SharedFileManager *manager);

public slots:
    void onUserListUpdated(const QList<LANDropUser> &users);

signals:
    /** Emitted when user requests to download a shared file */
    void downloadRequested(const QString &userIP, quint16 userPort, const QString &relativePath, const QString &fileName);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onDownloadButtonClicked();
    void onOpenSharedFolderClicked();
    void onRefreshClicked();

private:
    void setupUI();
    void populateUserFiles();
    void addUserToTree(const LANDropUser &user);
    void addFileToUser(QTreeWidgetItem *userItem, const QJsonObject &fileInfo, const QString &userIP, quint16 userPort);
    QTreeWidgetItem* createFileItem(const QJsonObject &fileInfo, const QString &userIP, quint16 userPort);
    QString formatFileSize(qint64 bytes) const;

    /** Tree widget displaying users and their shared files */
    QTreeWidget *treeWidget;
    
    /** Button to download selected file */
    QPushButton *downloadButton;
    
    /** Button to open local shared folder */
    QPushButton *openFolderButton;
    
    /** Button to refresh the shared files list */
    QPushButton *refreshButton;
    
    /** Label showing status and shared folders information */
    QLabel *statusLabel;
    
    /** List of currently discovered users with their shared files */
    QList<LANDropUser> discoveredUsers;
    
    /** Manager for local shared file operations */
    SharedFileManager *sharedFileManager;
    
    /** Custom data roles for storing metadata in tree items */
    static const int UserIPRole = Qt::UserRole + 1;
    static const int UserPortRole = Qt::UserRole + 2;
    static const int FilePathRole = Qt::UserRole + 3;
    static const int FileTypeRole = Qt::UserRole + 4;
    static const int IsDownloadableRole = Qt::UserRole + 5;
};

#endif // SHAREDFILESWIDGET_H
