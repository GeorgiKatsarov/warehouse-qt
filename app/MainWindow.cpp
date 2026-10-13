#include "MainWindow.h"
#include "WarehouseService.h"
#include "ProductDialog.h"
#include "ProductTableModel.h"
#include "ProductDetailDialog.h"
#include "ReportDialog.h"
#include "SaleDialog.h"
#include "ReceiptDialog.h"
#include "UiHelpers.h"
#include "Database.h"
#include <QTableWidget>
#include <QDateEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QTabWidget>
#include <QTabBar>
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QStatusBar>
#include <QListWidget>
#include <QFrame>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QCheckBox>
#include <QShortcut>
#include <QStyle>
#include <QSignalBlocker>

MainWindow::MainWindow(QWidget *parent) : MainWindow(nullptr,parent) {}
MainWindow::MainWindow(WarehouseService *service, QWidget *parent) : QMainWindow(parent),m_service(service)
{
    resize(1360,850); setMinimumSize(1050,700); setWindowTitle(tr("Warehouse · Складова система")); build();
    auto refresh=new QShortcut(QKeySequence("Ctrl+R"),this);
    connect(refresh,&QShortcut::activated,this,[this] {
        auto tabs=findChild<QTabWidget*>("mainTabs"); if (!tabs) return;
        if (auto button=tabs->currentWidget()->findChild<QPushButton*>("pageRefresh")) button->click();
    });
}
void MainWindow::setService(WarehouseService *value) { m_service=value; build(); }
void MainWindow::build()
{
    delete takeCentralWidget();
    if (!m_service || !m_service->currentUser()) return;
    auto root=new QWidget(this); auto layout=new QHBoxLayout(root); layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);
    auto sidebar=new QFrame(root); sidebar->setObjectName("sidebar"); sidebar->setFixedWidth(225);
    auto side=new QVBoxLayout(sidebar); side->setContentsMargins(18,26,18,20);
    auto brand=new QLabel("WAREHOUSE",sidebar); brand->setObjectName("brand"); side->addWidget(brand);
    side->addWidget(new QLabel(tr("Управление на склада"),sidebar)); side->addSpacing(26);
    auto navigation=new QListWidget(sidebar); navigation->setObjectName("navigation"); side->addWidget(navigation,1);
    navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto identity=new QLabel(m_service->currentUser()->fullName(),sidebar); identity->setWordWrap(true); side->addWidget(identity);
    const QString role=m_service->currentUser()->roleName();
    side->addWidget(new QLabel(role=="admin" ? tr("Администратор") : role=="manager" ? tr("Управител на склада") : tr("Касиер"),sidebar));
    auto logout=new QPushButton(tr("Смяна на потребител"),sidebar); side->addWidget(logout);
    connect(logout,&QPushButton::clicked,this,[this] {
        if (confirmAction(this,tr("Изход"),tr("Да се затвори ли текущата сесия? Незавършената количка няма да бъде запазена."))) {
            m_service->logout(); close();
        }
    });
    layout->addWidget(sidebar);
    auto tabs=new QTabWidget(root); tabs->setObjectName("mainTabs"); tabs->setContentsMargins(16,12,16,12);
    tabs->addTab(productsTab(),tr("Продукти"));
    auto sales=new SaleDialog(m_service,tabs); sales->setWindowFlags(Qt::Widget); tabs->addTab(sales,tr("Нова продажба"));
    tabs->setTabEnabled(1,m_service->hasPermission(Permission::RegisterSale));
    tabs->addTab(reportsTab(),tr("Отчети")); tabs->setTabEnabled(2,m_service->hasPermission(Permission::ViewReports));
    if (m_service->hasPermission(Permission::ManageUsers)) tabs->addTab(usersTab(),tr("Потребители"));
    const int dashboard=tabs->addTab(dashboardTab(),tr("Преглед"));
    const int history=tabs->addTab(salesHistoryTab(),tr("История на продажбите"));
    tabs->setTabEnabled(history,m_service->hasPermission(Permission::RegisterSale)); tabs->tabBar()->hide(); layout->addWidget(tabs,1);
    auto addNavigation=[navigation,tabs](QString label,int page,int icon) {
        auto item=new QListWidgetItem(navigationIcon(icon),label,navigation);
        item->setData(Qt::UserRole,page);
        if (!tabs->isTabEnabled(page)) item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    };
    addNavigation(tr("Преглед"),dashboard,0);
    addNavigation(tr("Продукти"),0,1);
    addNavigation(tr("Нова продажба"),1,2);
    addNavigation(tr("Продажби · история"),history,3);
    addNavigation(tr("Отчети"),2,4);
    if (m_service->hasPermission(Permission::ManageUsers)) addNavigation(tr("Потребители"),3,5);
    connect(navigation,&QListWidget::currentRowChanged,this,[navigation,tabs](int row) {
        if (row>=0) tabs->setCurrentIndex(navigation->item(row)->data(Qt::UserRole).toInt());
    });
    connect(tabs,&QTabWidget::currentChanged,this,[tabs,sales,navigation](int index) {
        if (index==1) sales->reloadProducts();
        if (auto button=tabs->widget(index)->findChild<QPushButton*>("pageRefresh")) button->click();
        const QSignalBlocker blocker(navigation);
        for (int row=0;row<navigation->count();++row)
            if (navigation->item(row)->data(Qt::UserRole).toInt()==index) navigation->setCurrentRow(row);
    });
    setCentralWidget(root); navigation->setCurrentRow(0);
    statusBar()->showMessage(tr("EUR · Данните се записват автоматично · Ctrl+R за обновяване"));
}
QWidget *MainWindow::productsTab()
{
    auto page=new QWidget(this); auto layout=new QVBoxLayout(page); layout->setContentsMargins(20,18,20,18);
    addPageHeading(layout,tr("Продукти"),tr("Търсене, наличности и история. Отворете продукт с двоен клик или десен бутон."));
    auto tools=new QHBoxLayout;
    auto search=new QLineEdit(page); search->setObjectName("productSearch"); search->setPlaceholderText(tr("Търсене по име, код или доставчик…")); search->setClearButtonEnabled(true);
    auto low=new QCheckBox(tr("Само ниски наличности"),page); low->setObjectName("lowStockFilter"); tools->addWidget(search,1); tools->addWidget(low);
    auto add=new QPushButton(tr("+ Добавяне"),page); add->setObjectName("addProduct"); add->setProperty("primary",true); tools->addWidget(add); layout->addLayout(tools);
    auto table=new QTableView(page); table->setObjectName("productsTable"); auto model=new ProductTableModel(table); table->setModel(model);
    table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true); table->setSortingEnabled(true); table->verticalHeader()->setDefaultSectionSize(42); table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); table->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(table,1);
    auto actions=new QHBoxLayout;
    auto details=new QPushButton(tr("Детайли и история"),page); details->setObjectName("productDetails");
    auto edit=new QPushButton(tr("Редактиране"),page); edit->setObjectName("editProduct");
    auto remove=new QPushButton(tr("Изтриване"),page); remove->setObjectName("deleteProduct"); remove->setProperty("danger",true);
    auto refresh=new QPushButton(tr("Обновяване"),page); refresh->setObjectName("pageRefresh");
    for (auto button : {details,edit,remove}) actions->addWidget(button);
    actions->addStretch(); actions->addWidget(refresh); layout->addLayout(actions);
    auto count=new QLabel(page); count->setProperty("subtle",true); layout->addWidget(count);
    auto errorLabel=new QLabel(page); errorLabel->setObjectName("productsError"); errorLabel->setProperty("error",true); errorLabel->setWordWrap(true); layout->addWidget(errorLabel);
    const bool allowed=m_service->hasPermission(Permission::ManageProducts); add->setEnabled(allowed);
    auto selection=[this,table,details,edit,remove] {
        const bool selected=table->currentIndex().isValid(); details->setEnabled(selected);
        const bool canEdit=selected && m_service->hasPermission(Permission::ManageProducts); edit->setEnabled(canEdit); remove->setEnabled(canEdit);
    };
    auto reload=[this,model,table,search,low,count,errorLabel,selection] {
        QString error; const auto catalog=m_service->products(error); std::vector<Product> filtered;
        const QString query=search->text().trimmed();
        for (const Product &product : catalog) {
            if (low->isChecked() && product.quantity()>10) continue;
            if (!query.isEmpty() && !(product.name()+" "+product.sku()+" "+product.supplier()).contains(query,Qt::CaseInsensitive)) continue;
            filtered.push_back(product);
        }
        model->setProducts(filtered); model->sort(table->horizontalHeader()->sortIndicatorSection(),table->horizontalHeader()->sortIndicatorOrder());
        count->setText(tr("%1 показани от %2 продукта · ниска наличност: до 10 броя").arg(filtered.size()).arg(catalog.size()));
        errorLabel->setText(error); selection();
    };
    auto selected=[table,model](Product &product) {
        const int row=table->currentIndex().row(); const auto rows=model->products();
        if (row<0 || row>=static_cast<int>(rows.size())) return false;
        product=rows.at(row); return true;
    };
    auto open=[this,selected] {
        Product product; if (!selected(product)) return;
        ProductDetailDialog dialog(m_service,product,this); dialog.exec();
    };
    connect(table->selectionModel(),&QItemSelectionModel::selectionChanged,page,selection);
    connect(refresh,&QPushButton::clicked,page,reload); connect(search,&QLineEdit::textChanged,page,reload); connect(low,&QCheckBox::toggled,page,reload);
    connect(details,&QPushButton::clicked,page,open); connect(table,&QTableView::doubleClicked,page,open);
    connect(add,&QPushButton::clicked,page,[this,reload,errorLabel] {
        ProductDialog dialog(this); if (dialog.exec()!=QDialog::Accepted) return;
        Product product=dialog.product(); const QString error=m_service->saveProduct(product); reload();
        errorLabel->setText(error);
        if (error.isEmpty()) statusBar()->showMessage(tr("Продуктът е добавен."),5000);
    });
    connect(edit,&QPushButton::clicked,page,[this,selected,reload,errorLabel] {
        Product product; if (!selected(product)) return;
        ProductDialog dialog(product,this); if (dialog.exec()!=QDialog::Accepted) return;
        product=dialog.product(); const QString error=m_service->saveProduct(product); reload(); errorLabel->setText(error);
    });
    connect(remove,&QPushButton::clicked,page,[this,selected,reload,errorLabel] {
        Product product; if (!selected(product)) return;
        if (!confirmAction(this,tr("Изтриване на продукт"),tr("Да се изтрие ли „%1“? Продукти с продажби остават защитени.").arg(product.name()))) return;
        const QString error=m_service->deleteProduct(product.id()); reload(); errorLabel->setText(error);
    });
    connect(table,&QTableView::customContextMenuRequested,page,[table,details,edit,remove,add](QPoint point) {
        const QModelIndex index=table->indexAt(point); if (index.isValid()) table->setCurrentIndex(index);
        QMenu menu(table); auto profile=menu.addAction(QObject::tr("Детайли и история"));
        auto change=menu.addAction(QObject::tr("Редактиране")); auto erase=menu.addAction(QObject::tr("Изтриване"));
        menu.addSeparator(); auto create=menu.addAction(QObject::tr("Нов продукт"));
        profile->setEnabled(index.isValid()); change->setEnabled(index.isValid() && edit->isEnabled());
        erase->setEnabled(index.isValid() && remove->isEnabled()); create->setEnabled(add->isEnabled());
        auto chosen=menu.exec(table->viewport()->mapToGlobal(point));
        if (chosen==profile) details->click(); else if (chosen==change) edit->click(); else if (chosen==erase) remove->click(); else if (chosen==create) add->click();
    });
    reload(); return page;
}
QWidget *MainWindow::reportsTab()
{
    auto page=new QWidget(this); auto layout=new QVBoxLayout(page); layout->setContentsMargins(20,18,20,18);
    addPageHeading(layout,tr("Отчети и анализи"),tr("Пет вида отчети със запазени резултати. Отваряйте ги отново и експортирайте в PDF или XLS."));
    auto form=new QHBoxLayout;
    auto type=new QComboBox(page); type->setObjectName("reportType");
    for (int i=0;i<5;++i) type->addItem(reportTypeName(static_cast<ReportType>(i)),i);
    auto from=new QDateEdit(QDate::currentDate().addDays(-30),page); from->setObjectName("reportFrom");
    auto to=new QDateEdit(QDate::currentDate(),page); to->setObjectName("reportTo");
    for (auto date : {from,to}) { date->setCalendarPopup(true); date->setDisplayFormat("dd.MM.yyyy"); }
    form->addWidget(type,2); form->addWidget(new QLabel(tr("От"),page)); form->addWidget(from);
    form->addWidget(new QLabel(tr("До"),page)); form->addWidget(to);
    auto generate=new QPushButton(tr("Създай отчет"),page); generate->setObjectName("generateReport"); generate->setProperty("primary",true);
    generate->setEnabled(m_service->hasPermission(Permission::ViewReports)); form->addWidget(generate); layout->addLayout(form);
    auto hint=new QLabel(tr("Отчетите за наличности са снимка към момента на създаване. Останалите използват избрания период."),page);
    hint->setProperty("subtle",true); hint->setWordWrap(true); layout->addWidget(hint);
    auto search=new QLineEdit(page); search->setObjectName("reportSearch"); search->setPlaceholderText(tr("Търсене в архива по име или тип…")); search->setClearButtonEnabled(true); layout->addWidget(search);
    auto table=new QTableWidget(0,6,page); table->setObjectName("reportsTable");
    table->setHorizontalHeaderLabels({tr("№"),tr("Наименование"),tr("Тип"),tr("Период"),tr("Създаден"),tr("Редове")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true); table->verticalHeader()->hide(); table->verticalHeader()->setDefaultSectionSize(44);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents); table->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch); layout->addWidget(table,1);
    auto actions=new QHBoxLayout;
    auto open=new QPushButton(tr("Отвори"),page); open->setObjectName("openReport");
    auto rename=new QPushButton(tr("Преименувай"),page); rename->setObjectName("renameReport");
    auto remove=new QPushButton(tr("Изтрий"),page); remove->setObjectName("deleteReport"); remove->setProperty("danger",true);
    auto pdf=new QPushButton(tr("PDF"),page); pdf->setObjectName("archivePdf");
    auto xls=new QPushButton(tr("XLS"),page); xls->setObjectName("archiveXls");
    auto refresh=new QPushButton(tr("Обновяване"),page); refresh->setObjectName("pageRefresh");
    for (auto button : {open,rename,remove,pdf,xls}) actions->addWidget(button);
    actions->addStretch(); actions->addWidget(refresh); layout->addLayout(actions);
    auto errorLabel=new QLabel(page); errorLabel->setObjectName("reportError"); errorLabel->setProperty("error",true); errorLabel->setWordWrap(true); layout->addWidget(errorLabel);
    auto selection=[table,open,rename,remove,pdf,xls] { for (auto button : {open,rename,remove,pdf,xls}) button->setEnabled(table->currentRow()>=0); };
    auto reload=[this,table,search,errorLabel,selection] {
        QString error; const auto reports=m_service->reports(error); table->setRowCount(0);
        for (const Report &report : reports) {
            if (!(report.title()+" "+reportTypeName(report.type())).contains(search->text(),Qt::CaseInsensitive)) continue;
            const int row=table->rowCount(); table->insertRow(row);
            const QStringList values={QString::number(report.id()),report.title(),reportTypeName(report.type()),
                report.dateFrom().toString("dd.MM.yyyy")+" — "+report.dateTo().toString("dd.MM.yyyy"),
                report.createdAt().toString("dd.MM.yyyy HH:mm"),QString::number(report.rows().size())};
            for (int col=0;col<values.size();++col) table->setItem(row,col,new QTableWidgetItem(values.at(col)));
        }
        table->setCurrentCell(-1,-1); errorLabel->setText(error); selection();
    };
    auto selected=[this,table,errorLabel](Report &report) {
        if (table->currentRow()<0) return false;
        QString error; const bool found=m_service->findReport(table->item(table->currentRow(),0)->text().toInt(),report,error);
        errorLabel->setText(error); return found;
    };
    auto view=[this,selected] { Report report; if (selected(report)) { ReportDialog dialog(report,this); dialog.exec(); } };
    connect(table,&QTableWidget::itemSelectionChanged,page,selection);
    connect(refresh,&QPushButton::clicked,page,reload); connect(search,&QLineEdit::textChanged,page,reload);
    connect(open,&QPushButton::clicked,page,view); connect(table,&QTableWidget::cellDoubleClicked,page,view);
    connect(generate,&QPushButton::clicked,page,[this,type,from,to,reload,errorLabel] {
        Report report(from->date(),to->date()); report.setType(static_cast<ReportType>(type->currentData().toInt()));
        const QString error=m_service->generateReport(report); reload(); errorLabel->setText(error);
    });
    connect(rename,&QPushButton::clicked,page,[this,selected,reload,errorLabel] {
        Report report; if (!selected(report)) return; bool ok=false;
        const QString title=QInputDialog::getText(this,tr("Преименуване"),tr("Наименование (до 160 знака):"),QLineEdit::Normal,report.title(),&ok);
        if (!ok) return;
        const QString error=m_service->renameReport(report.id(),title); reload(); errorLabel->setText(error);
    });
    connect(remove,&QPushButton::clicked,page,[this,selected,reload,errorLabel] {
        Report report; if (!selected(report)) return;
        if (!confirmAction(this,tr("Изтриване на отчет"),tr("Да се изтрие ли запазеният отчет „%1“?").arg(report.title()))) return;
        const QString error=m_service->deleteReport(report.id()); reload(); errorLabel->setText(error);
    });
    auto exportFile=[this,selected,errorLabel](bool excel) {
        Report report; if (!selected(report)) return;
        const QString suffix=excel ? ".xls" : ".pdf";
        QString path=QFileDialog::getSaveFileName(this,tr("Експорт на отчет"),QString("report-%1").arg(report.id())+suffix,excel ? tr("Excel 97–2003 (*.xls)") : tr("PDF (*.pdf)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(suffix,Qt::CaseInsensitive)) path+=suffix;
        ReportDialog dialog(report,this); const QString error=excel ? dialog.exportXls(path) : dialog.exportPdf(path);
        errorLabel->setText(error);
        if (error.isEmpty()) statusBar()->showMessage(tr("Отчетът е експортиран успешно."),5000);
    };
    connect(pdf,&QPushButton::clicked,page,[exportFile] { exportFile(false); }); connect(xls,&QPushButton::clicked,page,[exportFile] { exportFile(true); });
    reload(); return page;
}
QWidget *MainWindow::usersTab()
{
    auto page = new QWidget(this); auto layout = new QVBoxLayout(page);
    layout->setContentsMargins(20,18,20,18);
    addPageHeading(layout,tr("Потребители"),tr("Управлявайте профилите и ролите. Защитени са активният профил и последният администратор."));
    auto table = new QTableWidget(0,3,page);
    table->setAlternatingRowColors(true); table->verticalHeader()->hide(); table->verticalHeader()->setDefaultSectionSize(42); table->setObjectName("usersTable");
    table->setHorizontalHeaderLabels({tr("Потребителско име"),tr("Име"),tr("Роля")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); layout->addWidget(table);
    auto form = new QFormLayout;
    auto username = new QLineEdit(page); username->setMaxLength(64); username->setPlaceholderText(tr("3–64 латински букви, цифри, . _ -")); username->setObjectName("userUsername");
    auto fullName = new QLineEdit(page); fullName->setMaxLength(160); fullName->setObjectName("userFullName");
    auto password = new QLineEdit(page); password->setMaxLength(128); password->setPlaceholderText(tr("Нова парола: поне 8 знака")); password->setObjectName("userPassword"); password->setEchoMode(QLineEdit::Password);
    auto role = new QComboBox(page); role->setObjectName("userRole");
    role->addItem(tr("Администратор"),"admin"); role->addItem(tr("Управител на склада"),"manager");
    role->addItem(tr("Касиер"),"cashier");
    form->addRow(tr("Потребителско име:"),username); form->addRow(tr("Име:"),fullName);
    form->addRow(tr("Парола (празна = без промяна):"),password); form->addRow(tr("Роля:"),role);
    layout->addLayout(form);
    auto actions = new QHBoxLayout;
    auto create = new QPushButton(tr("Нов потребител"),page); create->setObjectName("newUser");
    auto save = new QPushButton(tr("Запис"),page); save->setObjectName("saveUser"); save->setProperty("primary",true);
    auto remove = new QPushButton(tr("Изтриване"),page); remove->setObjectName("deleteUser"); remove->setProperty("danger",true);
    for (auto button : {create,save,remove})
        actions->addWidget(button);
    layout->addLayout(actions);
    auto errorLabel = new QLabel(page); errorLabel->setObjectName("userError"); errorLabel->setProperty("error",true); errorLabel->setWordWrap(true);
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
        if ((!id || !password->text().isEmpty()) && password->text().size()<8) {
            errorLabel->setText(tr("Новата парола трябва да е поне 8 знака.")); return;
        }
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
        const QString error = m_service->saveUser(*user,password->text());
        if (error.isEmpty()) { reload(); clear(); }
        errorLabel->setText(error);
    });
    connect(remove,&QPushButton::clicked,page,[this,table,reload,clear,errorLabel] {
        const int row = table->currentRow();
        if (row<0 || !table->item(row,0)) { errorLabel->setText(tr("Изберете потребител.")); return; }
        if (!confirmAction(this,tr("Изтриване на потребител"),tr("Да се изтрие ли избраният потребител? Профили с продажби остават защитени."))) return;
        const QString error = m_service->deleteUser(table->item(row,0)->data(Qt::UserRole).toInt());
        if (error.isEmpty()) { reload(); clear(); }
        errorLabel->setText(error);
    });
    reload(); clear(); return page;
}

