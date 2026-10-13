#pragma once
#include <QString>

enum class Permission { ManageProducts, RegisterSale, ViewReports, ManageUsers };

/// Abstract user role; permissions are checked in the service.
class User
{
public:
    /// Constructs an empty user.
    User() = default;
    /// Constructs a stored user.
    User(int id, QString username, QString passwordHash, QString fullName);
    /// Destroys a derived role through its base pointer.
    virtual ~User() = default;
    /// Returns the stable database role identifier.
    virtual QString roleName() const = 0;
    /// Returns whether the role grants the operation.
    virtual bool hasPermission(Permission permission) const = 0;
    /// Returns id.
    int id() const { return m_id; }
    /// Sets id.
    void setId(int value) { m_id = value; }
    /// Returns username.
    QString username() const { return m_username; }
    /// Sets username.
    void setUsername(QString value) { m_username = value; }
    /// Returns passwordHash.
    QString passwordHash() const { return m_passwordHash; }
    /// Sets passwordHash.
    void setPasswordHash(QString value) { m_passwordHash = value; }
    /// Returns fullName.
    QString fullName() const { return m_fullName; }
    /// Sets fullName.
    void setFullName(QString value) { m_fullName = value; }
private:
    int m_id = 0;
    QString m_username;
    QString m_passwordHash;
    QString m_fullName;
};

/// Admin permission policy.
class Admin : public User
{
public:
    /// Constructs an empty role.
    Admin() = default;
    /// Constructs a stored role.
    Admin(int id, QString username, QString passwordHash, QString fullName);
    /// Returns the database role identifier.
    QString roleName() const override;
    /// Checks the role permission table.
    bool hasPermission(Permission permission) const override;
};

/// WarehouseManager permission policy.
class WarehouseManager : public User
{
public:
    /// Constructs an empty role.
    WarehouseManager() = default;
    /// Constructs a stored role.
    WarehouseManager(int id, QString username, QString passwordHash, QString fullName);
    /// Returns the database role identifier.
    QString roleName() const override;
    /// Checks the role permission table.
    bool hasPermission(Permission permission) const override;
};

/// Cashier permission policy.
class Cashier : public User
{
public:
    /// Constructs an empty role.
    Cashier() = default;
    /// Constructs a stored role.
    Cashier(int id, QString username, QString passwordHash, QString fullName);
    /// Returns the database role identifier.
    QString roleName() const override;
    /// Checks the role permission table.
    bool hasPermission(Permission permission) const override;
};
