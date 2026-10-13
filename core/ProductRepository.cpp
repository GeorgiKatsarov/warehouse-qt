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
        ? "INSERT INTO products(id,name,sku,quantity,supplier,delivery_price,sale_price) VALUES((SELECT COALESCE(MAX(value),0)+1 FROM (SELECT id AS value FROM products UNION ALL SELECT product_id AS value FROM product_history)),?,?,?,?,?,?)"
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

std::vector<ProductHistoryEntry> ProductRepository::history(int productId, QString &error) const
{
    error.clear();
    std::vector<ProductHistoryEntry> result;
    QSqlQuery query(m_database);
    query.prepare("SELECT id,product_id,created_at,actor_name,action,quantity_before,quantity_after,name_before,name_after,delivery_before,delivery_after,price_before,price_after,sale_id,notes FROM product_history WHERE product_id=? ORDER BY created_at DESC,id DESC");
    query.addBindValue(productId);
    if (!query.exec()) { error = query.lastError().text(); return result; }
    while (query.next()) {
        ProductHistoryEntry entry;
        entry.setId(query.value(0).toInt()); entry.setProductId(query.value(1).toInt());
        entry.setCreatedAt(QDateTime::fromString(query.value(2).toString(),Qt::ISODate));
        entry.setActorName(query.value(3).toString()); entry.setAction(query.value(4).toString());
        entry.setQuantityBefore(query.value(5).toInt()); entry.setQuantityAfter(query.value(6).toInt());
        entry.setNameBefore(query.value(7).toString()); entry.setNameAfter(query.value(8).toString());
        entry.setDeliveryBefore(query.value(9).toLongLong()); entry.setDeliveryAfter(query.value(10).toLongLong());
        entry.setPriceBefore(query.value(11).toLongLong()); entry.setPriceAfter(query.value(12).toLongLong());
        entry.setSaleId(query.value(13).toInt()); entry.setNotes(query.value(14).toString()); result.push_back(entry);
    }
    return result;
}
QString ProductRepository::recordHistory(const Product &before, const Product &after, QString action,
                                         QString actorName, int saleId, QString notes)
{
    if (notes.isEmpty() && before.id() && after.id()) {
        QStringList changes;
        if (before.name()!=after.name()) changes.append(tr("Име: %1 → %2").arg(before.name(),after.name()));
        if (before.sku()!=after.sku()) changes.append(tr("Код: %1 → %2").arg(before.sku(),after.sku()));
        if (before.supplier()!=after.supplier()) changes.append(tr("Доставчик: %1 → %2").arg(before.supplier(),after.supplier()));
        notes=changes.join("; ");
    }
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO product_history(product_id,created_at,actor_name,action,quantity_before,quantity_after,name_before,name_after,delivery_before,delivery_after,price_before,price_after,sale_id,notes) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
    query.addBindValue(after.id() ? after.id() : before.id());
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(actorName); query.addBindValue(action);
    query.addBindValue(before.quantity()); query.addBindValue(after.quantity());
    query.addBindValue(before.name().isNull() ? QStringLiteral("") : before.name());
    query.addBindValue(after.name().isNull() ? QStringLiteral("") : after.name());
    query.addBindValue(before.deliveryPrice()); query.addBindValue(after.deliveryPrice());
    query.addBindValue(before.salePrice()); query.addBindValue(after.salePrice());
    query.addBindValue(saleId); query.addBindValue(notes.isNull() ? QStringLiteral("") : notes);
    return query.exec() ? QString() : query.lastError().text();
}
