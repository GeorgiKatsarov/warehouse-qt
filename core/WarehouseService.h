#pragma once
#include <QCoreApplication>
#include "ProductRepository.h"
#include "UserRepository.h"
#include "SaleRepository.h"
#include "Discount.h"
#include <memory>

/// Business operations and permission checks; no Widgets dependencies.
class WarehouseService
{
    Q_DECLARE_TR_FUNCTIONS(WarehouseService)
public:
    /// Constructs an unconnected service.
    WarehouseService();
    /// Constructs the service on an open database.
    explicit WarehouseService(QSqlDatabase database);
    /// Returns the database handle.
    QSqlDatabase database() const { return m_database; }
    /// Sets the connection and logs out the current user.
    void setDatabase(QSqlDatabase value);
    /// Authenticates a user and clears any previous session on failure.
    QString login(QString username, QString password);
    /// Clears the session.
    void logout() { m_currentUser.reset(); }
    /// Returns the authenticated user.
    std::shared_ptr<User> currentUser() const { return m_currentUser; }
    /// Reauthenticates the session using credentials (no role injection).
    QString setCurrentUser(QString username, QString password) { return login(username, password); }
    /// Checks the current user's permission.
    bool hasPermission(Permission permission) const;
    /// Returns the active strategy.
    const DiscountStrategy *discountStrategy() const { return m_discount.get(); }
    /// Replaces the strategy; null selects NoDiscount.
    void setDiscountStrategy(std::unique_ptr<DiscountStrategy> value);
    /// Lists products for any authenticated role.
    std::vector<Product> products(QString &error) const;
    /// Saves a product with ManageProducts permission.
    QString saveProduct(Product &product);
    /// Deletes a product with ManageProducts permission.
    QString deleteProduct(int id);
    /// Calculates current prices and stock availability without changing stock.
    QString previewSale(Sale &sale) const;
    /// Validates, decreases stock, and persists a sale atomically.
    QString registerSale(Sale &sale);
    /// Loads an authorized sale as escaped HTML.
    QString receipt(int saleId, bool invoice, QString &error) const;
    /// Computes and saves a report with ViewReports permission.
    QString generateReport(Report &report);
    /// Lists saved reports with ViewReports permission.
    std::vector<Report> reports(QString &error) const;
    /// Lists users with ManageUsers permission.
    std::vector<std::shared_ptr<User>> users(QString &error) const;
    /// Saves a user with ManageUsers permission.
    QString saveUser(User &user);
    /// Deletes a user with ManageUsers permission.
    QString deleteUser(int id);
private:
    QSqlDatabase m_database;
    std::shared_ptr<User> m_currentUser;
    std::unique_ptr<DiscountStrategy> m_discount;
    QString permissionError(Permission permission) const;
    QString calculate(Sale &sale) const;
};
