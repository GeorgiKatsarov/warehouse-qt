#include "MainWindow.h"
#include "WarehouseService.h"
#include "ProductDialog.h"
#include "ProductTableModel.h"
#include <QTabWidget>
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : MainWindow(nullptr,parent) {}
MainWindow::MainWindow(WarehouseService *service, QWidget *parent) : QMainWindow(parent),m_service(service)
{
    resize(1050,650); setWindowTitle(tr("Складова система")); build();
}
void MainWindow::setService(WarehouseService *value) { m_service = value; build(); }
void MainWindow::build()
{
    delete takeCentralWidget();
    if (!m_service || !m_service->currentUser()) return;
    auto tabs = new QTabWidget(this); tabs->setObjectName("mainTabs");
    tabs->addTab(productsTab(),tr("Продукти")); setCentralWidget(tabs);
    statusBar()->showMessage(tr("Потребител: %1").arg(m_service->currentUser()->fullName()));
}
QWidget *MainWindow::productsTab()
{
    auto page = new QWidget(this);
    auto layout = new QVBoxLayout(page);
    auto table = new QTableView(page); table->setObjectName("productsTable");
    auto model = new ProductTableModel(table); table->setModel(model);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);
    auto errorLabel = new QLabel(page); errorLabel->setWordWrap(true); layout->addWidget(errorLabel);
    auto actions = new QHBoxLayout;
    auto add = new QPushButton(tr("Добавяне"),page); add->setObjectName("addProduct");
    auto edit = new QPushButton(tr("Редактиране"),page); edit->setObjectName("editProduct");
    auto remove = new QPushButton(tr("Изтриване"),page); remove->setObjectName("deleteProduct");
    auto refresh = new QPushButton(tr("Обновяване"),page);
    for (auto button : {add,edit,remove,refresh}) actions->addWidget(button);
    actions->addStretch(); layout->addLayout(actions);
    const bool allowed = m_service->hasPermission(Permission::ManageProducts);
    for (auto button : {add,edit,remove}) button->setEnabled(allowed);
    auto reload = [this,model,errorLabel] {
        QString error; model->setProducts(m_service->products(error)); errorLabel->setText(error);
    };
    auto selected = [table,model](Product &product) {
        const int row = table->currentIndex().row();
        const auto rows = model->products();
        if (row < 0 || row >= static_cast<int>(rows.size())) return false;
        product = rows.at(row); return true;
    };
    connect(refresh,&QPushButton::clicked,page,reload);
    connect(add,&QPushButton::clicked,page,[this,reload,errorLabel] {
        ProductDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) return;
        Product product = dialog.product();
        const QString error = m_service->saveProduct(product);
        reload(); errorLabel->setText(error);
    });
    connect(edit,&QPushButton::clicked,page,[this,selected,reload,errorLabel] {
        Product product;
        if (!selected(product)) { errorLabel->setText(tr("Изберете продукт.")); return; }
        ProductDialog dialog(product,this);
        if (dialog.exec() != QDialog::Accepted) return;
        product = dialog.product();
        const QString error = m_service->saveProduct(product);
        reload(); errorLabel->setText(error);
    });
    connect(remove,&QPushButton::clicked,page,[this,selected,reload,errorLabel] {
        Product product;
        if (!selected(product)) { errorLabel->setText(tr("Изберете продукт.")); return; }
        if (QMessageBox::question(this,tr("Изтриване"),tr("Да се изтрие ли %1?").arg(product.name()),
                                  QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        const QString error = m_service->deleteProduct(product.id());
        reload(); errorLabel->setText(error);
    });
    reload(); return page;
}
