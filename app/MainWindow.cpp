#include "MainWindow.h"
#include "WarehouseService.h"
#include "ProductDialog.h"
#include "ProductTableModel.h"
#include "SaleDialog.h"
#include "Database.h"
#include <QTableWidget>
#include <QDateEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
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
    tabs->addTab(productsTab(),tr("Продукти"));
    auto sales = new SaleDialog(m_service,tabs); sales->setWindowFlags(Qt::Widget);
    const int saleIndex = tabs->addTab(sales,tr("Продажби"));
    tabs->setTabEnabled(saleIndex,m_service->hasPermission(Permission::RegisterSale));
    const int reportIndex = tabs->addTab(reportsTab(),tr("Отчети"));
    tabs->setTabEnabled(reportIndex,m_service->hasPermission(Permission::ViewReports));
    if (m_service->hasPermission(Permission::ManageUsers)) tabs->addTab(usersTab(),tr("Потребители"));
    connect(tabs,&QTabWidget::currentChanged,this,[sales,saleIndex](int index) {
        if (index==saleIndex) sales->reloadProducts();
    });
    setCentralWidget(tabs);
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

QWidget *MainWindow::reportsTab()
{
    auto page = new QWidget(this); auto layout = new QVBoxLayout(page);
    auto period = new QHBoxLayout;
    auto from = new QDateEdit(QDate::currentDate().addDays(-30),page); from->setObjectName("reportFrom");
    auto to = new QDateEdit(QDate::currentDate(),page); to->setObjectName("reportTo");
    for (auto date : {from,to}) { date->setCalendarPopup(true); date->setDisplayFormat("dd.MM.yyyy"); }
    period->addWidget(new QLabel(tr("От:"),page)); period->addWidget(from);
    period->addWidget(new QLabel(tr("До:"),page)); period->addWidget(to);
    auto generate = new QPushButton(tr("Генериране"),page); generate->setObjectName("generateReport");
    generate->setEnabled(m_service->hasPermission(Permission::ViewReports)); period->addWidget(generate);
    layout->addLayout(period);
    auto table = new QTableWidget(0,7,page); table->setObjectName("reportsTable");
    table->setHorizontalHeaderLabels({tr("Номер"),tr("Създаден на"),tr("От"),tr("До"),tr("Приходи"),tr("Печалба"),tr("Продадени бройки")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); layout->addWidget(table);
    auto errorLabel = new QLabel(page); errorLabel->setObjectName("reportError");
    errorLabel->setWordWrap(true); layout->addWidget(errorLabel);
    auto reload = [this,table,errorLabel] {
        QString error; const auto reports = m_service->reports(error);
        table->setRowCount(static_cast<int>(reports.size()));
        int row = 0;
        for (const Report &report : reports) {
            const QStringList values = {QString::number(report.id()),report.createdAt().toString("dd.MM.yyyy HH:mm"),
                report.dateFrom().toString("dd.MM.yyyy"),report.dateTo().toString("dd.MM.yyyy"),
                formatCents(report.revenue()),formatCents(report.profit()),QString::number(report.unitsSold())};
            for (int col=0; col<values.size(); ++col) table->setItem(row,col,new QTableWidgetItem(values.at(col)));
            ++row;
        }
        errorLabel->setText(error);
    };
    connect(generate,&QPushButton::clicked,page,[this,from,to,reload,errorLabel] {
        Report report(from->date(),to->date());
        const QString error = m_service->generateReport(report); reload(); errorLabel->setText(error);
    });
    reload(); return page;
}

QWidget *MainWindow::usersTab()
{
    auto page = new QWidget(this); auto layout = new QVBoxLayout(page);
    auto table = new QTableWidget(0,3,page); table->setObjectName("usersTable");
    table->setHorizontalHeaderLabels({tr("Потребителско име"),tr("Име"),tr("Роля")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); layout->addWidget(table);
    auto form = new QFormLayout;
    auto username = new QLineEdit(page); username->setObjectName("userUsername");
    auto fullName = new QLineEdit(page); fullName->setObjectName("userFullName");
    auto password = new QLineEdit(page); password->setObjectName("userPassword"); password->setEchoMode(QLineEdit::Password);
    auto role = new QComboBox(page); role->setObjectName("userRole");
    role->addItem(tr("Администратор"),"admin"); role->addItem(tr("Управител на склада"),"manager");
    role->addItem(tr("Касиер"),"cashier");
    form->addRow(tr("Потребителско име:"),username); form->addRow(tr("Име:"),fullName);
    form->addRow(tr("Парола (празна = без промяна):"),password); form->addRow(tr("Роля:"),role);
    layout->addLayout(form);
    auto actions = new QHBoxLayout;
    auto create = new QPushButton(tr("Нов потребител"),page); create->setObjectName("newUser");
    auto save = new QPushButton(tr("Запис"),page); save->setObjectName("saveUser");
    auto remove = new QPushButton(tr("Изтриване"),page); remove->setObjectName("deleteUser");
    for (auto button : {create,save,remove})
        actions->addWidget(button);
    layout->addLayout(actions);
    auto errorLabel = new QLabel(page); errorLabel->setObjectName("userError"); errorLabel->setWordWrap(true);
    layout->addWidget(errorLabel);
    auto roleLabel = [this](const QString &value) {
        if (value=="admin") return tr("Администратор");
        if (value=="manager") return tr("Управител на склада");
        return tr("Касиер");
    };
    auto reload = [this,table,errorLabel,roleLabel] {
        QString error; const auto users = m_service->users(error);
        table->setRowCount(static_cast<int>(users.size())); int row = 0;
        for (const auto &user : users) {
            auto item = new QTableWidgetItem(user->username()); item->setData(Qt::UserRole,user->id());
            table->setItem(row,0,item); table->setItem(row,1,new QTableWidgetItem(user->fullName()));
            table->setItem(row,2,new QTableWidgetItem(roleLabel(user->roleName()))); ++row;
        }
        table->clearSelection(); table->setCurrentCell(-1,-1); errorLabel->setText(error);
    };
    auto clear = [table,username,fullName,password,role] {
        table->clearSelection(); table->setCurrentCell(-1,-1);
        username->clear(); fullName->clear(); password->clear(); role->setCurrentIndex(2);
    };
    connect(create,&QPushButton::clicked,page,clear);
    connect(table,&QTableWidget::cellClicked,page,[this,table,username,fullName,password,role](int row,int) {
        if (row<0 || !table->item(row,0)) return;
        const int id = table->item(row,0)->data(Qt::UserRole).toInt();
        QString error;
        for (const auto &user : m_service->users(error)) {
            if (user->id()!=id) continue;
            username->setText(user->username()); fullName->setText(user->fullName()); password->clear();
            role->setCurrentIndex(role->findData(user->roleName())); break;
        }
    });
    connect(save,&QPushButton::clicked,page,[this,table,username,fullName,password,role,reload,clear,errorLabel] {
        const int row = table->currentRow();
        const int id = row>=0 && table->item(row,0) ? table->item(row,0)->data(Qt::UserRole).toInt() : 0;
        QString hash = password->text().isEmpty() ? QString() : hashPassword(password->text());
        if (id && hash.isEmpty()) {
            QString error;
            for (const auto &user : m_service->users(error))
                if (user->id()==id) { hash = user->passwordHash(); break; }
        }
        std::shared_ptr<User> user;
        const QString key = role->currentData().toString();
        if (key=="admin") user = std::make_shared<Admin>(id,username->text(),hash,fullName->text());
        else if (key=="manager") user = std::make_shared<WarehouseManager>(id,username->text(),hash,fullName->text());
        else user = std::make_shared<Cashier>(id,username->text(),hash,fullName->text());
        const QString error = m_service->saveUser(*user);
        if (error.isEmpty()) { reload(); clear(); }
        errorLabel->setText(error);
    });
    connect(remove,&QPushButton::clicked,page,[this,table,reload,clear,errorLabel] {
        const int row = table->currentRow();
        if (row<0 || !table->item(row,0)) { errorLabel->setText(tr("Изберете потребител.")); return; }
        if (QMessageBox::question(this,tr("Изтриване"),tr("Да се изтрие ли избраният потребител?"),
                                  QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        const QString error = m_service->deleteUser(table->item(row,0)->data(Qt::UserRole).toInt());
        if (error.isEmpty()) { reload(); clear(); }
        errorLabel->setText(error);
    });
    reload(); clear(); return page;
}
