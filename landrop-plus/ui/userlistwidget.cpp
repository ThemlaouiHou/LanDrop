/**
 * @file userlistwidget.cpp
 */

#include "userlistwidget.h"
#include "../config/config.h"
#include "../services/sharedfilemanager.h"
#include <QVBoxLayout>
#include <QListWidgetItem>

/**
 * @brief Constructs the UserListWidget and initializes the user discovery service.
 *
 * Connects to the BroadcastDiscoveryService to receive updates on discovered users
 * and sets up the UI components for displaying the user list.
 *
 * @param discovery Pointer to the BroadcastDiscoveryService instance
 * @param parent Parent widget for this widget
 */
UserListWidget::UserListWidget(BroadcastDiscoveryService *discovery, QWidget *parent)
    : QWidget(parent), discoveryService(discovery)
{
    setupUI();

    // Connect to user list updates
    connect(discoveryService, &BroadcastDiscoveryService::userListUpdated,
            this, &UserListWidget::onUserListUpdated);

    connect(listWidget, &QListWidget::itemClicked, this, &UserListWidget::onItemClicked);
    connect(refreshButton, &QPushButton::clicked, this, &UserListWidget::triggerUIUpdate);

    // Initial update
    triggerUIUpdate();
}

/**
 * @brief Destructor for UserListWidget.
 *
 * Stops the discovery service.
 */
UserListWidget::~UserListWidget()
{
    if (discoveryService)
    {
        discoveryService->stopDiscovery();
    }
}

/**
 * @brief Sets up the user interface layout and styling.
 */
void UserListWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *title = new QLabel("Available Users", this);
    title->setStyleSheet("font-weight: bold; font-size: 16px;");

    listWidget = new QListWidget(this);
    refreshButton = new QPushButton("Refresh", this);
    refreshButton->setStyleSheet(Config::getButtonStyleSheet());

    statusLabel = new QLabel("Discovering LANDrop users...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: orange; font-style: italic;");

    layout->addWidget(title);
    layout->addWidget(statusLabel);
    layout->addWidget(listWidget);
    layout->addWidget(refreshButton);
}

/**
 * @brief Updates the user list display with the latest discovered users.
 *
 * Completely rebuilds the list widget with the provided users. Each list item
 * displays the hostname and IP address, while storing the complete LANDropUser
 * object in the item's user data for later access.
 *
 * @param users Complete list of currently discovered LANDrop users
 */
void UserListWidget::onUserListUpdated(const QList<LANDropUser> &users)
{
    // qDebug() << "UserListWidget: list contains" << users.size() << "users";

    listWidget->clear();

    for (const LANDropUser &user : users)
    {
        QString displayText = QString("%1 [%2]").arg(user.hostname, user.ipAddress);
        QListWidgetItem *item = new QListWidgetItem(displayText);

        QVariant userData;
        userData.setValue(user);
        item->setData(Qt::UserRole, userData);

        listWidget->addItem(item);
    }

    updateStatusLabel(users.size());
    setState(false);
}

/**
 * @brief Handles item click events in the user list.
 *
 * @param item The QListWidgetItem that was clicked
 */
void UserListWidget::onItemClicked(QListWidgetItem *item)
{
    // Get complete user data
    QVariant userData = item->data(Qt::UserRole);
    LANDropUser user = userData.value<LANDropUser>();

    // Emit signal with user information
    emit userSelected(user);
}

/**
 * @brief Triggers a manual UI update to refresh the user list.
 *
 * Sets the state to discovering, updates the status label, and requests
 * an update from the discovery service.
 */
void UserListWidget::triggerUIUpdate()
{
    setState(true);
    statusLabel->setText("Refreshing user list...");

    discoveryService->requestUserListUpdate();
}

/**
 * @brief Updates the status label based on the number of discovered users.
 *
 * If no users are found, sets a info message that no users were discovered.
 *
 * @param userCount Number of discovered LANDrop users
 */
void UserListWidget::updateStatusLabel(int userCount)
{
    if (userCount == 0)
    {
        statusLabel->setText("No LANDrop users found");
    }
    else
    {
        statusLabel->setText("");
    }
}

/**
 * @brief Sets the state of the refresh button and status label based on discovery state.
 *
 * @param discovering True if discovery is in progress, false otherwise
 */
void UserListWidget::setState(bool discovering)
{
    if (discovering)
    {
        refreshButton->setEnabled(false);
        refreshButton->setStyleSheet(Config::getDisabledButtonStyleSheet());
    }
    else
    {
        refreshButton->setEnabled(true);
        refreshButton->setStyleSheet(Config::getButtonStyleSheet());
    }
}