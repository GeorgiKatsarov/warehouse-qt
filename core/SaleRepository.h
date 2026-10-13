#pragma once
#include <QCoreApplication>
#include <QSqlDatabase>
#include <vector>
#include "Sale.h"

/// Concrete SQLite repository.
class SaleRepository
{
    Q_DECLARE_TR_FUNCTIONS(SaleRepository)
public:
    /// Constructs a repository without a connection.
    SaleRepository() = default;
    /// Constructs a repository on an open connection.
    explicit SaleRepository(QSqlDatabase database) : m_database(database) {}
    /// Returns the database handle.
    QSqlDatabase database() const { return m_database; }
    /// Sets the database handle.
    void setDatabase(QSqlDatabase value) { m_database = value; }
    /// Saves sale and items; caller owns the SQL transaction.
    QString save(Sale &sale);
    /// Loads a sale and its receipt lines.
    bool find(int id, Sale &sale, QString &error) const;
    /// Computes and saves a financial report.
    QString generate(Report &report);
    /// Lists previously generated reports.
    std::vector<Report> reports(QString &error) const;
private:
    QSqlDatabase m_database;
};
