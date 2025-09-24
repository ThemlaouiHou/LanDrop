/**
 * @file userlistwidget.h
 * @brief Widget for displaying discovered LANDrop users on the network
 */

#ifndef USERLISTWIDGET_H
#define USERLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include "../services/broadcastdiscoveryservice.h"

/**
 * @class UserListWidget
 * @brief Widget that displays a list of discovered LANDrop users on the local network.
 *
 * This widget integrates with the BroadcastDiscoveryService to show available users
 * and allows selection for file transfers. It provides automatic refresh functionality
 * and manual refresh controls, displaying user information including hostnames,
 * IP addresses.
 */
class UserListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UserListWidget(BroadcastDiscoveryService *discovery, QWidget *parent = nullptr);
    ~UserListWidget();

private slots:
    void onUserListUpdated(const QList<LANDropUser> &users);
    void onItemClicked(QListWidgetItem *item);

signals:
    /** Emitted when a user is selected */
    void userSelected(const LANDropUser &user);

private:
    void setupUI();
    void setState(bool discovering);
    void updateStatusLabel(int userCount);
    void triggerUIUpdate();

    /** List widget displaying discovered users */
    QListWidget *listWidget;

    /** Button to manually refresh the user list */
    QPushButton *refreshButton;

    /** Label showing discovery status */
    QLabel *statusLabel;

    /** Service for network user discovery */
    BroadcastDiscoveryService *discoveryService;
};

#endif // USERLISTWIDGET_H
