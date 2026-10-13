#pragma once
#include <QSqlDatabase>
#include <vector>
#include "User.h"
#include <memory>

/// Concrete SQLite repository.
class UserRepository
{
public:
    /// Constructs a repository without a connection.
    UserRepository() = default;
    /// Constructs a repository on an open connection.
    explicit UserRepository(QSqlDatabase database) : m_database(database) {}
    /// Returns the database handle.
    QSqlDatabase database() const { return m_database; }
    /// Sets the database handle.
    void setDatabase(QSqlDatabase value) { m_database = value; }
    /// Lists polymorphic roles using shared pointers.
    std::vector<std::shared_ptr<User>> all(QString &error) const;
    /// Authenticates a username and password.
    std::shared_ptr<User> login(QString username, QString password, QString &error) const;
    /// Inserts or updates a user.
    QString save(User &user);
    /// Deletes a user, preserving referenced sales.
    QString remove(int id);
private:
    QSqlDatabase m_database;
};
