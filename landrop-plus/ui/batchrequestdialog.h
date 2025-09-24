/**
 * @file batchrequestdialog.h
 * @brief Dialog for approving or rejecting multiple incoming file transfers
 */

#ifndef BATCHREQUESTDIALOG_H
#define BATCHREQUESTDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QMap>

class QListWidget;
class QDialogButtonBox;

/**
 * @class BatchRequestDialog
 * @brief Dialog widget for handling multiple incoming file transfer requests.
 *
 * This dialog presents a list of files that are in for reception, allowing
 * the user to accept or reject individual files.
 */
class BatchRequestDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BatchRequestDialog(const QMap<QString, qint64> &files, QWidget *parent = nullptr);

    QMap<QString, bool> results() const;

private:
    /** List widget displaying files */
    QListWidget *listWidget;

    /** Standard dialog buttons */
    QDialogButtonBox *buttonBox;
};

#endif // BATCHREQUESTDIALOG_H
