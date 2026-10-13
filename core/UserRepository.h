#pragma once
#include <QCoreApplication>
#include <QSqlDatabase>
#include <vector>
#include "User.h"
#include <memory>

/// Concrete SQLite repository.
class UserRepository
{
    Q_DECLARE_TR_FUNCTIONS(UserRepository)
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
    /// Finds a current stored role by ID.
    std::shared_ptr<User> find(int id, QString &error) const;
    /// Inserts or updates a user.
    QString save(User &user);
    /// Deletes a user, preserving referenced sales.
    QString remove(int id);
private:
    QSqlDatabase m_database;
};
