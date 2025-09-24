/**
 * @file configdialog.h
 * @brief Dialog for configuring LANDrop application settings
 */

#ifndef CONFIGURATIONDIALOG_H
#define CONFIGURATIONDIALOG_H

#include <QWidget>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QLabel>

/**
 * @class ConfigDialog
 * @brief Dialog widget for modifying LANDrop application configuration settings.
 *
 * This dialog provides a user interface for configuring application settings
 * including download path, network port, and buffer size. Changes are applied
 * when the user accepts the dialog.
 */
class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);

    QString getDownloadPath() const;
    int getPort() const;
    int getBufferSize() const;

private slots:
    void selectDownloadDirectory();

private:
    /** Label displaying current download/received folder path */
    QLabel *downloadPathEdit;

    /** Text inputs for port and buffer size */
    QLineEdit *portEdit, *bufferEdit;

    /** Button to open directory browser for download path selection */
    QPushButton *downloadBrowseButton;

    /** Ation buttons */
    QPushButton *saveButton, *cancelButton, *resetButton;

    /** Layouts for the interface*/
    QFormLayout *formLayout;
    QVBoxLayout *mainLayout, *buttonLayout;
    QHBoxLayout *altButtonLayout, *downloadPathLayout;
};

#endif // CONFIGURATIONDIALOG_H
