#include "Database.h"
#include "WarehouseService.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include "ProductDialog.h"
#include "ProductTableModel.h"
#include "SaleDialog.h"
#include "ReceiptDialog.h"
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

class GuiTest : public QObject
{
    Q_OBJECT
public:
    GuiTest() = default;
    explicit GuiTest(QObject *parent) : QObject(parent) {}
private slots:
    void checkoutReportsUsersAndPdf()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        MainWindow window(&service,nullptr); window.show(); QTest::qWait(50);
        auto tabs = window.findChild<QTabWidget*>("mainTabs"); QCOMPARE(tabs->count(),4);
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
        const QString html = service.receipt(1,true,error); QVERIFY(error.isEmpty());
        QVERIFY(html.contains(QString::fromUtf8("Фактура")));
        ReceiptDialog receipt(html,nullptr);
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QVERIFY(receipt.savePdf(directory.path()+"/invoice.pdf").isEmpty());
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
        QCOMPARE(tabs->count(),3); QVERIFY(!tabs->isTabEnabled(2));
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
        QCOMPARE(model.data(model.index(0,5)).toString(),QString::fromUtf8("12.34 лв."));
        QCOMPARE(formatCents(-123),QString::fromUtf8("-1.23 лв."));
        model.setProducts({}); QCOMPARE(model.rowCount(),0);
    }
};
QTEST_MAIN(GuiTest)
#include "tst_gui.moc"
