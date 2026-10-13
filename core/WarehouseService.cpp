#include "WarehouseService.h"
#include "Database.h"
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <limits>
#include <set>

namespace {
QString cents(qint64 value)
{
    return QString("%1.%2").arg(value / 100).arg(value % 100, 2, 10, QChar('0'));
}
}
WarehouseService::WarehouseService() : m_discount(std::make_unique<NoDiscount>()) {}
WarehouseService::WarehouseService(QSqlDatabase database) : WarehouseService() { m_database = database; }
void WarehouseService::setDatabase(QSqlDatabase value) { logout(); m_database = value; }
QString WarehouseService::login(QString username, QString password)
{
    logout();
    if (username.trimmed().isEmpty() || username.size()>64 || password.isEmpty() || password.size()>128)
        return tr("Въведете валидно потребителско име и парола.");
    QString error;
    m_currentUser = UserRepository(m_database).login(username, password, error);
    return error;
}
bool WarehouseService::hasPermission(Permission permission) const
{
    if (!m_currentUser) return false;
    QString error;
    const auto stored = UserRepository(m_database).find(m_currentUser->id(),error);
    return stored && stored->hasPermission(permission);
}
QString WarehouseService::permissionError(Permission permission) const
{
    return hasPermission(permission) ? QString() : tr("Нямате право за тази операция.");
}
void WarehouseService::setDiscountStrategy(std::unique_ptr<DiscountStrategy> value)
{
    m_discount = value ? std::move(value) : std::make_unique<NoDiscount>();
}
std::vector<Product> WarehouseService::products(QString &error) const
{
    QString sessionError;
    if (!m_currentUser || !UserRepository(m_database).find(m_currentUser->id(),sessionError)) {
        error = tr("Влезте в системата."); return {};
    }
    return ProductRepository(m_database).all(error);
}
QString WarehouseService::saveProduct(Product &product)
{
    QString error = permissionError(Permission::ManageProducts);
    if (!error.isEmpty()) return error;
    error = product.validate(); if (!error.isEmpty()) return error;
    if (!m_database.transaction()) return m_database.lastError().text();
    Product before;
    if (product.id()!=0 && !ProductRepository(m_database).find(product.id(),before,error)) {
        m_database.rollback(); return error;
    }
    Product saved = product;
    error = ProductRepository(m_database).save(saved);
    if (error.isEmpty()) error = ProductRepository(m_database).recordHistory(before,saved,
        before.id()==0 ? tr("Добавяне") : tr("Редакция"),m_currentUser->fullName());
    if (!error.isEmpty()) { m_database.rollback(); return error; }
    if (!m_database.commit()) { error = m_database.lastError().text(); m_database.rollback(); return error; }
    product = saved; return {};
}
QString WarehouseService::deleteProduct(int id)
{
    QString error = permissionError(Permission::ManageProducts);
    if (!error.isEmpty()) return error;
    if (id<=0) return tr("Изберете валиден продукт.");
    if (!m_database.transaction()) return m_database.lastError().text();
    Product before;
    if (!ProductRepository(m_database).find(id,before,error)) { m_database.rollback(); return error; }
    error = ProductRepository(m_database).remove(id);
    if (error.isEmpty()) error = ProductRepository(m_database).recordHistory(before,Product(),tr("Изтриване"),m_currentUser->fullName());
    if (!error.isEmpty()) { m_database.rollback(); return tr("Продуктът не може да бъде изтрит. Ако има продажби, историята му трябва да се запази."); }
    if (!m_database.commit()) { error = m_database.lastError().text(); m_database.rollback(); return error; }
    return {};
}
QString WarehouseService::calculate(Sale &sale) const
{
    if (sale.items().empty()) return tr("Добавете поне един продукт.");
    if (sale.customerName().size()>160 || sale.items().size()>1000) return tr("Клиентът може да е до 160 знака, а количката — до 1000 реда.");
    Sale original;
    if (sale.id()!=0) {
        QString error;
        if (!findSale(sale.id(),original,error)) return error;
        if (original.cancelled()) return tr("Отменена продажба не може да се редактира.");
    }
    qint64 total = 0;
    qint64 deliveryTotal = 0;
    std::set<int> productIds;
    auto items = sale.items();
    for (SaleItem &item : items) {
        if (!productIds.insert(item.productId()).second)
            return tr("Обединете количествата за един и същ продукт.");
        if (item.quantity() <= 0) return tr("Количеството трябва да е положително.");
        Product product;
        QString error;
        if (!ProductRepository(m_database).find(item.productId(), product, error)) return error;
        qint64 available = product.quantity();
        for (const SaleItem &reserved : original.items())
            if (reserved.productId()==item.productId()) available += reserved.quantity();
        if (item.quantity() > available) return tr("Недостатъчна наличност за %1. Достъпни: %2.").arg(product.name()).arg(available);
        const qint64 line = m_discount->apply(product.salePrice(), item.quantity(), sale.isVip(),
                                             QDate::currentDate());
        if (line < 0 || line > std::numeric_limits<qint64>::max() - total)
            return tr("Сумата е твърде голяма.");
        if (product.deliveryPrice() > std::numeric_limits<qint64>::max() / item.quantity())
            return tr("Доставната стойност е твърде голяма.");
        const qint64 deliveryLine = product.deliveryPrice() * item.quantity();
        if (deliveryLine > std::numeric_limits<qint64>::max() - deliveryTotal)
            return tr("Доставната стойност е твърде голяма.");
        deliveryTotal += deliveryLine;
        item.setProductName(product.name()); item.setProductSku(product.sku());
        item.setUnitPrice(product.salePrice()); item.setDeliveryPrice(product.deliveryPrice());
        item.setLineTotal(line);
        total += line;
    }
    sale.setItems(items); sale.setTotal(total); sale.setDiscountName(m_discount->name());
    return {};
}
QString WarehouseService::previewSale(Sale &sale) const
{
    const QString error = permissionError(Permission::RegisterSale);
    return error.isEmpty() ? calculate(sale) : error;
}
QString WarehouseService::registerSale(Sale &sale)
{
    QString error = permissionError(Permission::RegisterSale);
    if (!error.isEmpty()) return error;
    if (sale.id() != 0) return tr("Продажбата вече е записана.");
    if (!m_database.transaction()) return m_database.lastError().text();
    Sale prepared = sale;
    error = calculate(prepared);
    if (error.isEmpty()) {
        if (prepared.customerName().trimmed().isEmpty())
            prepared.setCustomerName(tr("Клиент на дребно"));
        prepared.setUserId(m_currentUser->id());
        prepared.setCreatedAt(QDateTime::currentDateTime());
    }
    if (error.isEmpty()) error = SaleRepository(m_database).save(prepared);
    if (error.isEmpty()) {
        for (const SaleItem &item : prepared.items()) {
            error = changeStock(item.productId(),-item.quantity(),tr("Продажба"),prepared.id());
            if (!error.isEmpty()) break;
        }
    }
    if (!error.isEmpty()) { m_database.rollback(); return error; }
    if (!m_database.commit()) {
        error = m_database.lastError().text(); m_database.rollback(); return error;
    }
    sale = prepared;
    return {};
}
QString WarehouseService::receipt(int saleId, bool invoice, QString &error) const
{
    error = permissionError(Permission::RegisterSale);
    if (!error.isEmpty()) return {};
    Sale sale;
    if (!findSale(saleId,sale,error)) return {};
    QString html = "<html><head><meta charset='utf-8'></head><body>";
    html += "<h1>" + (invoice ? tr("Фактура") : tr("Касова бележка")) + "</h1>";
    if (sale.cancelled()) html += "<h2 style='color:#b91c1c'>" + tr("ОТМЕНЕНА ПРОДАЖБА") + "</h2>";
    html += "<p>" + tr("Номер: ") + QString::number(sale.id()) + "<br>"
        + tr("Дата: ") + sale.createdAt().toString("dd.MM.yyyy HH:mm").toHtmlEscaped() + "<br>"
        + tr("Клиент: ") + sale.customerName().toHtmlEscaped() + "<br>"
        + tr("Отстъпка: ") + sale.discountName().toHtmlEscaped() + "</p>";
    html += "<table border='1' cellspacing='0' cellpadding='6'><tr><th>" + tr("Продукт")
        + "</th><th>" + tr("Код") + "</th><th>" + tr("Количество")
        + "</th><th>" + tr("Единична цена (€)") + "</th><th>" + tr("Сума (€)") + "</th></tr>";
    for (const SaleItem &item : sale.items()) {
        html += "<tr><td>" + item.productName().toHtmlEscaped() + "</td><td>"
            + item.productSku().toHtmlEscaped() + "</td><td>" + QString::number(item.quantity())
            + "</td><td>" + cents(item.unitPrice()) + "</td><td>" + cents(item.lineTotal()) + "</td></tr>";
    }
    html += "</table><h2>" + tr("Общо: ") + cents(sale.total()) + " "
        + tr("€") + "</h2></body></html>";
    return html;
}
QString WarehouseService::generateReport(Report &report)
{
    const QString error = permissionError(Permission::ViewReports);
    if (!error.isEmpty()) return error;
    if (!report.dateFrom().isValid() || !report.dateTo().isValid() || report.dateFrom() > report.dateTo())
        return tr("Невалиден период за отчета.");
    if (static_cast<int>(report.type())<0 || static_cast<int>(report.type())>4 || report.title().size()>160)
        return tr("Невалиден тип отчет или заглавие над 160 знака.");
    if (!m_database.transaction()) return m_database.lastError().text();
    Report saved = report;
    QString failure = SaleRepository(m_database).generate(saved);
    if (!failure.isEmpty()) { m_database.rollback(); return failure; }
    if (!m_database.commit()) { failure = m_database.lastError().text(); m_database.rollback(); return failure; }
    report = saved; return {};
}
std::vector<Report> WarehouseService::reports(QString &error) const
{
    error = permissionError(Permission::ViewReports);
    return error.isEmpty() ? SaleRepository(m_database).reports(error) : std::vector<Report>();
}
std::vector<std::shared_ptr<User>> WarehouseService::users(QString &error) const
{
    error = permissionError(Permission::ManageUsers);
    return error.isEmpty() ? UserRepository(m_database).all(error) : std::vector<std::shared_ptr<User>>();
}
QString WarehouseService::saveUser(User &user)
{
    QString error=permissionError(Permission::ManageUsers); if (!error.isEmpty()) return error;
    if (user.id()==m_currentUser->id() && user.roleName()!="admin")
        return tr("Не можете да премахнете собствената си администраторска роля.");
    if (!m_database.transaction()) return m_database.lastError().text();
    if (user.id()!=0) {
        const auto previous=UserRepository(m_database).find(user.id(),error);
        if (!previous) { m_database.rollback(); return error; }
        if (previous->roleName()=="admin" && user.roleName()!="admin") {
            QSqlQuery count(m_database);
            if (!count.exec("SELECT COUNT(*) FROM users WHERE role='admin'") || !count.next()) {
                error=count.lastError().text(); m_database.rollback(); return error;
            }
            if (count.value(0).toInt()<=1) { m_database.rollback(); return tr("Последният администратор не може да бъде понижен."); }
        }
    }
    const int originalId=user.id();
    error=UserRepository(m_database).save(user);
    if (!error.isEmpty()) { m_database.rollback(); user.setId(originalId); return error; }
    if (!m_database.commit()) { error=m_database.lastError().text(); m_database.rollback(); user.setId(originalId); return error; }
    if (user.id()==m_currentUser->id())
        m_currentUser=std::make_shared<Admin>(user.id(),user.username(),user.passwordHash(),user.fullName());
    return {};
}
QString WarehouseService::deleteUser(int id)
{
    QString error=permissionError(Permission::ManageUsers); if (!error.isEmpty()) return error;
    if (id==m_currentUser->id()) return tr("Не можете да изтриете текущия потребител.");
    if (!m_database.transaction()) return m_database.lastError().text();
    const auto user=UserRepository(m_database).find(id,error);
    if (!user) { m_database.rollback(); return error; }
    if (user->roleName()=="admin") {
        QSqlQuery count(m_database);
        if (!count.exec("SELECT COUNT(*) FROM users WHERE role='admin'") || !count.next()) {
            error=count.lastError().text(); m_database.rollback(); return error;
        }
        if (count.value(0).toInt()<=1) { m_database.rollback(); return tr("Последният администратор не може да бъде изтрит."); }
    }
    error=UserRepository(m_database).remove(id);
    if (!error.isEmpty()) { m_database.rollback(); return error; }
    if (!m_database.commit()) { error=m_database.lastError().text(); m_database.rollback(); return error; }
    return {};
}
QString WarehouseService::saveUser(User &user, const QString &password)
{
    const QString error=permissionError(Permission::ManageUsers); if (!error.isEmpty()) return error;
    if (!password.isEmpty()) {
        if (password.size()<8 || password.size()>128) return tr("Новата парола трябва да е 8–128 знака.");
        user.setPasswordHash(hashPassword(password));
    } else {
        QString failure; const auto original=UserRepository(m_database).find(user.id(),failure);
        if (!original) return tr("Новият потребител изисква парола.");
        user.setPasswordHash(original->passwordHash());
    }
    return saveUser(user);
}
std::vector<ProductHistoryEntry> WarehouseService::productHistory(int productId, QString &error) const
{
    if (!m_currentUser) { error = tr("Влезте в системата."); return {}; }
    QString sessionError;
    if (!UserRepository(m_database).find(m_currentUser->id(),sessionError)) { error = tr("Невалиден профил."); return {}; }
    if (productId<=0) { error = tr("Невалиден продукт."); return {}; }
    return ProductRepository(m_database).history(productId,error);
}
QString WarehouseService::saleAccess(const Sale &sale) const
{
    const QString error = permissionError(Permission::RegisterSale);
    if (!error.isEmpty()) return error;
    if (!hasPermission(Permission::ManageUsers) && sale.userId()!=m_currentUser->id())
        return tr("Можете да преглеждате и променяте само собствените си продажби.");
    return {};
}

