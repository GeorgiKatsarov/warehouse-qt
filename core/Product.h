#pragma once
#include <QCoreApplication>
#include <QString>

/// Product data; all prices are integer cents.
class Product
{
    Q_DECLARE_TR_FUNCTIONS(Product)
public:
    /// Constructs an empty product.
    Product() = default;
    /// Constructs a product with all stored fields.
    Product(int id, QString name, QString sku, int quantity, QString supplier,
            qint64 deliveryPrice, qint64 salePrice);
    /// Returns an empty string when valid, otherwise a Bulgarian error.
    QString validate() const;
    /// Returns id.
    int id() const { return m_id; }
    /// Sets id.
    void setId(int value) { m_id = value; }
    /// Returns name.
    QString name() const { return m_name; }
    /// Sets name.
    void setName(QString value) { m_name = value; }
    /// Returns sku.
    QString sku() const { return m_sku; }
    /// Sets sku.
    void setSku(QString value) { m_sku = value; }
    /// Returns quantity.
    int quantity() const { return m_quantity; }
    /// Sets quantity.
    void setQuantity(int value) { m_quantity = value; }
    /// Returns supplier.
    QString supplier() const { return m_supplier; }
    /// Sets supplier.
    void setSupplier(QString value) { m_supplier = value; }
    /// Returns deliveryPrice.
    qint64 deliveryPrice() const { return m_deliveryPrice; }
    /// Sets deliveryPrice.
    void setDeliveryPrice(qint64 value) { m_deliveryPrice = value; }
    /// Returns salePrice.
    qint64 salePrice() const { return m_salePrice; }
    /// Sets salePrice.
    void setSalePrice(qint64 value) { m_salePrice = value; }
private:
    int m_id = 0;
    QString m_name;
    QString m_sku;
    int m_quantity = 0;
    QString m_supplier;
    qint64 m_deliveryPrice = 0;
    qint64 m_salePrice = 0;
};
