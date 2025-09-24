/**
 * @file sendfilewidget.h
 * @brief User interface for selecting files and recipients for transfer
 */

#ifndef SENDFILEWIDGET_H
#define SENDFILEWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QFileInfo>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>
#include "../services/filetransfermanager.h"
#include "../services/broadcastdiscoveryservice.h"
#include "transferhistorywidget.h"

class TransferHistoryWidget;
class TransferItemWidget;

/**
 * @class SendFileWidget
 * @brief Widget providing file selection and management for sending transfers.
 * 
 * This widget allows users to select files for transfer and specify recipients either
 * by IP address or by selecting discovered LANDrop users. It provides an interface
 * for managing multiple file selections and recipients.
 */
class SendFileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SendFileWidget(TransferHistoryWidget *historyWidget, FileTransferManager *transferManager, QWidget *parent = nullptr);

public slots:
    void setRecipientUser(const LANDropUser &user);
    void setTransferManager(FileTransferManager *manager);

private slots:
    void onSendButtonClicked();
    void onSelectFilesClicked();

private:
    void setupUI();
    QStringList validateRecipients(const QString &input) const;
    QStringList getSelectedFilePaths() const;
    void clearFileList();

    /** List widget showing selected files for transfer */
    QListWidget *fileListWidget;
    
    /** Text input for recipient IP addresses or hostnames */
    QLineEdit *recipientInput;
    
    /** Scroll area containing the file list */
    QScrollArea *scrollArea;
    
    /** Reference to transfer history widget for status updates */
    TransferHistoryWidget *historyWidget;
    
    /** Manager for coordinating file transfers */
    FileTransferManager *transferManager;
    
    /** Map storing selected user information for port handling */
    QMap<QString, LANDropUser> selectedUsers;
};

#endif // SENDFILEWIDGET_H
