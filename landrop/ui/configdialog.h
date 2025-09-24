#ifndef CONFIGURATIONDIALOG_H
#define CONFIGURATIONDIALOG_H

#include <QWidget>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QLabel>

class ConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);

    QString getDownloadPath() const;
    int getPort() const;
    int getBufferSize() const;
    
private slots:
    void selectDirectory();

private:
    QLabel *downloadPathEdit;
    QLineEdit *portEdit, *bufferEdit;
    QPushButton *browseButton;
    QPushButton *saveButton, *cancelButton, *resetButton;

    QFormLayout *formLayout;
    QVBoxLayout *mainLayout, *buttonLayout;
    QHBoxLayout *altButtonLayout, *pathLayout;
};

#endif // CONFIGURATIONDIALOG_H
