#include "WarehouseService.h"
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
    QString error;
    m_currentUser = UserRepository(m_database).login(username, password, error);
    return error;
}
bool WarehouseService::hasPermission(Permission permission) const
{
    return m_currentUser && m_currentUser->hasPermission(permission);
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
    if (!m_currentUser) { error = tr("Влезте в системата."); return {}; }
    return ProductRepository(m_database).all(error);
}
QString WarehouseService::saveProduct(Product &product)
{
    const QString error = permissionError(Permission::ManageProducts);
    return error.isEmpty() ? ProductRepository(m_database).save(product) : error;
}
QString WarehouseService::deleteProduct(int id)
{
    const QString error = permissionError(Permission::ManageProducts);
    return error.isEmpty() ? ProductRepository(m_database).remove(id) : error;
}
QString WarehouseService::calculate(Sale &sale) const
{
    if (sale.items().empty()) return tr("Добавете поне един продукт.");
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
        if (item.quantity() > product.quantity()) return tr("Недостатъчна наличност.");
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
        for (const SaleItem &item : prepared.items()) {
            QSqlQuery query(m_database);
            query.prepare("UPDATE products SET quantity=quantity-? WHERE id=? AND quantity>=?");
            query.addBindValue(item.quantity()); query.addBindValue(item.productId());
            query.addBindValue(item.quantity());
            if (!query.exec()) { error = query.lastError().text(); break; }
            if (query.numRowsAffected() != 1) { error = tr("Недостатъчна наличност."); break; }
        }
    }
    if (error.isEmpty()) error = SaleRepository(m_database).save(prepared);
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
    if (!SaleRepository(m_database).find(saleId, sale, error)) return {};
    QString html = "<html><head><meta charset='utf-8'></head><body>";
    html += "<h1>" + (invoice ? tr("Фактура") : tr("Касова бележка")) + "</h1>";
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
    return SaleRepository(m_database).generate(report);
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
    const QString error = permissionError(Permission::ManageUsers);
    if (!error.isEmpty()) return error;
    if (user.id() == m_currentUser->id() && user.roleName() != "admin")
        return tr("Не можете да премахнете собствената си администраторска роля.");
    const QString saved = UserRepository(m_database).save(user);
    if (saved.isEmpty() && user.id() == m_currentUser->id())
        m_currentUser = std::make_shared<Admin>(user.id(),user.username(),user.passwordHash(),user.fullName());
    return saved;
}
QString WarehouseService::deleteUser(int id)
{
    const QString error = permissionError(Permission::ManageUsers);
    if (!error.isEmpty()) return error;
    if (id == m_currentUser->id()) return tr("Не можете да изтриете текущия потребител.");
    return UserRepository(m_database).remove(id);
}
