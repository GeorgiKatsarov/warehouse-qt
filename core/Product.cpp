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
    if (m_id<0 || m_name.size()>160 || m_sku.size()>64 || m_supplier.size()>160)
        return tr("Невалидни данни: име и доставчик до 160 знака, код до 64 знака.");
    if (m_name.trimmed().isEmpty())
        return tr("Въведете име на продукта.");
    if (m_sku.trimmed().isEmpty())
        return tr("Въведете продуктов код.");
    if (m_quantity < 0)
        return tr("Наличността не може да бъде отрицателна.");
    if (m_deliveryPrice < 0 || m_salePrice < 0)
        return tr("Цената не може да бъде отрицателна.");
    return {};
}
