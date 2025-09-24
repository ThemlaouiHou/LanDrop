/**
 * @file transferitemwidget.h
 * @brief Widget for displaying individual file transfer progress and status
 */

#ifndef TRANSFERITEMWIDGET_H
#define TRANSFERITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QString>
#include "../core/transferstatus.h"

/**
 * @class TransferItemWidget
 * @brief Widget that displays the progress and status of a single file transfer.
 *
 * This widget provides a visual representation of file transfer operations,
 * showing the filename, progress bar, and current status with appropriate
 * styling based on transfer direction (send/receive) and status.
 */
class TransferItemWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @enum TransferDirection
     * @brief Represents the direction of the file transfer.
     */
    enum class TransferDirection
    {
        SEND,   /**< Outgoing file transfer */
        RECEIVE /**< Incoming file transfer */
    };

    explicit TransferItemWidget(const QString &fileName, TransferDirection dir, QWidget *parent = nullptr);

    void updateProgress(int percent);
    void setStatus(const TransferStatus status);
    TransferStatus getStatus() const;
    QString getFileName() const;

private:
    /** Name of the file being transferred */
    QString fileName;

    /** Label displaying the file name */
    QLabel *fileNameLabel;

    /** Label showing current transfer status and direction */
    QLabel *statusLabel;

    /** Progress bar showing transfer completion percentage */
    QProgressBar *progressBar;

    /** Current transfer status */
    TransferStatus status = TransferStatus::WAITING;

    /** Direction of the transfer (send or receive) */
    TransferDirection direction;
};

#endif // TRANSFERITEMWIDGET_H
