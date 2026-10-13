#pragma once
#include <QCoreApplication>
#include <QSqlDatabase>
#include <vector>
#include "Product.h"
#include "ProductHistory.h"

/// Concrete SQLite repository.
class ProductRepository
{
    Q_DECLARE_TR_FUNCTIONS(ProductRepository)
public:
    /// Constructs a repository without a connection.
    ProductRepository() = default;
    /// Constructs a repository on an open connection.
    explicit ProductRepository(QSqlDatabase database) : m_database(database) {}
    /// Returns the database handle.
    QSqlDatabase database() const { return m_database; }
    /// Sets the database handle.
    void setDatabase(QSqlDatabase value) { m_database = value; }
    /// Lists products ordered by name.
    std::vector<Product> all(QString &error) const;
    /// Looks up one product.
    bool find(int id, Product &product, QString &error) const;
    /// Inserts or updates validated product data.
    QString save(Product &product);
    /// Deletes an unsold product; foreign keys preserve sales.
    QString remove(int id);
    /// Loads newest-first audit entries for a product.
    std::vector<ProductHistoryEntry> history(int productId, QString &error) const;
    /// Writes an audit entry inside the caller's transaction.
    QString recordHistory(const Product &before, const Product &after, QString action,
                          QString actorName, int saleId = 0, QString notes = QStringLiteral(""));
private:
    QSqlDatabase m_database;
};
