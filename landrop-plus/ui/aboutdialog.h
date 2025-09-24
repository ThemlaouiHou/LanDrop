/**
 * @file aboutdialog.h
 * @brief Dialog displaying application information and credits
 */

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QWidget>
#include <QDialog>
#include <QBoxLayout>
#include <QLabel>

/**
 * @class AboutDialog
 * @brief Simple dialog widget displaying LANDrop application information.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);

private:
    QLabel *title, *description;
    QVBoxLayout *mainLayout;
};

#endif // ABOUTDIALOG_H
