#include "SaleRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>

QString SaleRepository::save(Sale &sale)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO sales(created_at,user_id,customer_name,is_vip,discount_name,total) VALUES(?,?,?,?,?,?)");
    query.addBindValue(sale.createdAt().toString(Qt::ISODate)); query.addBindValue(sale.userId());
    query.addBindValue(sale.customerName()); query.addBindValue(sale.isVip());
    query.addBindValue(sale.discountName()); query.addBindValue(sale.total());
    if (!query.exec()) return query.lastError().text();
    sale.setId(query.lastInsertId().toInt());
    for (const SaleItem &item : sale.items()) {
        query.prepare("INSERT INTO sale_items(sale_id,product_id,quantity,unit_price,line_total,delivery_price,product_name,product_sku) VALUES(?,?,?,?,?,?,?,?)");
        query.addBindValue(sale.id()); query.addBindValue(item.productId()); query.addBindValue(item.quantity());
        query.addBindValue(item.unitPrice()); query.addBindValue(item.lineTotal());
        query.addBindValue(item.deliveryPrice()); query.addBindValue(item.productName()); query.addBindValue(item.productSku());
        if (!query.exec()) return query.lastError().text();
    }
    return {};
}
bool SaleRepository::find(int id, Sale &sale, QString &error) const
{
    error.clear();
    QSqlQuery query(m_database);
    query.prepare("SELECT id,created_at,user_id,customer_name,is_vip,discount_name,total FROM sales WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) { error = query.lastError().text(); return false; }
    if (!query.next()) { error = tr("Продажбата не е намерена."); return false; }
    sale = Sale(query.value(3).toString(),query.value(4).toBool());
    sale.setId(query.value(0).toInt()); sale.setCreatedAt(QDateTime::fromString(query.value(1).toString(),Qt::ISODate));
    sale.setUserId(query.value(2).toInt()); sale.setDiscountName(query.value(5).toString());
    sale.setTotal(query.value(6).toLongLong());
    query.prepare("SELECT product_id,quantity,unit_price,line_total,product_name,product_sku,delivery_price FROM sale_items WHERE sale_id=? ORDER BY id");
    query.addBindValue(id);
    if (!query.exec()) { error = query.lastError().text(); return false; }
    std::vector<SaleItem> items;
    while (query.next()) {
        SaleItem item(query.value(0).toInt(),query.value(1).toInt());
        item.setUnitPrice(query.value(2).toLongLong()); item.setLineTotal(query.value(3).toLongLong());
        item.setProductName(query.value(4).toString()); item.setProductSku(query.value(5).toString());
        item.setDeliveryPrice(query.value(6).toLongLong()); items.push_back(item);
    }
    sale.setItems(items);
    return true;
}
QString SaleRepository::generate(Report &report)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT COALESCE(SUM(i.line_total),0),COALESCE(SUM(i.line_total-i.quantity*i.delivery_price),0),COALESCE(SUM(i.quantity),0) FROM sale_items i JOIN sales s ON s.id=i.sale_id WHERE date(s.created_at) BETWEEN ? AND ?");
    query.addBindValue(report.dateFrom().toString(Qt::ISODate)); query.addBindValue(report.dateTo().toString(Qt::ISODate));
    if (!query.exec() || !query.next()) return query.lastError().text();
    report.setRevenue(query.value(0).toLongLong()); report.setProfit(query.value(1).toLongLong());
    report.setUnitsSold(query.value(2).toLongLong()); report.setCreatedAt(QDateTime::currentDateTime());
    query.prepare("INSERT INTO reports(created_at,date_from,date_to,revenue,profit,units_sold) VALUES(?,?,?,?,?,?)");
    query.addBindValue(report.createdAt().toString(Qt::ISODate)); query.addBindValue(report.dateFrom().toString(Qt::ISODate));
    query.addBindValue(report.dateTo().toString(Qt::ISODate)); query.addBindValue(report.revenue());
    query.addBindValue(report.profit()); query.addBindValue(report.unitsSold());
    if (!query.exec()) return query.lastError().text();
    report.setId(query.lastInsertId().toInt());
    return {};
}
std::vector<Report> SaleRepository::reports(QString &error) const
{
    error.clear();
    std::vector<Report> result;
    QSqlQuery query(m_database);
    if (!query.exec("SELECT id,created_at,date_from,date_to,revenue,profit,units_sold FROM reports ORDER BY id DESC")) {
        error = query.lastError().text(); return result;
    }
    while (query.next()) {
        Report report(QDate::fromString(query.value(2).toString(),Qt::ISODate),QDate::fromString(query.value(3).toString(),Qt::ISODate));
        report.setId(query.value(0).toInt()); report.setCreatedAt(QDateTime::fromString(query.value(1).toString(),Qt::ISODate));
        report.setRevenue(query.value(4).toLongLong()); report.setProfit(query.value(5).toLongLong());
        report.setUnitsSold(query.value(6).toLongLong()); result.push_back(report);
    }
    return result;
}
