#pragma once
#include "Product.h"
#include <QDialog>
class WarehouseService;

/// Product profile with a read-only stock and price timeline.
class ProductDetailDialog : public QDialog
{
public:
    /// Constructs an empty profile.
    explicit ProductDetailDialog(QWidget *parent = nullptr);
    /// Constructs a product profile.
    ProductDetailDialog(WarehouseService *service, Product product, QWidget *parent);
    /// Returns the service.
    WarehouseService *service() const { return m_service; }
    /// Sets the service and reloads the timeline.
    void setService(WarehouseService *value);
    /// Returns the product.
    Product product() const { return m_product; }
    /// Replaces the product and reloads the profile.
    void setProduct(Product value);
private:
    WarehouseService *m_service = nullptr;
    Product m_product;
    void refresh();
};
