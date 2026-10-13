#pragma once
#include "Product.h"
#include <QDialog>

/// Edits product input; core validates before acceptance.
class ProductDialog : public QDialog
{
public:
    /// Constructs a new-product editor.
    explicit ProductDialog(QWidget *parent = nullptr);
    /// Constructs an editor for a product.
    ProductDialog(Product product, QWidget *parent);
    /// Returns validated input.
    Product product() const { return m_product; }
    /// Replaces the product and updates input widgets.
    void setProduct(Product value);
private:
    Product m_product;
};
