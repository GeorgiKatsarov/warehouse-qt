#include "User.h"
#include "Database.h"
#include "WarehouseService.h"
#include <QtTest>
#include <memory>
#include <vector>

class PermissionsTest : public QObject
{
    Q_OBJECT
public:
    PermissionsTest() = default;
    explicit PermissionsTest(QObject *parent) : QObject(parent) {}
private slots:
    void userMaintenanceAndSessionSafety()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        const int adminId = service.currentUser()->id();
        QVERIFY(!service.deleteUser(adminId).isEmpty());
        Cashier demoted(adminId,"admin",hashPassword("admin"),"Admin");
        QVERIFY(!service.saveUser(demoted).isEmpty());
        Cashier added(0,"new",hashPassword("first"),"New");
        QVERIFY(service.saveUser(added).isEmpty());
        WarehouseManager changed(added.id(),"renamed",hashPassword("second"),"Manager");
        QVERIFY(service.saveUser(changed).isEmpty());
        WarehouseService other(database.connection());
        QVERIFY(!other.login("new","first").isEmpty());
        QVERIFY(other.login("renamed","second").isEmpty());
        QVERIFY(other.hasPermission(Permission::ViewReports));
        QVERIFY(!other.hasPermission(Permission::RegisterSale));
        QVERIFY(service.deleteUser(added.id()).isEmpty());
        QVERIFY(!other.login("renamed","second").isEmpty());
        Cashier duplicate(0,"admin",hashPassword("test"),"Duplicate");
        QVERIFY(!service.saveUser(duplicate).isEmpty());
        service.logout(); QVERIFY(!service.currentUser());
    }
    void serviceChecks_data()
    {
        QTest::addColumn<QString>("username");
        QTest::addColumn<bool>("products");
        QTest::addColumn<bool>("sale");
        QTest::addColumn<bool>("reports");
        QTest::addColumn<bool>("users");
        QTest::newRow("admin") << QString("admin") << true << true << true << true;
        QTest::newRow("manager") << QString("manager") << true << false << true << false;
        QTest::newRow("cashier") << QString("cashier") << false << true << false << false;
    }
    void serviceChecks()
    {
        QFETCH(QString, username); QFETCH(bool, products); QFETCH(bool, sale);
        QFETCH(bool, reports); QFETCH(bool, users);
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection());
        QVERIFY(service.login(username,username).isEmpty());
        Product product(0,"New","NEW",5,"Supplier",100,200);
        QCOMPARE(service.saveProduct(product).isEmpty(), products);
        if (products) QVERIFY(service.deleteProduct(product.id()).isEmpty());
        else QVERIFY(!service.deleteProduct(1).isEmpty());
        Sale transaction("Client",false); transaction.setItems({SaleItem(1,1)});
        QCOMPARE(service.previewSale(transaction).isEmpty(), sale);
        QCOMPARE(service.registerSale(transaction).isEmpty(), sale);
        QString error;
        service.receipt(1,false,error); QCOMPARE(error.isEmpty(), sale);
        Report report(QDate::currentDate(),QDate::currentDate());
        QCOMPARE(service.generateReport(report).isEmpty(), reports);
        service.reports(error); QCOMPARE(error.isEmpty(), reports);
        service.users(error); QCOMPARE(error.isEmpty(), users);
        Cashier cashier(0,"newcashier",hashPassword("password"),"New Cashier");
        QCOMPARE(service.saveUser(cashier).isEmpty(), users);
        if (users) QVERIFY(service.deleteUser(cashier.id()).isEmpty());
        else QVERIFY(!service.deleteUser(1).isEmpty());
        QVERIFY(!service.login(username,"wrong").isEmpty());
        QVERIFY(!service.currentUser());
        QVERIFY(!service.saveProduct(product).isEmpty());
        service.products(error); QVERIFY(!error.isEmpty());
    }
    void roles_data()
    {
        QTest::addColumn<int>("role");
        QTest::addColumn<int>("permission");
        QTest::addColumn<bool>("allowed");
        const bool expected[3][4] = {{true,true,true,true},{true,false,true,false},{false,true,false,false}};
        for (int role = 0; role < 3; ++role)
            for (int permission = 0; permission < 4; ++permission)
                QTest::newRow(qPrintable(QString("%1-%2").arg(role).arg(permission)))
                    << role << permission << expected[role][permission];
    }
    void roles()
    {
        QFETCH(int, role);
        QFETCH(int, permission);
        QFETCH(bool, allowed);
        const std::vector<std::shared_ptr<User>> users = {
            std::make_shared<Admin>(), std::make_shared<WarehouseManager>(), std::make_shared<Cashier>()
        };
        QCOMPARE(users.at(role)->hasPermission(static_cast<Permission>(permission)), allowed);
    }
};
QTEST_GUILESS_MAIN(PermissionsTest)
#include "tst_permissions.moc"
