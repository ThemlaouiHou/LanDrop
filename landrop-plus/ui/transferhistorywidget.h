/**
 * @file transferhistorywidget.h
 * @brief Widget for displaying and managing file transfer history
 */

#ifndef TRANSFERHISTORYWIDGET_H
#define TRANSFERHISTORYWIDGET_H

#include <QWidget>
#include <QMap>
#include "../core/transferstatus.h"

class QScrollArea;
class QVBoxLayout;
class QPushButton;
class TransferItemWidget;

/**
 * @class TransferHistoryWidget
 * @brief Widget that displays a scrollable list of file transfer operations.
 *
 * This widget provides a view of all file transfer sessions.
 * Provides access to received files through folder navigation.
 */
class TransferHistoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TransferHistoryWidget(QWidget *parent = nullptr);

    void addTransferItem(int sessionId, TransferItemWidget *item);
    void updateProgress(int id, int percent);
    void setStatus(int id, TransferStatus status);

private:
    /** Map of transfer session IDs to their corresponding widget items */
    QMap<int, TransferItemWidget *> items;

    /** Scroll area for displaying the items */
    QScrollArea *scrollArea;

    /** Container widget for the scrollable content */
    QWidget *container;

    /** Layout for arranging transfer item widgets */
    QVBoxLayout *itemsLayout;

    /** Button for opening the received files folder */
    QPushButton *openFolderButton;
};

#endif // TRANSFERHISTORYWIDGET_H
