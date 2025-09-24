/**
 * @file batchrequestdialog.cpp
 * @brief Dialog for handling multiple simultaneous file transfer requests
 */
#include "batchrequestdialog.h"
#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

/**
 * @brief Constructs a batch request dialog
 * @param files Map of filename to file size for all pending transfers
 * @param parent Parent widget for modal dialog behavior
 */
BatchRequestDialog::BatchRequestDialog(const QMap<QString, qint64> &files, QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Reception requests"));
    resize(400, 300);

    QVBoxLayout *mainLay = new QVBoxLayout(this);

    // --- Scrollable File List Setup ---
    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    listWidget->setFocusPolicy(Qt::NoFocus);

    // --- Create List Items for Each File ---
    for (auto [name, size] : files.asKeyValueRange()) {
        QListWidgetItem *item = new QListWidgetItem(listWidget);
        QWidget *w = new QWidget;
        QHBoxLayout *h = new QHBoxLayout(w);
        h->setContentsMargins(5,2,5,2);

        // File info label (name + size)
        QLabel *lbl = new QLabel(name + "\t(" + QString::number(size) + " bytes)", w);
        QCheckBox *chk = new QCheckBox(tr("Accept"), w);
        chk->setChecked(true);

        h->addWidget(lbl);
        h->addStretch();
        h->addWidget(chk);

        // Store checkbox pointer in item data
        item->setData(Qt::UserRole,
                      QVariant::fromValue<void*>(static_cast<void*>(chk)));

        w->setLayout(h);
        listWidget->setItemWidget(item, w);
    }

    mainLay->addWidget(listWidget);

    // --- Dialog Buttons ---
    buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this
    );
    mainLay->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &BatchRequestDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &BatchRequestDialog::reject);
}

/**
 * @brief Retrieves user's accept/refuse decisions for each file
 * @return Map of filename to boolean (true=accept, false=refuse)
 */
QMap<QString,bool> BatchRequestDialog::results() const
{
    QMap<QString,bool> map;
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem *item = listWidget->item(i);
        QWidget *w = listWidget->itemWidget(item);

        // Retrieve checkbox pointer stored in UserRole
        auto ptr = item->data(Qt::UserRole).value<void*>();
        QCheckBox *chk = static_cast<QCheckBox*>(ptr);

        // Extract filename from label
        QLabel *lbl = w->findChild<QLabel*>();
        QString fn = lbl ? lbl->text().split('\t')[0] : QString();

        map[fn] = chk->isChecked();
    }
    return map;
}
