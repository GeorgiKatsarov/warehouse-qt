#include "SaleDialog.h"
#include "WarehouseService.h"
#include "ReceiptDialog.h"
#include "ProductTableModel.h"
#include "UiHelpers.h"
#include <QGroupBox>
#include <QCompleter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateEdit>
#include <limits>

SaleDialog::SaleDialog(QWidget *parent) : SaleDialog(nullptr,parent) {}
SaleDialog::SaleDialog(WarehouseService *service, QWidget *parent) : QDialog(parent),m_service(service)
{
    setWindowTitle(tr("Продажба")); resize(900,600);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(20,18,20,18);
    addPageHeading(layout,tr("Нова продажба"),tr("Добавете продукти, изберете отстъпка и проверете сумата преди запис."));
    auto entry = new QHBoxLayout;
    auto product = new QComboBox(this); product->setObjectName("saleProduct"); entry->addWidget(product,1);
    product->setEditable(true); product->setInsertPolicy(QComboBox::NoInsert);
    product->completer()->setFilterMode(Qt::MatchContains); product->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    auto quantity = new QSpinBox(this); quantity->setObjectName("saleQuantity");
    quantity->setRange(1,1000000000); entry->addWidget(quantity);
    auto add = new QPushButton(tr("Добавяне в количката"),this); add->setObjectName("addToCart");
    entry->addWidget(add); layout->addLayout(entry);
    add->setProperty("primary",true);
    auto cart = new QTableWidget(0,4,this); cart->setObjectName("cartTable");
    cart->setHorizontalHeaderLabels({tr("Продукт"),tr("Количество"),tr("Единична цена"),tr("Сума")});
    cart->setSelectionBehavior(QAbstractItemView::SelectRows);
    cart->setSelectionMode(QAbstractItemView::SingleSelection); cart->setEditTriggers(QAbstractItemView::NoEditTriggers);
    cart->setAlternatingRowColors(true); cart->verticalHeader()->hide(); cart->verticalHeader()->setDefaultSectionSize(42);
    cart->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); layout->addWidget(cart,1);
    auto remove = new QPushButton(tr("Премахване на ред"),this); layout->addWidget(remove);
    auto form = new QFormLayout;
    auto customer = new QLineEdit(this); customer->setObjectName("customerName");
    customer->setMaxLength(160); customer->setPlaceholderText(tr("Оставете празно за клиент на дребно"));
    auto vip = new QCheckBox(tr("VIP клиент"),this); vip->setObjectName("vip");
    auto discount = new QComboBox(this); discount->setObjectName("discount");
    discount->addItems({tr("Без отстъпка"),tr("За количество"),tr("VIP — 10%"),tr("Сезонна")});
    auto seasonal = new QWidget(this); auto seasonLayout = new QHBoxLayout(seasonal);
    auto percent = new QSpinBox(seasonal); percent->setObjectName("seasonPercent"); percent->setRange(0,100); percent->setSuffix("%");
    auto from = new QDateEdit(QDate::currentDate(),seasonal); from->setObjectName("seasonFrom"); from->setCalendarPopup(true);
    auto to = new QDateEdit(QDate::currentDate(),seasonal); to->setObjectName("seasonTo"); to->setCalendarPopup(true);
    for (auto date : {from,to}) date->setDisplayFormat("dd.MM.yyyy");
    seasonLayout->addWidget(new QLabel(tr("От:"),seasonal)); seasonLayout->addWidget(from);
    seasonLayout->addWidget(new QLabel(tr("До:"),seasonal)); seasonLayout->addWidget(to);
    seasonLayout->addWidget(percent); seasonal->setVisible(false);
    form->addRow(tr("Клиент:"),customer); form->addRow(vip); form->addRow(tr("Отстъпка:"),discount);
    form->addRow(seasonal);
    auto invoice = new QCheckBox(tr("Издаване на фактура"),this); invoice->setObjectName("invoice"); form->addRow(invoice);
    layout->addLayout(form);
    auto total = new QLabel(this); total->setObjectName("saleTotal"); layout->addWidget(total);
    total->setProperty("metric",true);
    auto error = new QLabel(this); error->setObjectName("saleError"); error->setWordWrap(true); layout->addWidget(error);
    error->setProperty("error",true);
    auto complete = new QPushButton(tr("Завършване на продажбата"),this); complete->setObjectName("completeSale");
    complete->setProperty("primary",true);
    layout->addWidget(complete);
    connect(customer,&QLineEdit::textChanged,this,[this](const QString &value) { m_sale.setCustomerName(value); });
    connect(vip,&QCheckBox::toggled,this,[this](bool value) { m_sale.setIsVip(value); refresh(); });
    connect(discount,&QComboBox::currentIndexChanged,this,[this,seasonal](int index) {
        seasonal->setVisible(index==3); changeStrategy();
    });
    connect(percent,&QSpinBox::valueChanged,this,[this] { changeStrategy(); });
    for (auto date : {from,to}) connect(date,&QDateEdit::dateChanged,this,[this] { changeStrategy(); });
    connect(add,&QPushButton::clicked,this,[this,product,quantity,error] {
        if (product->currentIndex()<0 || product->currentText()!=product->itemText(product->currentIndex())) {
            error->setText(tr("Изберете продукт от списъка с предложения.")); return;
        }
        auto items = m_sale.items();
        bool found = false;
        for (SaleItem &item : items) {
            if (item.productId() != product->currentData().toInt()) continue;
            if (item.quantity() > std::numeric_limits<int>::max() - quantity->value()) {
                error->setText(tr("Количеството е твърде голямо.")); return;
            }
            item.setQuantity(item.quantity()+quantity->value()); found = true; break;
        }
        if (!found) items.emplace_back(product->currentData().toInt(),quantity->value());
        m_sale.setItems(items); refresh();
    });
    connect(remove,&QPushButton::clicked,this,[this,cart] {
        auto items = m_sale.items(); const int row = cart->currentRow();
        if (row>=0 && row<static_cast<int>(items.size())) items.erase(items.begin()+row);
        m_sale.setItems(items); refresh();
    });
    connect(complete,&QPushButton::clicked,this,[this,invoice,error] {
        if (!m_service) return;
        const bool editing=m_sale.id()!=0;
        QString failure = editing ? m_service->updateSale(m_sale) : m_service->registerSale(m_sale);
        if (!failure.isEmpty()) { refresh(); error->setText(failure); return; }
        const int saleId = m_sale.id();
        const QString html = m_service->receipt(saleId,invoice->isChecked(),failure);
        setSale(Sale()); reloadProducts();
        if (!failure.isEmpty()) { error->setText(failure); return; }
        ReceiptDialog receipt(html,this); receipt.exec();
        if (editing) accept();
    });
    reloadProducts(); changeStrategy();
}
void SaleDialog::setService(WarehouseService *value)
{
    m_service = value; setSale(Sale()); reloadProducts(); changeStrategy();
}
void SaleDialog::setSale(Sale value)
{
    m_sale = value;
    auto discount=findChild<QComboBox*>("discount");
    if (value.id()) {
        const int choice=value.discountName()==VipDiscount().name() ? 2 :
            value.discountName()==QuantityDiscount().name() ? 1 :
            value.discountName()==SeasonalDiscount().name() ? 3 : 0;
        discount->setCurrentIndex(choice);
    }
    findChild<QLineEdit*>("customerName")->setText(value.customerName());
    findChild<QCheckBox*>("vip")->setChecked(value.isVip()); refresh();
    findChild<QPushButton*>("completeSale")->setText(value.id() ? tr("Запази корекцията") : tr("Завършване на продажбата"));
    if (value.id()) setWindowTitle(tr("Корекция на продажба №%1 — текущи цени и избрана отстъпка").arg(value.id()));
    findChild<QLabel*>("pageHeading")->setText(value.id() ? tr("Корекция на продажба №%1").arg(value.id()) : tr("Нова продажба"));
    findChild<QLabel*>("pageSubtitle")->setText(value.id()
        ? tr("Цените се преизчисляват по текущия каталог. Проверете отстъпката и общата сума преди запис.")
        : tr("Добавете продукти, изберете отстъпка и проверете сумата преди запис."));
}
void SaleDialog::reloadProducts()
{
    auto combo = findChild<QComboBox*>("saleProduct");
    const int selected = combo->currentData().toInt();
    combo->clear();
    if (!m_service) return;
    QString error;
    for (const Product &product : m_service->products(error))
        combo->addItem(tr("%1 [%2] — наличност %3").arg(product.name(),product.sku()).arg(product.quantity()),product.id());
    const int index = combo->findData(selected);
    if (index>=0) combo->setCurrentIndex(index);
    changeStrategy();
    if (!error.isEmpty()) findChild<QLabel*>("saleError")->setText(error);
}
void SaleDialog::changeStrategy()
{
    if (!m_service) { refresh(); return; }
    const int choice = findChild<QComboBox*>("discount")->currentIndex();
    switch (choice) {
    case 1: m_service->setDiscountStrategy(std::make_unique<QuantityDiscount>()); break;
    case 2: m_service->setDiscountStrategy(std::make_unique<VipDiscount>()); break;
    case 3:
        m_service->setDiscountStrategy(std::make_unique<SeasonalDiscount>(
            findChild<QSpinBox*>("seasonPercent")->value(),findChild<QDateEdit*>("seasonFrom")->date(),
            findChild<QDateEdit*>("seasonTo")->date()));
        break;
    default: m_service->setDiscountStrategy(std::make_unique<NoDiscount>()); break;
    }
    refresh();
}
void SaleDialog::refresh()
{
    auto table = findChild<QTableWidget*>("cartTable");
    QString error = m_service ? m_service->previewSale(m_sale) : tr("Влезте в системата.");
    if (findChild<QComboBox*>("discount")->currentIndex()==3
        && findChild<QDateEdit*>("seasonFrom")->date()>findChild<QDateEdit*>("seasonTo")->date())
        error = tr("Началната дата на отстъпката трябва да е преди крайната.");
    if (m_sale.id() && m_sale.discountName()==SeasonalDiscount().name()
        && findChild<QComboBox*>("discount")->currentIndex()==3 && findChild<QSpinBox*>("seasonPercent")->value()==0)
        error=tr("При корекция въведете сезонния процент и периода отново или изберете друга отстъпка.");
    table->setRowCount(static_cast<int>(m_sale.items().size()));
    int row = 0;
    for (const SaleItem &item : m_sale.items()) {
        const QStringList values = {item.productName().isEmpty() ? tr("Продукт №%1").arg(item.productId()) : item.productName(),
                                    QString::number(item.quantity()),formatCents(item.unitPrice()),
                                    error.isEmpty() ? formatCents(item.lineTotal()) : tr("—")};
        for (int column=0; column<values.size(); ++column)
            table->setItem(row,column,new QTableWidgetItem(values.at(column)));
        ++row;
    }
    findChild<QLabel*>("saleTotal")->setText(tr("Общо: %1").arg(error.isEmpty() ? formatCents(m_sale.total()) : tr("—")));
    findChild<QLabel*>("saleError")->setText(error);
    const bool allowed = m_service && m_service->hasPermission(Permission::RegisterSale);
    findChild<QPushButton*>("completeSale")->setEnabled(allowed && error.isEmpty());
    findChild<QPushButton*>("addToCart")->setEnabled(allowed);
}
