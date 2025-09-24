/**
 * @file batchrequestdialog.cpp
 */

#include "batchrequestdialog.h"
#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

/**
 * @brief Constructs a batch file request dialog.
 *
 * Creates a dialog displaying all files in the transfer request.
 *
 * @param files Map of file names to their sizes in bytes
 * @param parent Parent widget for the dialog
 */
BatchRequestDialog::BatchRequestDialog(const QMap<QString, qint64> &files, QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Demandes de réception"));
    resize(400, 300);

    QVBoxLayout *mainLay = new QVBoxLayout(this);

    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    listWidget->setFocusPolicy(Qt::NoFocus);

    // Populate the list widget with file names and sizes
    for (auto [name, size] : files.asKeyValueRange()) {
        QListWidgetItem *item = new QListWidgetItem(listWidget);
        QWidget *w = new QWidget;
        QHBoxLayout *h = new QHBoxLayout(w);
        h->setContentsMargins(5,2,5,2);

        QLabel *lbl = new QLabel(name + "\t(" + QString::number(size) + " bytes)", w);
        QCheckBox *chk = new QCheckBox(tr("Accepter"), w);
        chk->setChecked(true);

        h->addWidget(lbl);
        h->addStretch();
        h->addWidget(chk);

        // Store pointer to the checkbox in UserRole
        item->setData(Qt::UserRole,
                      QVariant::fromValue<void*>(static_cast<void*>(chk)));

        w->setLayout(h);
        listWidget->setItemWidget(item, w);
    }

    mainLay->addWidget(listWidget);

    buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this
    );
    mainLay->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &BatchRequestDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &BatchRequestDialog::reject);
}

/**
 * @brief Returns the user's acceptance decisions for each file.
 *
 * Iterates through all list items to extract the checkbox states and file names,
 * building a map that indicates which files the user chose to accept or reject.
 *
 * @return Map where keys are file names and values are true for accepted files, false for rejected
 */
QMap<QString,bool> BatchRequestDialog::results() const
{
    QMap<QString,bool> map;
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem *item = listWidget->item(i);
        QWidget *w = listWidget->itemWidget(item);

        // Retrieve the checkbox stored in UserRole
        auto ptr = item->data(Qt::UserRole).value<void*>();
        QCheckBox *chk = static_cast<QCheckBox*>(ptr);

        // Extract file name from the label
        QLabel *lbl = w->findChild<QLabel*>();
        QString fn = lbl ? lbl->text().split('\t')[0] : QString();

        map[fn] = chk->isChecked();
    }
    return map;
}
