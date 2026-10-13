#pragma once
#include <QSqlDatabase>
#include <QString>

/// Owns one named SQLite connection; outlives repositories and services.
class Database
{
public:
    /// Uses the default file path.
    Database();
    /// Uses a file path or :memory: for tests.
    explicit Database(QString filePath);
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    /// Opens the connection, creates the schema, and seeds empty tables.
    QString open();
    /// Returns a connection handle.
    QSqlDatabase connection() const;
    /// Returns the database file path.
    QString filePath() const { return m_filePath; }
    /// Changes the path before open; returns false once open.
    bool setFilePath(QString value);
    /// Returns the unique Qt connection name.
    QString connectionName() const { return m_connectionName; }
    /// Changes the connection name before open; returns false once registered.
    bool setConnectionName(QString value);
private:
    QString m_filePath;
    QString m_connectionName;
};

/// SHA-256 hash used by the assignment's login implementation.
QString hashPassword(const QString &password);
