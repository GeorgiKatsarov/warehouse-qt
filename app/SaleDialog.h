#pragma once
#include "Sale.h"
#include <QDialog>
class WarehouseService;

/// Checkout form, also embedded in the Sales tab.
class SaleDialog : public QDialog
{
public:
    /// Constructs an unconnected checkout.
    explicit SaleDialog(QWidget *parent = nullptr);
    /// Constructs a checkout for a service.
    SaleDialog(WarehouseService *service, QWidget *parent);
    /// Returns the service.
    WarehouseService *service() const { return m_service; }
    /// Sets the service and clears the cart.
    void setService(WarehouseService *value);
    /// Returns the cart and calculated values.
    Sale sale() const { return m_sale; }
    /// Replaces the cart and refreshes input.
    void setSale(Sale value);
    /// Reloads products from current stock.
    void reloadProducts();
private:
    WarehouseService *m_service = nullptr;
    Sale m_sale;
    void refresh();
    void changeStrategy();
};
