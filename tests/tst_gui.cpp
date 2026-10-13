#include "Database.h"
#include "WarehouseService.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include "ProductDialog.h"
#include "ProductTableModel.h"
#include "SaleDialog.h"
#include "ReceiptDialog.h"
#include "UiHelpers.h"
#include "ReportDialog.h"
#include "ProductDetailDialog.h"
#include <QtTest>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QDateEdit>
#include <QInputDialog>

class GuiTest : public QObject
{
    Q_OBJECT
public:
    GuiTest() = default;
    explicit GuiTest(QObject *parent) : QObject(parent) {}
private slots:
    void initTestCase() { applyTheme(*qobject_cast<QApplication*>(QCoreApplication::instance())); }
    void productHistorySearchAndReportExports()
    {
        Database db(":memory:"); QVERIFY(db.open().isEmpty());
        WarehouseService service(db.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        Sale sale("Client",false); sale.setItems({SaleItem(1,3)}); QVERIFY(service.registerSale(sale).isEmpty());
        MainWindow window(&service,nullptr); window.show();
        auto tabs=window.findChild<QTabWidget*>("mainTabs"); tabs->setCurrentIndex(0);
        auto search=window.findChild<QLineEdit*>("productSearch"); search->setText("P001");
        auto table=window.findChild<QTableView*>("productsTable"); QCOMPARE(table->model()->rowCount(),1); table->selectRow(0);
        bool opened=false;
        QTimer::singleShot(0,[&opened] {
            auto dialog=dynamic_cast<ProductDetailDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            const QString folder=qEnvironmentVariable("WAREHOUSE_SCREENSHOT_DIR");
            if (!folder.isEmpty()) { QTest::qWait(30); dialog->grab().save(folder+"/history-new.png"); }
            opened=dialog->findChild<QTableWidget*>("productHistoryTable")->rowCount()==2; dialog->accept();
        });
        window.findChild<QPushButton*>("productDetails")->click(); QVERIFY(opened);
        opened=false;
        const QPoint cell=table->visualRect(table->model()->index(0,0)).center();
        QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,cell,0);
        QTimer::singleShot(0,[&opened] {
            auto dialog=dynamic_cast<ProductDetailDialog*>(QApplication::activeModalWidget());
            if (dialog) { opened=true; dialog->accept(); }
        });
        QTest::mouseDClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,cell,0);
        QVERIFY(opened);
        const QString directory=qEnvironmentVariable("WAREHOUSE_SCREENSHOT_DIR");
        if (!directory.isEmpty()) {
            QVERIFY(QDir().mkpath(directory));
            QVERIFY(window.grab().save(directory+"/products-new.png"));
            tabs->setCurrentIndex(4); QVERIFY(window.grab().save(directory+"/dashboard-new.png"));
        }
        search->clear(); QCOMPARE(table->model()->rowCount(),10);
        tabs->setCurrentIndex(2);
        for (int type=0;type<5;++type) {
            window.findChild<QComboBox*>("reportType")->setCurrentIndex(type);
            window.findChild<QPushButton*>("generateReport")->click();
        }
        QCOMPARE(window.findChild<QTableWidget*>("reportsTable")->rowCount(),5);
        if (!directory.isEmpty()) QVERIFY(window.grab().save(directory+"/reports-new.png"));
        QString error; const auto reports=service.reports(error); QCOMPARE(reports.size(),size_t(5));
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        for (const Report &report : reports) {
            ReportDialog dialog(report,nullptr);
            const QString prefix=temporary.path()+"/report-"+QString::number(static_cast<int>(report.type()));
            QVERIFY2(dialog.exportXls(prefix+".xls").isEmpty(),"XLS export failed"); QVERIFY(dialog.exportPdf(prefix+".pdf").isEmpty());
            QFile file(prefix+".xls"); QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.read(8),QByteArray::fromHex("d0cf11e0a1b11ae1"));
            QVERIFY(!dialog.exportXls(temporary.path()+"/missing/file.xls").isEmpty());
            if (!directory.isEmpty()) {
                const QString name=directory+"/report-"+QString::number(static_cast<int>(report.type()));
                QVERIFY(dialog.exportXls(name+".xls").isEmpty()); QVERIFY(dialog.exportPdf(name+".pdf").isEmpty());
                if (report.type()==ReportType::FinancialSummary) { dialog.show(); QTest::qWait(50); QVERIFY(dialog.grab().save(directory+"/report-open-new.png")); dialog.hide(); }
            }
        }
        Report stress=reports.front(); stress.setHeaders({"Text","EUR"}); stress.setMoneyColumns({1});
        std::vector<QStringList> rows;
        for (int i=0;i<700;++i) rows.push_back({QString::fromUtf8("Тестов продукт ")+QString::number(i),QString::number(i*123)});
        rows.push_back({"=1+1","9007199254740993"}); stress.setRows(rows);
        ReportDialog stressDialog(stress,nullptr); QVERIFY(stressDialog.exportXls(temporary.path()+"/stress.xls").isEmpty());
        if (!directory.isEmpty()) QVERIFY(stressDialog.exportXls(directory+"/stress.xls").isEmpty());
        auto archive=window.findChild<QTableWidget*>("reportsTable"); archive->selectRow(0);
        QTimer::singleShot(0,[] {
            if (auto input=qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
                input->setTextValue("Renamed saved report"); input->accept();
            }
        });
        window.findChild<QPushButton*>("renameReport")->click();
        QCOMPARE(archive->item(0,1)->text(),QString("Renamed saved report"));
        archive->selectRow(0);
        QTimer::singleShot(0,[] {
            if (auto box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) box->button(QMessageBox::Yes)->click();
        });
        window.findChild<QPushButton*>("deleteReport")->click(); QCOMPARE(archive->rowCount(),4);
        tabs->setCurrentIndex(5);
        auto historyTable=window.findChild<QTableWidget*>("salesHistoryTable"); QCOMPARE(historyTable->rowCount(),1); historyTable->selectRow(0);
        QTimer::singleShot(0,[] {
            auto editor=dynamic_cast<SaleDialog*>(QApplication::activeModalWidget()); if (!editor) return;
            Sale changed=editor->sale(); changed.setItems({SaleItem(1,1)}); editor->setSale(changed);
            QTimer::singleShot(0,[] { if (auto receipt=qobject_cast<QDialog*>(QApplication::activeModalWidget())) receipt->accept(); });
            editor->findChild<QPushButton*>("completeSale")->click();
        });
        window.findChild<QPushButton*>("editSale")->click();
        Sale corrected; QVERIFY(service.findSale(sale.id(),corrected,error)); QCOMPARE(corrected.items().front().quantity(),1);
        historyTable->selectRow(0);
        QTimer::singleShot(0,[] {
            if (auto box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) box->button(QMessageBox::Yes)->click();
        });
        window.findChild<QPushButton*>("cancelSale")->click();
        QVERIFY(service.findSale(sale.id(),corrected,error)); QVERIFY(corrected.cancelled());
    }
    void productCrudThroughWindow()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        MainWindow window(&service,nullptr);
        QTimer::singleShot(0,[] {
            auto dialog = dynamic_cast<ProductDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            dialog->findChild<QLineEdit*>("productName")->setText("UI Product");
            dialog->findChild<QLineEdit*>("productSku")->setText("UI-SKU");
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
        });
        window.findChild<QPushButton*>("addProduct")->click();
        auto table = window.findChild<QTableView*>("productsTable");
        QCOMPARE(table->model()->rowCount(),11);
        auto model = static_cast<ProductTableModel*>(table->model());
        auto selectSku = [table,model] {
            const auto rows = model->products();
            for (int row=0; row<static_cast<int>(rows.size()); ++row)
                if (rows.at(row).sku()=="UI-SKU") { table->selectRow(row); return true; }
            return false;
        };
        QVERIFY(selectSku());
        QTimer::singleShot(0,[] {
            auto dialog = dynamic_cast<ProductDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            dialog->findChild<QLineEdit*>("productName")->setText("Edited UI Product");
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
        });
        window.findChild<QPushButton*>("editProduct")->click();
        QVERIFY(selectSku());
        QCOMPARE(model->data(model->index(table->currentIndex().row(),0)).toString(),QString("Edited UI Product"));
        QTimer::singleShot(0,[] {
            if (auto box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
                box->button(QMessageBox::Yes)->click();
        });
        window.findChild<QPushButton*>("deleteProduct")->click();
        QCOMPARE(model->rowCount(),10);
    }
    void userEditingAndDeletionThroughWindow()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        Cashier user(0,"editable",hashPassword("old"),"Editable");
        QVERIFY(service.saveUser(user).isEmpty());
        MainWindow window(&service,nullptr); window.show();
        window.findChild<QTabWidget*>("mainTabs")->setCurrentIndex(3);
        auto table = window.findChild<QTableWidget*>("usersTable");
        auto selectUsername = [table](const QString &username) {
            for (int row=0; row<table->rowCount(); ++row) {
                if (table->item(row,0)->text()!=username) continue;
                table->setCurrentCell(row,0);
                QMetaObject::invokeMethod(table,"cellClicked",Q_ARG(int,row),Q_ARG(int,0));
                return true;
            }
            return false;
        };
        QVERIFY(selectUsername("editable"));
        window.findChild<QLineEdit*>("userFullName")->setText("Updated");
        window.findChild<QComboBox*>("userRole")->setCurrentIndex(1);
        window.findChild<QPushButton*>("saveUser")->click();
        WarehouseService other(database.connection());
        QVERIFY(other.login("editable","old").isEmpty());
        QCOMPARE(other.currentUser()->fullName(),QString("Updated"));
        QCOMPARE(other.currentUser()->roleName(),QString("manager"));
        QVERIFY(selectUsername("editable"));
        QTimer::singleShot(0,[] {
            if (auto box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
                box->button(QMessageBox::Yes)->click();
        });
        window.findChild<QPushButton*>("deleteUser")->click();
        QCOMPARE(table->rowCount(),3);
        QVERIFY(!other.login("editable","old").isEmpty());
    }
    void seasonalCheckoutValidation()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("cashier","cashier").isEmpty());
        SaleDialog dialog(&service,nullptr);
        dialog.findChild<QComboBox*>("saleProduct")->setCurrentIndex(
            dialog.findChild<QComboBox*>("saleProduct")->findData(1));
        dialog.findChild<QPushButton*>("addToCart")->click();
        dialog.findChild<QComboBox*>("discount")->setCurrentIndex(3);
        dialog.findChild<QSpinBox*>("seasonPercent")->setValue(20);
        QCOMPARE(dialog.sale().total(),qint64(80));
        dialog.findChild<QDateEdit*>("seasonTo")->setDate(QDate::currentDate().addDays(-1));
        QVERIFY(!dialog.findChild<QPushButton*>("completeSale")->isEnabled());
        dialog.findChild<QDateEdit*>("seasonTo")->setDate(QDate::currentDate());
        QVERIFY(dialog.findChild<QPushButton*>("completeSale")->isEnabled());
        dialog.findChild<QSpinBox*>("saleQuantity")->setValue(1000);
        dialog.findChild<QPushButton*>("addToCart")->click();
        QVERIFY(!dialog.findChild<QPushButton*>("completeSale")->isEnabled());
    }
    void checkoutReportsUsersAndPdf()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        MainWindow window(&service,nullptr); window.show(); QTest::qWait(50);
        auto tabs = window.findChild<QTabWidget*>("mainTabs"); QCOMPARE(tabs->count(),6);
        tabs->setCurrentIndex(1);
        auto sales = static_cast<SaleDialog*>(tabs->widget(1));
        auto product = sales->findChild<QComboBox*>("saleProduct");
        product->setCurrentIndex(product->findData(1));
        sales->findChild<QSpinBox*>("saleQuantity")->setValue(10);
        sales->findChild<QPushButton*>("addToCart")->click();
        sales->findChild<QCheckBox*>("vip")->setChecked(true);
        sales->findChild<QComboBox*>("discount")->setCurrentIndex(2);
        QCOMPARE(sales->sale().total(),qint64(900));
        QVERIFY(sales->findChild<QPushButton*>("completeSale")->isEnabled());
        const QString previewDirectory = qEnvironmentVariable("WAREHOUSE_SCREENSHOT_DIR");
        if (!previewDirectory.isEmpty()) {
            QVERIFY(QDir().mkpath(previewDirectory));
            QVERIFY(window.grab().save(previewDirectory+"/sales.png"));
        }
        QTimer::singleShot(0,[] {
            if (auto dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) dialog->accept();
        });
        sales->findChild<QPushButton*>("completeSale")->click();
        QVERIFY2(sales->sale().items().empty(),
                 qPrintable(sales->findChild<QLabel*>("saleError")->text()));
        QString error; Product current;
        QVERIFY(ProductRepository(database.connection()).find(1,current,error)); QCOMPARE(current.quantity(),90);
        tabs->setCurrentIndex(0);
        // Model rows are sorted by name; verify the sold SKU's row.
        auto productModel = static_cast<ProductTableModel*>(window.findChild<QTableView*>("productsTable")->model());
        for (const Product &row : productModel->products())
            if (row.id()==1) QCOMPARE(row.quantity(),90);
        const QString html = service.receipt(1,true,error); QVERIFY(error.isEmpty());
        QVERIFY(html.contains(QString::fromUtf8("Фактура")));
        ReceiptDialog receipt(html,nullptr);
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QVERIFY(receipt.savePdf(directory.path()+"/invoice.pdf").isEmpty());
        if (!previewDirectory.isEmpty()) {
            QVERIFY(receipt.savePdf(previewDirectory+"/invoice.pdf").isEmpty());
            receipt.show(); QTest::qWait(50);
            QVERIFY(receipt.grab().save(previewDirectory+"/receipt.png"));
            receipt.hide();
        }
        QFile pdf(directory.path()+"/invoice.pdf"); QVERIFY(pdf.open(QIODevice::ReadOnly));
        QCOMPARE(pdf.read(5),QByteArray("%PDF-")); QVERIFY(pdf.size()>1000);
        QVERIFY(!receipt.savePdf(directory.path()+"/missing/invoice.pdf").isEmpty());
        tabs->setCurrentIndex(2);
        window.findChild<QPushButton*>("generateReport")->click();
        QCOMPARE(window.findChild<QTableWidget*>("reportsTable")->rowCount(),1);
        QVERIFY(window.findChild<QLabel*>("reportError")->text().isEmpty());
        if (!previewDirectory.isEmpty()) QVERIFY(window.grab().save(previewDirectory+"/reports.png"));
        tabs->setCurrentIndex(3);
        window.findChild<QPushButton*>("newUser")->click();
        window.findChild<QLineEdit*>("userUsername")->setText("testcashier");
        window.findChild<QLineEdit*>("userFullName")->setText("Тестов касиер");
        window.findChild<QLineEdit*>("userPassword")->setText("testpassword");
        window.findChild<QPushButton*>("saveUser")->click();
        QVERIFY(window.findChild<QLabel*>("userError")->text().isEmpty());
        QCOMPARE(window.findChild<QTableWidget*>("usersTable")->rowCount(),4);
        if (!previewDirectory.isEmpty()) QVERIFY(window.grab().save(previewDirectory+"/users.png"));
        QVERIFY(service.login("testcashier","testpassword").isEmpty());
        window.setService(&service);
        tabs = window.findChild<QTabWidget*>("mainTabs");
        QCOMPARE(tabs->count(),5); QVERIFY(!tabs->isTabEnabled(2));
    }
    void loginAndRoleButtons()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection());
        LoginDialog login(&service,nullptr);
        login.findChild<QLineEdit*>("username")->setText("cashier");
        login.findChild<QLineEdit*>("password")->setText("cashier");
        login.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(login.result(),int(QDialog::Accepted));
        MainWindow window(&service,nullptr);
        QVERIFY(!window.findChild<QPushButton*>("addProduct")->isEnabled());
        QCOMPARE(window.findChild<QTableView*>("productsTable")->model()->rowCount(),10);
        QVERIFY(service.login("admin","admin").isEmpty());
        window.setService(&service);
        QVERIFY(window.findChild<QPushButton*>("addProduct")->isEnabled());
    }
    void productInputAndModel()
    {
        ProductDialog dialog;
        auto buttons = dialog.findChild<QDialogButtonBox*>();
        buttons->button(QDialogButtonBox::Save)->click();
        QCOMPARE(dialog.result(),int(QDialog::Rejected));
        dialog.findChild<QLineEdit*>("productName")->setText("Test");
        dialog.findChild<QLineEdit*>("productSku")->setText("TEST");
        dialog.findChild<QLineEdit*>("salePrice")->setText("12,34");
        dialog.findChild<QLineEdit*>("deliveryPrice")->setText("10.00");
        dialog.findChild<QSpinBox*>("quantity")->setValue(3);
        buttons->button(QDialogButtonBox::Save)->click();
        QCOMPARE(dialog.result(),int(QDialog::Accepted));
        QCOMPARE(dialog.product().salePrice(),qint64(1234));
        ProductTableModel model({dialog.product()},nullptr);
        QCOMPARE(model.rowCount(),1);
        QCOMPARE(model.data(model.index(0,5)).toString(),QString::fromUtf8("12.34 €"));
        QCOMPARE(formatCents(-123),QString::fromUtf8("-1.23 €"));
        model.setProducts({}); QCOMPARE(model.rowCount(),0);
    }
};
QTEST_MAIN(GuiTest)
#include "tst_gui.moc"
