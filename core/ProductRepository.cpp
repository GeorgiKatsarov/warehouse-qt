#include "ProductRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>

namespace {
Product readProduct(const QSqlQuery &query)
{
    return Product(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString(),
                   query.value(3).toInt(), query.value(4).toString(), query.value(5).toLongLong(),
                   query.value(6).toLongLong());
}
}
std::vector<Product> ProductRepository::all(QString &error) const
{
    error.clear();
    std::vector<Product> products;
    QSqlQuery query(m_database);
    if (!query.exec("SELECT id,name,sku,quantity,supplier,delivery_price,sale_price FROM products ORDER BY name")) {
        error = query.lastError().text();
        return products;
    }
    while (query.next())
        products.push_back(readProduct(query));
    return products;
}
bool ProductRepository::find(int id, Product &product, QString &error) const
{
    error.clear();
    QSqlQuery query(m_database);
    query.prepare("SELECT id,name,sku,quantity,supplier,delivery_price,sale_price FROM products WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) { error = query.lastError().text(); return false; }
    if (!query.next()) {
        error = tr("Продуктът не е намерен.");
        return false;
    }
    product = readProduct(query);
    return true;
}
QString ProductRepository::save(Product &product)
{
    const QString error = product.validate();
    if (!error.isEmpty())
        return error;
    QSqlQuery query(m_database);
    query.prepare(product.id() == 0
        ? "INSERT INTO products(name,sku,quantity,supplier,delivery_price,sale_price) VALUES(?,?,?,?,?,?)"
        : "UPDATE products SET name=?,sku=?,quantity=?,supplier=?,delivery_price=?,sale_price=? WHERE id=?");
    query.addBindValue(product.name().trimmed()); query.addBindValue(product.sku().trimmed());
    query.addBindValue(product.quantity());
    query.addBindValue(product.supplier().isNull() ? QStringLiteral("") : product.supplier());
    query.addBindValue(product.deliveryPrice()); query.addBindValue(product.salePrice());
    if (product.id() != 0) query.addBindValue(product.id());
    if (!query.exec()) return query.lastError().text();
    if (product.id() == 0) product.setId(query.lastInsertId().toInt());
    else if (query.numRowsAffected() != 1)
        return tr("Продуктът не е намерен.");
    return {};
}
QString ProductRepository::remove(int id)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM products WHERE id=?"); query.addBindValue(id);
    if (!query.exec()) return query.lastError().text();
    if (query.numRowsAffected() != 1)
        return tr("Продуктът не е намерен.");
    return {};
}