QStringList WarehouseService::overview(QString &error) const
{
    const auto catalog=products(error); if (!error.isEmpty()) return {};
    qint64 units=0; int low=0;
    for (const Product &product : catalog) { units+=product.quantity(); if (product.quantity()<=10) ++low; }
    const int userId=hasPermission(Permission::ViewReports) ? 0 : m_currentUser->id();
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*),COALESCE(SUM(total),0) FROM sales WHERE status='active' AND date(created_at)=? AND (?=0 OR user_id=?)");
    query.addBindValue(QDate::currentDate().toString(Qt::ISODate)); query.addBindValue(userId); query.addBindValue(userId);
    if (!query.exec() || !query.next()) { error=query.lastError().text(); return {}; }
    return {QString::number(catalog.size()),QString::number(low),QString::number(units),
            query.value(0).toString(),query.value(1).toString()};
}
std::vector<Sale> WarehouseService::sales(QString &error) const
{
    error = permissionError(Permission::RegisterSale);
    if (!error.isEmpty()) return {};
    return SaleRepository(m_database).all(error,hasPermission(Permission::ManageUsers) ? 0 : m_currentUser->id());
}
bool WarehouseService::findSale(int id, Sale &sale, QString &error) const
{
    error = permissionError(Permission::RegisterSale); if (!error.isEmpty()) return false;
    if (!SaleRepository(m_database).find(id,sale,error)) return false;
    error = saleAccess(sale); return error.isEmpty();
}
QString WarehouseService::changeStock(int productId, int delta, QString action, int saleId)
{
    Product before; QString error;
    if (!ProductRepository(m_database).find(productId,before,error)) return error;
    const qint64 quantity = qint64(before.quantity())+delta;
    if (quantity<0 || quantity>std::numeric_limits<int>::max()) return tr("Невалидна наличност.");
    QSqlQuery query(m_database);
    query.prepare("UPDATE products SET quantity=? WHERE id=? AND quantity=?");
    query.addBindValue(quantity); query.addBindValue(productId); query.addBindValue(before.quantity());
    if (!query.exec()) return query.lastError().text();
    if (query.numRowsAffected()!=1) return tr("Наличността се промени. Обновете данните.");
    Product after = before; after.setQuantity(static_cast<int>(quantity));
    return ProductRepository(m_database).recordHistory(before,after,action,m_currentUser->fullName(),saleId);
}
QString WarehouseService::updateSale(Sale &sale)
{
    QString error = permissionError(Permission::RegisterSale); if (!error.isEmpty()) return error;
    if (!m_database.transaction()) return m_database.lastError().text();
    Sale original;
    if (!findSale(sale.id(),original,error)) { m_database.rollback(); return error; }
    if (original.cancelled()) { m_database.rollback(); return tr("Продажбата е отменена."); }
    Sale prepared = sale;
    for (const SaleItem &item : original.items()) {
        error = changeStock(item.productId(),item.quantity(),tr("Възстановяване при редакция"),original.id());
        if (!error.isEmpty()) break;
    }
    // Stock is now restored, so calculate against ordinary available stock.
    prepared.setId(0);
    if (error.isEmpty()) error = calculate(prepared);
    prepared.setId(original.id()); prepared.setCreatedAt(original.createdAt()); prepared.setUserId(original.userId());
    if (prepared.customerName().trimmed().isEmpty()) prepared.setCustomerName(tr("Клиент на дребно"));
    if (error.isEmpty()) error = SaleRepository(m_database).save(prepared);
    if (error.isEmpty()) for (const SaleItem &item : prepared.items()) {
        error = changeStock(item.productId(),-item.quantity(),tr("Редакция на продажба"),prepared.id());
        if (!error.isEmpty()) break;
    }
    if (!error.isEmpty()) { m_database.rollback(); return error; }
    if (!m_database.commit()) { error = m_database.lastError().text(); m_database.rollback(); return error; }
    sale = prepared; return {};
}
QString WarehouseService::cancelSale(int id)
{
    QString error = permissionError(Permission::RegisterSale); if (!error.isEmpty()) return error;
    if (!m_database.transaction()) return m_database.lastError().text();
    Sale sale;
    if (!findSale(id,sale,error)) { m_database.rollback(); return error; }
    if (sale.cancelled()) { m_database.rollback(); return tr("Продажбата вече е отменена."); }
    for (const SaleItem &item : sale.items()) {
        error = changeStock(item.productId(),item.quantity(),tr("Отмяна на продажба"),id);
        if (!error.isEmpty()) break;
    }
    if (error.isEmpty()) {
        QSqlQuery query(m_database); query.prepare("UPDATE sales SET status='cancelled' WHERE id=? AND status='active'");
        query.addBindValue(id);
        if (!query.exec()) error = query.lastError().text();
        else if (query.numRowsAffected()!=1) error = tr("Продажбата е променена.");
    }
    if (!error.isEmpty()) { m_database.rollback(); return error; }
    if (!m_database.commit()) { error = m_database.lastError().text(); m_database.rollback(); return error; }
    return {};
}
bool WarehouseService::findReport(int id, Report &report, QString &error) const
{
    error = permissionError(Permission::ViewReports);
    return error.isEmpty() && SaleRepository(m_database).findReport(id,report,error);
}
QString WarehouseService::renameReport(int id, QString title)
{
    const QString error = permissionError(Permission::ViewReports); if (!error.isEmpty()) return error;
    if (title.trimmed().isEmpty() || title.size()>160) return tr("Заглавието трябва да е между 1 и 160 знака.");
    return SaleRepository(m_database).renameReport(id,title);
}
QString WarehouseService::deleteReport(int id)
{
    const QString error = permissionError(Permission::ViewReports);
    return error.isEmpty() ? SaleRepository(m_database).removeReport(id) : error;
}
