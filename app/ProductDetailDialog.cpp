#include "ProductDetailDialog.h"
#include "WarehouseService.h"
#include "ProductTableModel.h"
#include "ReceiptDialog.h"
#include "UiHelpers.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QPushButton>

ProductDetailDialog::ProductDetailDialog(QWidget *parent) : ProductDetailDialog(nullptr,Product(),parent) {}
ProductDetailDialog::ProductDetailDialog(WarehouseService *service, Product product, QWidget *parent)
    : QDialog(parent),m_service(service),m_product(product)
{
    setObjectName("productDetailDialog"); resize(1200,720);
    auto layout = new QVBoxLayout(this);
    auto title = new QLabel(this); title->setObjectName("detailTitle"); title->setProperty("heading",true); layout->addWidget(title);
    auto summary = new QLabel(this); summary->setObjectName("detailSummary"); summary->setWordWrap(true); layout->addWidget(summary);
    auto help = new QLabel(tr("История на движенията и цените. Записите се пазят за проследимост и не могат да се редактират."),this);
    help->setProperty("subtle",true); help->setWordWrap(true); layout->addWidget(help);
    auto table = new QTableWidget(0,9,this); table->setObjectName("productHistoryTable");
    table->setHorizontalHeaderLabels({tr("Дата"),tr("Действие"),tr("Преди"),tr("След"),tr("Промяна"),tr("Продажна цена"),tr("Доставна цена"),tr("Потребител"),tr("Продажба")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection); table->setAlternatingRowColors(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true); layout->addWidget(table);
    auto error = new QLabel(this); error->setObjectName("historyError"); error->setProperty("error",true); layout->addWidget(error);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close,this);
    buttons->button(QDialogButtonBox::Close)->setText(tr("Затваряне"));
    auto document = buttons->addButton(tr("Отвори свързаната продажба"),QDialogButtonBox::ActionRole);
    document->setObjectName("historyReceipt"); layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::accept);
    connect(document,&QPushButton::clicked,this,[this,table,error] {
        if (!m_service || table->currentRow()<0) { error->setText(tr("Изберете запис с номер на продажба.")); return; }
        const int saleId = table->item(table->currentRow(),8)->data(Qt::UserRole).toInt();
        if (!saleId) { error->setText(tr("Това движение не е свързано с продажба.")); return; }
        QString failure; const QString html = m_service->receipt(saleId,false,failure);
        if (!failure.isEmpty()) { error->setText(failure); return; }
        ReceiptDialog receipt(html,this); receipt.exec();
    });
    refresh();
}
void ProductDetailDialog::setService(WarehouseService *value) { m_service=value; refresh(); }
void ProductDetailDialog::setProduct(Product value) { m_product=value; refresh(); }
void ProductDetailDialog::refresh()
{
    setWindowTitle(tr("Продукт · %1").arg(m_product.name()));
    findChild<QLabel*>("detailTitle")->setText(m_product.name());
    findChild<QLabel*>("detailSummary")->setText(tr("Код: %1  ·  Наличност: %2 бр.  ·  Доставчик: %3\nПродажна цена: %4  ·  Доставна цена: %5")
        .arg(m_product.sku()).arg(m_product.quantity()).arg(m_product.supplier().isEmpty() ? tr("Не е посочен") : m_product.supplier())
        .arg(formatCents(m_product.salePrice()),formatCents(m_product.deliveryPrice())));
    auto table=findChild<QTableWidget*>("productHistoryTable"); table->setRowCount(0);
    if (!m_service) return;
    QString error; const auto history=m_service->productHistory(m_product.id(),error);
    table->setRowCount(static_cast<int>(history.size())); int row=0;
    for (const auto &entry : history) {
        const int delta=entry.quantityAfter()-entry.quantityBefore();
        const QStringList values={entry.createdAt().toString("dd.MM.yyyy HH:mm"),entry.action(),
            entry.quantityBefore()<0 ? tr("—") : QString::number(entry.quantityBefore()),
            entry.quantityAfter()<0 ? tr("—") : QString::number(entry.quantityAfter()),
            entry.quantityBefore()<0 ? tr("—") : (delta>0 ? "+" : "")+QString::number(delta),
            formatCents(entry.priceBefore())+" → "+formatCents(entry.priceAfter()),
            formatCents(entry.deliveryBefore())+" → "+formatCents(entry.deliveryAfter()),
            entry.actorName(),entry.saleId() ? QString::number(entry.saleId()) : tr("—")};
        for (int col=0;col<values.size();++col) {
            auto item=new QTableWidgetItem(values.at(col)); item->setToolTip(entry.notes());
            if (col==8) item->setData(Qt::UserRole,entry.saleId());
            table->setItem(row,col,item);
        }
        ++row;
    }
    findChild<QLabel*>("historyError")->setText(error);
}
