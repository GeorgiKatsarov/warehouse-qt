#pragma once
#include "Product.h"
#include <QAbstractTableModel>
#include <vector>

/// Adapts product data to QTableView.
class ProductTableModel : public QAbstractTableModel
{
public:
    /// Constructs an empty model.
    explicit ProductTableModel(QObject *parent = nullptr);
    /// Constructs a model with product rows.
    ProductTableModel(std::vector<Product> products, QObject *parent);
    /// Returns model rows.
    std::vector<Product> products() const { return m_products; }
    /// Replaces rows and notifies the view.
    void setProducts(std::vector<Product> value);
    /// Returns row count.
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    /// Returns column count.
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    /// Returns cell display data.
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    /// Returns Bulgarian column headers.
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
private:
    std::vector<Product> m_products;
};

/// Formats integer cents in the GUI.
QString formatCents(qint64 value);
