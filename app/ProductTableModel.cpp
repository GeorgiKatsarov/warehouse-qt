#include "ProductTableModel.h"
#include <QCoreApplication>

QString formatCents(qint64 value)
{
    const bool negative = value < 0;
    // Unsigned conversion also handles the minimum signed integer.
    const quint64 absolute = negative ? quint64(-(value + 1)) + 1 : quint64(value);
    return QString("%1%2.%3 %4").arg(negative ? "-" : "").arg(absolute / 100)
        .arg(absolute % 100, 2, 10, QChar('0'))
        .arg(QCoreApplication::translate("Money", "€"));
}
ProductTableModel::ProductTableModel(QObject *parent) : QAbstractTableModel(parent) {}
ProductTableModel::ProductTableModel(std::vector<Product> products, QObject *parent)
    : QAbstractTableModel(parent), m_products(products) {}
void ProductTableModel::setProducts(std::vector<Product> value)
{
    beginResetModel(); m_products = value; endResetModel();
}
int ProductTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_products.size());
}
int ProductTableModel::columnCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : 6; }
QVariant ProductTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole || index.row() < 0
        || index.row() >= rowCount() || index.column() < 0 || index.column() >= columnCount())
        return {};
    const Product &product = m_products.at(index.row());
    switch (index.column()) {
    case 0: return product.name();
    case 1: return product.sku();
    case 2: return product.quantity();
    case 3: return product.supplier();
    case 4: return formatCents(product.deliveryPrice());
    case 5: return formatCents(product.salePrice());
    default: return {};
    }
}
QVariant ProductTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return {};
    if (orientation == Qt::Vertical) return section + 1;
    const QStringList headers = {tr("Продукт"),tr("Код"),tr("Наличност"),tr("Доставчик"),
                                 tr("Доставна цена"),tr("Продажна цена")};
    return section >= 0 && section < headers.size() ? headers.at(section) : QVariant();
}
