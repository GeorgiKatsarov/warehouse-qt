#include "SaleRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <limits>

QString SaleRepository::save(Sale &sale)
{
    QSqlQuery query(m_database);
    query.prepare(sale.id()==0
        ? "INSERT INTO sales(created_at,user_id,customer_name,is_vip,discount_name,total) VALUES(?,?,?,?,?,?)"
        : "UPDATE sales SET created_at=?,user_id=?,customer_name=?,is_vip=?,discount_name=?,total=? WHERE id=? AND status='active'");
    query.addBindValue(sale.createdAt().toString(Qt::ISODate)); query.addBindValue(sale.userId());
    query.addBindValue(sale.customerName()); query.addBindValue(sale.isVip());
    query.addBindValue(sale.discountName()); query.addBindValue(sale.total());
    if (sale.id()!=0) query.addBindValue(sale.id());
    if (!query.exec()) return query.lastError().text();
    if (sale.id()==0) sale.setId(query.lastInsertId().toInt());
    else {
        if (query.numRowsAffected()!=1) return tr("Продажбата не е намерена или е отменена.");
        query.prepare("DELETE FROM sale_items WHERE sale_id=?"); query.addBindValue(sale.id());
        if (!query.exec()) return query.lastError().text();
    }
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
    query.prepare("SELECT id,created_at,user_id,customer_name,is_vip,discount_name,total,status FROM sales WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) { error = query.lastError().text(); return false; }
    if (!query.next()) { error = tr("Продажбата не е намерена."); return false; }
    sale = Sale(query.value(3).toString(),query.value(4).toBool());
    sale.setId(query.value(0).toInt()); sale.setCreatedAt(QDateTime::fromString(query.value(1).toString(),Qt::ISODate));
    sale.setUserId(query.value(2).toInt()); sale.setDiscountName(query.value(5).toString());
    sale.setTotal(query.value(6).toLongLong()); sale.setCancelled(query.value(7).toString()=="cancelled");
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

QString reportTypeName(ReportType type)
{
    switch (type) {
    case ReportType::FinancialSummary: return QCoreApplication::translate("Report","Финансово обобщение");
    case ReportType::SalesByProduct: return QCoreApplication::translate("Report","Продажби по продукти");
    case ReportType::DailySales: return QCoreApplication::translate("Report","Дневни продажби");
    case ReportType::InventoryValuation: return QCoreApplication::translate("Report","Оценка на наличностите");
    case ReportType::LowStock: return QCoreApplication::translate("Report","Ниски наличности (до 10 бр.)");
    }
    return {};
}
namespace {
QString payload(const Report &report)
{
    QJsonArray headers,rows,money,numbers;
    for (const QString &header : report.headers()) headers.append(header);
    for (const QStringList &row : report.rows()) {
        QJsonArray values; for (const QString &value : row) values.append(value); rows.append(values);
    }
    for (int column : report.moneyColumns()) money.append(column);
    for (int column : report.numberColumns()) numbers.append(column);
    return QString::fromUtf8(QJsonDocument(QJsonObject{{"headers",headers},{"rows",rows},{"money",money},{"numbers",numbers}}).toJson(QJsonDocument::Compact));
}
void summaryRows(Report &report, qint64 saleCount)
{
    report.setHeaders({QCoreApplication::translate("Report","От"),QCoreApplication::translate("Report","До"),
        QCoreApplication::translate("Report","Приходи (€)"),QCoreApplication::translate("Report","Разходи (€)"),
        QCoreApplication::translate("Report","Печалба (€)"),QCoreApplication::translate("Report","Бройки"),
        QCoreApplication::translate("Report","Продажби")});
    report.setMoneyColumns({2,3,4}); report.setNumberColumns({5,6});
    report.setRows({{report.dateFrom().toString(Qt::ISODate),report.dateTo().toString(Qt::ISODate),
        QString::number(report.revenue()),QString::number(report.revenue()-report.profit()),
        QString::number(report.profit()),QString::number(report.unitsSold()),
        saleCount<0 ? QCoreApplication::translate("Report","Няма данни") : QString::number(saleCount)}});
}
bool readReport(const QSqlQuery &query, Report &report, QString &error)
{
    report = Report(QDate::fromString(query.value(2).toString(),Qt::ISODate),QDate::fromString(query.value(3).toString(),Qt::ISODate));
    report.setId(query.value(0).toInt()); report.setCreatedAt(QDateTime::fromString(query.value(1).toString(),Qt::ISODate));
    report.setRevenue(query.value(4).toLongLong()); report.setProfit(query.value(5).toLongLong()); report.setUnitsSold(query.value(6).toLongLong());
    report.setType(static_cast<ReportType>(query.value(7).toInt()));
    report.setTitle(query.value(8).toString().isEmpty() ? reportTypeName(report.type()) : query.value(8).toString());
    const QString stored = query.value(9).toString();
    if (stored.isEmpty()) { summaryRows(report,-1); return true; }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(stored.toUtf8(),&parseError);
    if (parseError.error!=QJsonParseError::NoError || !document.isObject()) {
        error = QCoreApplication::translate("Report","Записаният отчет е повреден."); return false;
    }
    const auto object = document.object();
    QStringList headers; std::vector<QStringList> rows; std::vector<int> money,numbers;
    for (const auto &value : object.value("headers").toArray()) headers.append(value.toString());
    for (const auto &value : object.value("rows").toArray()) {
        QStringList row; for (const auto &cell : value.toArray()) row.append(cell.toString());
        if (row.size()!=headers.size()) { error = QCoreApplication::translate("Report","Невалидни редове в отчета."); return false; }
        rows.push_back(row);
    }
    if (headers.isEmpty()) { error = QCoreApplication::translate("Report","Липсват колони в отчета."); return false; }
    for (const auto &value : object.value("money").toArray()) {
        if (value.toInt()<0 || value.toInt()>=headers.size()) { error = QCoreApplication::translate("Report","Невалидна колона в отчета."); return false; }
        money.push_back(value.toInt());
    }
    for (const auto &value : object.value("numbers").toArray()) {
        if (value.toInt()<0 || value.toInt()>=headers.size()) { error = QCoreApplication::translate("Report","Невалидна колона в отчета."); return false; }
        numbers.push_back(value.toInt());
    }
    report.setHeaders(headers); report.setRows(rows); report.setMoneyColumns(money); report.setNumberColumns(numbers);
    return true;
}
}
std::vector<Sale> SaleRepository::all(QString &error, int userId) const
{
    error.clear(); std::vector<Sale> result; QSqlQuery query(m_database);
    query.prepare("SELECT id,created_at,user_id,customer_name,is_vip,discount_name,total,status FROM sales WHERE (?=0 OR user_id=?) ORDER BY id DESC");
    query.addBindValue(userId); query.addBindValue(userId);
    if (!query.exec()) { error = query.lastError().text(); return result; }
    while (query.next()) {
        Sale sale(query.value(3).toString(),query.value(4).toBool());
        sale.setId(query.value(0).toInt()); sale.setCreatedAt(QDateTime::fromString(query.value(1).toString(),Qt::ISODate));
        sale.setUserId(query.value(2).toInt()); sale.setDiscountName(query.value(5).toString());
        sale.setTotal(query.value(6).toLongLong()); sale.setCancelled(query.value(7).toString()=="cancelled"); result.push_back(sale);
    }
    return result;
}
QString SaleRepository::generate(Report &report)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT COALESCE(SUM(i.line_total),0),COALESCE(SUM(i.line_total-i.quantity*i.delivery_price),0),COALESCE(SUM(i.quantity),0),COALESCE(SUM(i.quantity*i.delivery_price),0),COUNT(DISTINCT s.id) FROM sale_items i JOIN sales s ON s.id=i.sale_id WHERE s.status='active' AND date(s.created_at) BETWEEN ? AND ?");
    query.addBindValue(report.dateFrom().toString(Qt::ISODate)); query.addBindValue(report.dateTo().toString(Qt::ISODate));
    if (!query.exec() || !query.next()) return query.lastError().text();
    report.setRevenue(query.value(0).toLongLong()); report.setProfit(query.value(1).toLongLong());
    report.setUnitsSold(query.value(2).toLongLong()); report.setCreatedAt(QDateTime::currentDateTime());
    const qint64 count = query.value(4).toLongLong();
    if (report.title().trimmed().isEmpty()) report.setTitle(reportTypeName(report.type()));
    std::vector<QStringList> rows;
    if (report.type()==ReportType::FinancialSummary) summaryRows(report,count);
    else if (report.type()==ReportType::SalesByProduct || report.type()==ReportType::DailySales) {
        const bool byProduct = report.type()==ReportType::SalesByProduct;
        report.setHeaders(byProduct ? QStringList{tr("Код"),tr("Продукт"),tr("Продажби"),tr("Бройки"),tr("Приходи (€)"),tr("Разходи (€)"),tr("Печалба (€)")}
            : QStringList{tr("Дата"),tr("Продажби"),tr("Бройки"),tr("Приходи (€)"),tr("Разходи (€)"),tr("Печалба (€)")});
        report.setMoneyColumns(byProduct ? std::vector<int>{4,5,6} : std::vector<int>{3,4,5});
        report.setNumberColumns(byProduct ? std::vector<int>{2,3} : std::vector<int>{1,2});
        query.prepare(byProduct
            ? "SELECT i.product_sku,i.product_name,COUNT(DISTINCT s.id),SUM(i.quantity),SUM(i.line_total),SUM(i.quantity*i.delivery_price),SUM(i.line_total-i.quantity*i.delivery_price) FROM sale_items i JOIN sales s ON s.id=i.sale_id WHERE s.status='active' AND date(s.created_at) BETWEEN ? AND ? GROUP BY i.product_id,i.product_sku,i.product_name ORDER BY SUM(i.line_total) DESC"
            : "SELECT date(s.created_at),COUNT(DISTINCT s.id),SUM(i.quantity),SUM(i.line_total),SUM(i.quantity*i.delivery_price),SUM(i.line_total-i.quantity*i.delivery_price) FROM sale_items i JOIN sales s ON s.id=i.sale_id WHERE s.status='active' AND date(s.created_at) BETWEEN ? AND ? GROUP BY date(s.created_at) ORDER BY date(s.created_at)");
        query.addBindValue(report.dateFrom().toString(Qt::ISODate)); query.addBindValue(report.dateTo().toString(Qt::ISODate));
        if (!query.exec()) return query.lastError().text();
        while (query.next()) {
            QStringList row;
            for (int column=0; column<report.headers().size(); ++column) row.append(query.value(column).toString());
            rows.push_back(row);
        }
        report.setRows(rows);
    } else {
        report.setHeaders({tr("Код"),tr("Продукт"),tr("Наличност"),tr("Доставчик"),tr("Доставна цена (€)"),
                           tr("Продажна цена (€)"),tr("Стойност по доставка (€)"),tr("Стойност по продажба (€)")});
        report.setMoneyColumns({4,5,6,7}); report.setNumberColumns({2});
        if (!query.exec(report.type()==ReportType::LowStock
                ? "SELECT sku,name,quantity,supplier,delivery_price,sale_price FROM products WHERE quantity<=10 ORDER BY quantity,name"
                : "SELECT sku,name,quantity,supplier,delivery_price,sale_price FROM products ORDER BY name"))
            return query.lastError().text();
        while (query.next()) {
            const int quantity = query.value(2).toInt();
            const qint64 delivery = query.value(4).toLongLong(), price = query.value(5).toLongLong();
            if (quantity>0 && (delivery>std::numeric_limits<qint64>::max()/quantity || price>std::numeric_limits<qint64>::max()/quantity))
                return tr("Стойността на наличностите е твърде голяма.");
            rows.push_back({query.value(0).toString(),query.value(1).toString(),QString::number(quantity),
                query.value(3).toString(),QString::number(delivery),QString::number(price),
                QString::number(delivery*quantity),QString::number(price*quantity)});
        }
        report.setRows(rows);
    }
    query.prepare("INSERT INTO reports(created_at,date_from,date_to,revenue,profit,units_sold,type,title,payload) VALUES(?,?,?,?,?,?,?,?,?)");
    query.addBindValue(report.createdAt().toString(Qt::ISODate)); query.addBindValue(report.dateFrom().toString(Qt::ISODate));
    query.addBindValue(report.dateTo().toString(Qt::ISODate)); query.addBindValue(report.revenue());
    query.addBindValue(report.profit()); query.addBindValue(report.unitsSold()); query.addBindValue(static_cast<int>(report.type()));
    query.addBindValue(report.title().trimmed()); query.addBindValue(payload(report));
    if (!query.exec()) return query.lastError().text();
    report.setId(query.lastInsertId().toInt()); return {};
}
std::vector<Report> SaleRepository::reports(QString &error) const
{
    error.clear(); std::vector<Report> result; QSqlQuery query(m_database);
    if (!query.exec("SELECT id,created_at,date_from,date_to,revenue,profit,units_sold,type,title,payload FROM reports ORDER BY id DESC")) {
        error = query.lastError().text(); return result;
    }
    while (query.next()) {
        Report report;
        if (!readReport(query,report,error)) return {};
        result.push_back(report);
    }
    return result;
}
bool SaleRepository::findReport(int id, Report &report, QString &error) const
{
    error.clear(); QSqlQuery query(m_database);
    query.prepare("SELECT id,created_at,date_from,date_to,revenue,profit,units_sold,type,title,payload FROM reports WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) { error = query.lastError().text(); return false; }
    if (!query.next()) { error = tr("Отчетът не е намерен."); return false; }
    return readReport(query,report,error);
}
QString SaleRepository::renameReport(int id, QString title)
{
    QSqlQuery query(m_database); query.prepare("UPDATE reports SET title=? WHERE id=?");
    query.addBindValue(title.trimmed()); query.addBindValue(id);
    if (!query.exec()) return query.lastError().text();
    return query.numRowsAffected()==1 ? QString() : tr("Отчетът не е намерен.");
}
QString SaleRepository::removeReport(int id)
{
    QSqlQuery query(m_database); query.prepare("DELETE FROM reports WHERE id=?"); query.addBindValue(id);
    if (!query.exec()) return query.lastError().text();
    return query.numRowsAffected()==1 ? QString() : tr("Отчетът не е намерен.");
}
