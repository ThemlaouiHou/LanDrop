#ifndef BATCHREQUESTDIALOG_H
#define BATCHREQUESTDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QMap>

class QListWidget;
class QDialogButtonBox;

class BatchRequestDialog : public QDialog {
    Q_OBJECT
public:
    //filenames : the list of files the reciever must validate
    explicit BatchRequestDialog(const QMap<QString, qint64> &files, QWidget *parent = nullptr);

    // After exec(), returns map[fileName] = true if accepted, otherwise false
    QMap<QString,bool> results() const;

private:
    QListWidget      *listWidget;
    QDialogButtonBox *buttonBox;
};

#endif // BATCHREQUESTDIALOG_H
