#include "Database.h"
#include "WarehouseService.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include "ProductDialog.h"
#include "ProductTableModel.h"
#include <QtTest>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTabWidget>

class GuiTest : public QObject
{
    Q_OBJECT
public:
    GuiTest() = default;
    explicit GuiTest(QObject *parent) : QObject(parent) {}
private slots:
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
