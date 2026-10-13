#pragma once
#include <QDateTime>
#include <QString>
#include <vector>

/// SaleItem data stored by the core.
class SaleItem
{
public:
    /// Constructs empty data.
    SaleItem() = default;
    /// Constructs the main fields.
    SaleItem(int productId, int quantity) : m_productId(productId), m_quantity(quantity) {}
    /// Returns productId.
    int productId() const { return m_productId; }
    /// Sets productId.
    void setProductId(int value) { m_productId = value; }
    /// Returns quantity.
    int quantity() const { return m_quantity; }
    /// Sets quantity.
    void setQuantity(int value) { m_quantity = value; }
    /// Returns productName.
    QString productName() const { return m_productName; }
    /// Sets productName.
    void setProductName(QString value) { m_productName = value; }
    /// Returns productSku.
    QString productSku() const { return m_productSku; }
    /// Sets productSku.
    void setProductSku(QString value) { m_productSku = value; }
    /// Returns unitPrice.
    qint64 unitPrice() const { return m_unitPrice; }
    /// Sets unitPrice.
    void setUnitPrice(qint64 value) { m_unitPrice = value; }
    /// Returns deliveryPrice.
    qint64 deliveryPrice() const { return m_deliveryPrice; }
    /// Sets deliveryPrice.
    void setDeliveryPrice(qint64 value) { m_deliveryPrice = value; }
    /// Returns lineTotal.
    qint64 lineTotal() const { return m_lineTotal; }
    /// Sets lineTotal.
    void setLineTotal(qint64 value) { m_lineTotal = value; }
private:
    int m_productId = 0;
    int m_quantity = 0;
    QString m_productName;
    QString m_productSku;
    qint64 m_unitPrice = 0;
    qint64 m_deliveryPrice = 0;
    qint64 m_lineTotal = 0;
};

/// Sale data stored by the core.
class Sale
{
public:
    /// Constructs empty data.
    Sale() = default;
    /// Constructs the main fields.
    Sale(QString customerName, bool isVip) : m_customerName(customerName), m_isVip(isVip) {}
    /// Returns id.
    int id() const { return m_id; }
    /// Sets id.
    void setId(int value) { m_id = value; }
    /// Returns createdAt.
    QDateTime createdAt() const { return m_createdAt; }
    /// Sets createdAt.
    void setCreatedAt(QDateTime value) { m_createdAt = value; }
    /// Returns userId.
    int userId() const { return m_userId; }
    /// Sets userId.
    void setUserId(int value) { m_userId = value; }
    /// Returns customerName.
    QString customerName() const { return m_customerName; }
    /// Sets customerName.
    void setCustomerName(QString value) { m_customerName = value; }
    /// Returns isVip.
    bool isVip() const { return m_isVip; }
    /// Sets isVip.
    void setIsVip(bool value) { m_isVip = value; }
    /// Returns discountName.
    QString discountName() const { return m_discountName; }
    /// Sets discountName.
    void setDiscountName(QString value) { m_discountName = value; }
    /// Returns total.
    qint64 total() const { return m_total; }
    /// Sets total.
    void setTotal(qint64 value) { m_total = value; }
    /// Returns items.
    std::vector<SaleItem> items() const { return m_items; }
    /// Sets items.
    void setItems(std::vector<SaleItem> value) { m_items = value; }
private:
    int m_id = 0;
    QDateTime m_createdAt;
    int m_userId = 0;
    QString m_customerName;
    bool m_isVip = false;
    QString m_discountName;
    qint64 m_total = 0;
    std::vector<SaleItem> m_items;
};

/// Report data stored by the core.
class Report
{
public:
    /// Constructs empty data.
    Report() = default;
    /// Constructs the main fields.
    Report(QDate from, QDate to) : m_dateFrom(from), m_dateTo(to) {}
    /// Returns id.
    int id() const { return m_id; }
    /// Sets id.
    void setId(int value) { m_id = value; }
    /// Returns createdAt.
    QDateTime createdAt() const { return m_createdAt; }
    /// Sets createdAt.
    void setCreatedAt(QDateTime value) { m_createdAt = value; }
    /// Returns dateFrom.
    QDate dateFrom() const { return m_dateFrom; }
    /// Sets dateFrom.
    void setDateFrom(QDate value) { m_dateFrom = value; }
    /// Returns dateTo.
    QDate dateTo() const { return m_dateTo; }
    /// Sets dateTo.
    void setDateTo(QDate value) { m_dateTo = value; }
    /// Returns revenue.
    qint64 revenue() const { return m_revenue; }
    /// Sets revenue.
    void setRevenue(qint64 value) { m_revenue = value; }
    /// Returns profit.
    qint64 profit() const { return m_profit; }
    /// Sets profit.
    void setProfit(qint64 value) { m_profit = value; }
    /// Returns unitsSold.
    qint64 unitsSold() const { return m_unitsSold; }
    /// Sets unitsSold.
    void setUnitsSold(qint64 value) { m_unitsSold = value; }
private:
    int m_id = 0;
    QDateTime m_createdAt;
    QDate m_dateFrom;
    QDate m_dateTo;
    qint64 m_revenue = 0;
    qint64 m_profit = 0;
    qint64 m_unitsSold = 0;
};