QWidget *MainWindow::dashboardTab()
{
    auto page=new QWidget(this); page->setObjectName("dashboardPage");
    auto layout=new QVBoxLayout(page); layout->setContentsMargins(20,18,20,18);
    addPageHeading(layout,tr("Преглед на склада"),tr("Актуални наличности и продажби за днес. Всички суми са в евро."));
    auto cards=new QHBoxLayout;
    const QStringList names={tr("Продукти"),tr("Ниски наличности"),tr("Бройки в склада"),tr("Продажби днес"),tr("Приходи днес")};
    for (int i=0;i<names.size();++i) {
        auto card=new QFrame(page); card->setProperty("card",true); auto content=new QVBoxLayout(card);
        auto label=new QLabel(names.at(i),card); label->setProperty("subtle",true); content->addWidget(label);
        auto value=new QLabel("—",card); value->setObjectName(QString("metric%1").arg(i)); value->setProperty("metric",true); content->addWidget(value); cards->addWidget(card);
    }
    layout->addLayout(cards); layout->addSpacing(20);
    auto title=new QLabel(tr("Продукти, които изискват внимание"),page); title->setStyleSheet("font-size:15pt;font-weight:600"); layout->addWidget(title);
    auto table=new QTableWidget(0,4,page); table->setObjectName("dashboardLowStock");
    table->setHorizontalHeaderLabels({tr("Код"),tr("Продукт"),tr("Наличност"),tr("Доставчик")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true); table->verticalHeader()->hide(); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); layout->addWidget(table,1);
    auto empty=new QLabel(tr("Всички продукти имат достатъчна наличност.\nНяма продукти с 10 или по-малко броя."),page);
    empty->setAlignment(Qt::AlignCenter); empty->setWordWrap(true);
    empty->setStyleSheet("background:white;border:1px solid #e2e8f0;border-radius:8px;color:#64748b;font-size:14pt;padding:32px");
    layout->addWidget(empty,1);
    auto help=new QLabel(tr("Двоен клик отваря историята на продукта. Ниска наличност означава 10 или по-малко броя."),page);
    help->setProperty("subtle",true); layout->addWidget(help);
    auto errorLabel=new QLabel(page); errorLabel->setProperty("error",true); layout->addWidget(errorLabel);
    auto refresh=new QPushButton(tr("Обновяване"),page); refresh->setObjectName("pageRefresh"); layout->addWidget(refresh,0,Qt::AlignRight);
    auto reload=[this,page,table,errorLabel,empty] {
        QString error; const QStringList metrics=m_service->overview(error);
        for (int i=0;i<5;++i) page->findChild<QLabel*>(QString("metric%1").arg(i))->setText(
            metrics.size()==5 ? (i==4 ? formatCents(metrics.at(i).toLongLong()) : metrics.at(i)) : tr("—"));
        table->setRowCount(0);
        for (const Product &product : m_service->products(error)) {
            if (product.quantity()>10) continue;
            const int row=table->rowCount(); table->insertRow(row);
            const QStringList values={product.sku(),product.name(),QString::number(product.quantity()),product.supplier()};
            for (int col=0;col<values.size();++col) {
                auto item=new QTableWidgetItem(values.at(col)); item->setData(Qt::UserRole,product.id()); table->setItem(row,col,item);
            }
        }
        table->setVisible(table->rowCount()>0); empty->setVisible(table->rowCount()==0);
        errorLabel->setText(error);
    };
    connect(refresh,&QPushButton::clicked,page,reload);
    connect(table,&QTableWidget::cellDoubleClicked,page,[this,table](int row,int) {
        QString error;
        for (const Product &product : m_service->products(error))
            if (product.id()==table->item(row,0)->data(Qt::UserRole).toInt()) {
                ProductDetailDialog dialog(m_service,product,this); dialog.exec(); break;
            }
    });
    reload(); return page;
}

