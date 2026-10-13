#include "User.h"

User::User(int id, QString username, QString passwordHash, QString fullName)
    : m_id(id), m_username(username), m_passwordHash(passwordHash), m_fullName(fullName)
{
}

Admin::Admin(int id, QString username, QString passwordHash, QString fullName)
    : User(id, username, passwordHash, fullName)
{
}

QString Admin::roleName() const { return QStringLiteral("admin"); }

bool Admin::hasPermission(Permission permission) const
{
    switch (permission) {
    case Permission::ManageProducts:
    case Permission::RegisterSale:
    case Permission::ViewReports:
    case Permission::ManageUsers:
        return true;
    default:
        return false;
    }
}

WarehouseManager::WarehouseManager(int id, QString username, QString passwordHash, QString fullName)
    : User(id, username, passwordHash, fullName)
{
}

QString WarehouseManager::roleName() const { return QStringLiteral("manager"); }

bool WarehouseManager::hasPermission(Permission permission) const
{
    switch (permission) {
    case Permission::ManageProducts:
    case Permission::ViewReports:
        return true;
    default:
        return false;
    }
}

Cashier::Cashier(int id, QString username, QString passwordHash, QString fullName)
    : User(id, username, passwordHash, fullName)
{
}

QString Cashier::roleName() const { return QStringLiteral("cashier"); }

bool Cashier::hasPermission(Permission permission) const
{
    switch (permission) {
    case Permission::RegisterSale:
        return true;
    default:
        return false;
    }
}
