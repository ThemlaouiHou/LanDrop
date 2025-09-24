#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QWidget>
#include <QDialog>
#include <qboxlayout.h>
#include <qlabel.h>

class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);

private:
    QLabel *title, *description;
    QVBoxLayout *mainLayout;

signals:
};

#endif // ABOUTDIALOG_H