QWidget *MainWindow::salesHistoryTab()
{
    auto page=new QWidget(this); page->setObjectName("salesHistoryPage");
    auto layout=new QVBoxLayout(page); layout->setContentsMargins(20,18,20,18);
    addPageHeading(layout,tr("История на продажбите"),tr("Отворете документ, коригирайте активна продажба или я отменете с автоматично възстановяване на наличностите."));
    auto tools=new QHBoxLayout;
    auto search=new QLineEdit(page); search->setObjectName("saleSearch"); search->setPlaceholderText(tr("Търсене по номер или клиент…")); search->setClearButtonEnabled(true);
    auto cancelled=new QCheckBox(tr("Покажи и отменените"),page); cancelled->setChecked(true); tools->addWidget(search,1); tools->addWidget(cancelled); layout->addLayout(tools);
    auto table=new QTableWidget(0,6,page); table->setObjectName("salesHistoryTable");
    table->setHorizontalHeaderLabels({tr("№"),tr("Дата"),tr("Клиент"),tr("Отстъпка"),tr("Общо (€)"),tr("Статус")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection); table->setAlternatingRowColors(true); table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(44); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); layout->addWidget(table,1);
    auto actions=new QHBoxLayout;
    auto document=new QPushButton(tr("Документ / PDF"),page); document->setObjectName("openSale");
    auto edit=new QPushButton(tr("Корекция"),page); edit->setObjectName("editSale");
    auto remove=new QPushButton(tr("Отмяна на продажба"),page); remove->setObjectName("cancelSale"); remove->setProperty("danger",true);
    auto refresh=new QPushButton(tr("Обновяване"),page); refresh->setObjectName("pageRefresh");
    for (auto button : {document,edit,remove}) actions->addWidget(button);
    actions->addStretch(); actions->addWidget(refresh); layout->addLayout(actions);
    auto errorLabel=new QLabel(page); errorLabel->setObjectName("saleHistoryError"); errorLabel->setProperty("error",true); errorLabel->setWordWrap(true); layout->addWidget(errorLabel);
    auto selection=[table,document,edit,remove] {
        const int row=table->currentRow(); const bool selected=row>=0;
        document->setEnabled(selected); const bool active=selected && !table->item(row,0)->data(Qt::UserRole).toBool();
        edit->setEnabled(active); remove->setEnabled(active);
    };
    auto reload=[this,table,search,cancelled,errorLabel,selection] {
        QString error; const auto sales=m_service->sales(error); table->setRowCount(0);
        for (const Sale &sale : sales) {
            if (sale.cancelled() && !cancelled->isChecked()) continue;
            if (!(QString::number(sale.id())+" "+sale.customerName()).contains(search->text(),Qt::CaseInsensitive)) continue;
            const int row=table->rowCount(); table->insertRow(row);
            const QStringList values={QString::number(sale.id()),sale.createdAt().toString("dd.MM.yyyy HH:mm"),
                sale.customerName(),sale.discountName(),formatCents(sale.total()),sale.cancelled() ? tr("Отменена") : tr("Активна")};
            for (int col=0;col<values.size();++col) {
                auto item=new QTableWidgetItem(values.at(col)); item->setData(Qt::UserRole,sale.cancelled());
                if (sale.cancelled()) item->setForeground(QColor("#94a3b8"));
                table->setItem(row,col,item);
            }
        }
        table->setCurrentCell(-1,-1); errorLabel->setText(error); selection();
    };
    auto selected=[this,table,errorLabel](Sale &sale) {
        if (table->currentRow()<0) return false;
        QString error; const bool found=m_service->findSale(table->item(table->currentRow(),0)->text().toInt(),sale,error);
        errorLabel->setText(error); return found;
    };
    auto view=[this,selected,errorLabel] {
        Sale sale; if (!selected(sale)) return; QString error;
        const QString html=m_service->receipt(sale.id(),false,error);
        if (!error.isEmpty()) { errorLabel->setText(error); return; }
        ReceiptDialog dialog(html,this); dialog.exec();
    };
    connect(table,&QTableWidget::itemSelectionChanged,page,selection);
    connect(document,&QPushButton::clicked,page,view); connect(table,&QTableWidget::cellDoubleClicked,page,view);
    connect(edit,&QPushButton::clicked,page,[this,selected,reload] {
        Sale sale; if (!selected(sale) || sale.cancelled()) return;
        SaleDialog dialog(m_service,this); dialog.setSale(sale); dialog.exec(); reload();
    });
    connect(remove,&QPushButton::clicked,page,[this,selected,reload,errorLabel] {
        Sale sale; if (!selected(sale)) return;
        if (!confirmAction(this,tr("Отмяна на продажба"),tr("Да се отмени ли продажба №%1? Количествата ще бъдат върнати в склада. Действието се пази в историята.").arg(sale.id()))) return;
        const QString error=m_service->cancelSale(sale.id()); reload(); errorLabel->setText(error);
    });
    connect(refresh,&QPushButton::clicked,page,reload); connect(search,&QLineEdit::textChanged,page,reload); connect(cancelled,&QCheckBox::toggled,page,reload);
    reload(); return page;
}
