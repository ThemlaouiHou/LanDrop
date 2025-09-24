#include "aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog{parent}
{
    setWindowTitle("LANDrop - about");

    title = new QLabel("LANDrop");
    title->setStyleSheet("font-weight: bold; font-size: 35px");
    title->setAlignment(Qt::AlignCenter);
    description = new QLabel("LANdrop is a peer-to-peer file sharing application built with C++ and Qt that allows users on the same local network (LAN) to quickly send and receive files without relying on an external service.\n\n\nDevelopers:\n\n- Themlaoui Houssem\n- Jeddi Youssef\n- Charpié Ludovic");
    description->setWordWrap(true);

    mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->addWidget(title);
    mainLayout->addWidget(description);
}
