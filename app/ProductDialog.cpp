#include "ProductDialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpressionValidator>

ProductDialog::ProductDialog(QWidget *parent) : ProductDialog(Product(),parent) {}
ProductDialog::ProductDialog(Product product, QWidget *parent) : QDialog(parent),m_product(product)
{
    setWindowTitle(product.id() ? tr("Редактиране на продукт") : tr("Добавяне на продукт"));
    auto form = new QFormLayout(this);
    auto name = new QLineEdit(product.name(),this); name->setObjectName("productName");
    name->setMaxLength(160); name->setPlaceholderText(tr("Наименование на продукта"));
    auto sku = new QLineEdit(product.sku(),this); sku->setObjectName("productSku");
    sku->setMaxLength(64); sku->setPlaceholderText(tr("Уникален продуктов код"));
    auto supplier = new QLineEdit(product.supplier(),this); supplier->setObjectName("supplier");
    supplier->setMaxLength(160);
    auto quantity = new QSpinBox(this); quantity->setObjectName("quantity");
    quantity->setRange(0,1000000000); quantity->setValue(product.quantity());
    auto delivery = new QLineEdit(this); delivery->setObjectName("deliveryPrice");
    auto price = new QLineEdit(this); price->setObjectName("salePrice");
    for (auto field : {delivery,price})
        field->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{1,12}([.,][0-9]{1,2})?"),field));
    auto decimal = [](qint64 value) { return QString("%1.%2").arg(value/100).arg(value%100,2,10,QChar('0')); };
    delivery->setText(decimal(product.deliveryPrice())); price->setText(decimal(product.salePrice()));
    form->addRow(tr("Име:"),name); form->addRow(tr("Продуктов код:"),sku);
    form->addRow(tr("Наличност:"),quantity); form->addRow(tr("Доставчик:"),supplier);
    form->addRow(tr("Доставна цена (€):"),delivery); form->addRow(tr("Продажна цена (€):"),price);
    auto error = new QLabel(this); error->setObjectName("productError"); error->setWordWrap(true); form->addRow(error);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Запис"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отказ")); form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,this,[=] {
        if (!delivery->hasAcceptableInput() || !price->hasAcceptableInput()) {
            error->setText(tr("Въведете цена с най-много два знака след десетичния разделител.")); return;
        }
        auto parse = [](QString text) {
            text.replace(',','.');
            const QStringList parts = text.split('.');
            return parts.at(0).toLongLong()*100 + (parts.size()==2 ? parts.at(1).leftJustified(2,'0').toLongLong() : 0);
        };
        Product edited(m_product.id(),name->text(),sku->text(),quantity->value(),supplier->text(),
                       parse(delivery->text()),parse(price->text()));
        const QString validation = edited.validate();
        if (!validation.isEmpty()) { error->setText(validation); return; }
        m_product = edited; accept();
    });
}
void ProductDialog::setProduct(Product value)
{
    m_product = value;
    findChild<QLineEdit*>("productName")->setText(value.name());
    findChild<QLineEdit*>("productSku")->setText(value.sku());
    findChild<QLineEdit*>("supplier")->setText(value.supplier());
    findChild<QSpinBox*>("quantity")->setValue(value.quantity());
    findChild<QLineEdit*>("deliveryPrice")->setText(QString("%1.%2").arg(value.deliveryPrice()/100).arg(value.deliveryPrice()%100,2,10,QChar('0')));
    findChild<QLineEdit*>("salePrice")->setText(QString("%1.%2").arg(value.salePrice()/100).arg(value.salePrice()%100,2,10,QChar('0')));
}
