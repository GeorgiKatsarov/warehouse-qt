#include "Product.h"
#include <QCoreApplication>

Product::Product(int id, QString name, QString sku, int quantity, QString supplier,
                 qint64 deliveryPrice, qint64 salePrice)
    : m_id(id), m_name(name), m_sku(sku), m_quantity(quantity), m_supplier(supplier),
      m_deliveryPrice(deliveryPrice), m_salePrice(salePrice)
{
}

QString Product::validate() const
{
    if (m_name.trimmed().isEmpty())
        return QCoreApplication::translate("Product", "Въведете име на продукта.");
    if (m_sku.trimmed().isEmpty())
        return QCoreApplication::translate("Product", "Въведете продуктов код.");
    if (m_quantity < 0)
        return QCoreApplication::translate("Product", "Наличността не може да бъде отрицателна.");
    if (m_deliveryPrice < 0 || m_salePrice < 0)
        return QCoreApplication::translate("Product", "Цената не може да бъде отрицателна.");
    return {};
}
