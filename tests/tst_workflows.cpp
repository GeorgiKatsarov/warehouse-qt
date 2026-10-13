#include "Database.h"
#include "WarehouseService.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QSqlQuery>

class WorkflowsTest : public QObject
{
    Q_OBJECT
public:
    WorkflowsTest() = default;
    explicit WorkflowsTest(QObject *parent) : QObject(parent) {}
private slots:
    void upgradePreservesEarlierDatabase()
    {
        QTemporaryDir directory; const QString path=directory.path()+"/old.sqlite";
        {
            Database db(path); QVERIFY(db.open().isEmpty());
            WarehouseService service(db.connection()); QVERIFY(service.login("admin","admin").isEmpty());
            Sale sale("Existing",false); sale.setItems({SaleItem(1,2)}); QVERIFY(service.registerSale(sale).isEmpty());
            Report report(QDate::currentDate(),QDate::currentDate()); QVERIFY(service.generateReport(report).isEmpty());
            QSqlQuery query(db.connection());
            QVERIFY(query.exec("DROP TABLE product_history"));
            QVERIFY(query.exec("ALTER TABLE sales DROP COLUMN status"));
            for (const QString &column : {QString("type"),QString("title"),QString("payload")})
                QVERIFY(query.exec("ALTER TABLE reports DROP COLUMN "+column));
        }
        {
            Database db(path); QVERIFY(db.open().isEmpty());
            WarehouseService service(db.connection()); QVERIFY(service.login("admin","admin").isEmpty());
            QString error; Product product; QVERIFY(ProductRepository(db.connection()).find(1,product,error));
            QCOMPARE(product.quantity(),98);
            Sale sale; QVERIFY(service.findSale(1,sale,error)); QCOMPARE(sale.total(),qint64(200)); QVERIFY(!sale.cancelled());
            Report report; QVERIFY(service.findReport(1,report,error)); QCOMPARE(report.revenue(),qint64(200)); QVERIFY(!report.rows().empty());
            const auto history=service.productHistory(1,error); QCOMPARE(history.size(),size_t(2));
            bool imported=false; for (const auto &entry : history) if (entry.saleId()==1) imported=true;
            QVERIFY(imported);
        }
    }
    void rawPasswordValidationAndPermissionRefresh()
    {
        Database db(":memory:"); QVERIFY(db.open().isEmpty());
        WarehouseService admin(db.connection()); QVERIFY(admin.login("admin","admin").isEmpty());
        Cashier user(0,"newcashier","","New cashier");
        QVERIFY(!admin.saveUser(user,"short").isEmpty()); QVERIFY(admin.saveUser(user,"password123").isEmpty());
        WarehouseService session(db.connection()); QVERIFY(session.login("newcashier","password123").isEmpty());
        WarehouseManager manager(user.id(),user.username(),user.passwordHash(),user.fullName());
        QVERIFY(admin.saveUser(manager,QString()).isEmpty());
        QVERIFY(!session.hasPermission(Permission::RegisterSale)); QVERIFY(session.hasPermission(Permission::ViewReports));
    }
    void allReportTypesPersistAsSnapshots()
    {
        QTemporaryDir directory; const QString path=directory.path()+"/data.sqlite";
        std::vector<int> ids; std::vector<std::vector<QStringList>> snapshots;
        {
            Database db(path); QVERIFY(db.open().isEmpty());
            WarehouseService service(db.connection()); QVERIFY(service.login("admin","admin").isEmpty());
            Sale sale("Client",false); sale.setItems({SaleItem(1,3)}); QVERIFY(service.registerSale(sale).isEmpty());
            Product low; QString error; QVERIFY(ProductRepository(db.connection()).find(2,low,error));
            low.setQuantity(5); QVERIFY(service.saveProduct(low).isEmpty());
            for (int i=0;i<5;++i) {
                Report report(QDate::currentDate(),QDate::currentDate()); report.setType(static_cast<ReportType>(i));
                QVERIFY2(service.generateReport(report).isEmpty(),"Report generation failed");
                QVERIFY(!report.headers().isEmpty()); QVERIFY(!report.rows().empty());
                QCOMPARE(report.revenue(),qint64(300)); QCOMPARE(report.profit(),qint64(150)); QCOMPARE(report.unitsSold(),qint64(3));
                ids.push_back(report.id()); snapshots.push_back(report.rows());
            }
            low.setQuantity(99); low.setName("Changed"); QVERIFY(service.saveProduct(low).isEmpty());
            Sale later("Later",false); later.setItems({SaleItem(1,1)}); QVERIFY(service.registerSale(later).isEmpty());
        }
        {
            Database db(path); QVERIFY(db.open().isEmpty());
            WarehouseService service(db.connection()); QVERIFY(service.login("manager","manager").isEmpty());
            QString error; QCOMPARE(service.reports(error).size(),size_t(5));
            for (int i=0;i<5;++i) {
                Report report; QVERIFY(service.findReport(ids.at(i),report,error));
                QCOMPARE(static_cast<int>(report.type()),i); QCOMPARE(report.rows(),snapshots.at(i));
            }
            QVERIFY(service.renameReport(ids.front(),"Monthly review").isEmpty());
            Report renamed; QVERIFY(service.findReport(ids.front(),renamed,error)); QCOMPARE(renamed.title(),QString("Monthly review"));
            QVERIFY(!service.renameReport(ids.front()," ").isEmpty());
            QVERIFY(service.deleteReport(ids.back()).isEmpty()); QVERIFY(!service.findReport(ids.back(),renamed,error));
            QVERIFY(!service.deleteReport(99999).isEmpty());
            QVERIFY(service.login("cashier","cashier").isEmpty());
            QVERIFY(!service.findReport(ids.front(),renamed,error)); QVERIFY(!service.renameReport(ids.front(),"Denied").isEmpty());
            QVERIFY(!service.deleteReport(ids.front()).isEmpty());
        }
    }
    void editCancelAndAuditAreAtomic()
    {
        Database db(":memory:"); QVERIFY(db.open().isEmpty());
        WarehouseService service(db.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        Sale sale("Client",false); sale.setItems({SaleItem(1,10)}); QVERIFY(service.registerSale(sale).isEmpty());
        QString error; auto history=service.productHistory(1,error); QCOMPARE(history.size(),size_t(2));
        QCOMPARE(history.front().quantityBefore(),100); QCOMPARE(history.front().quantityAfter(),90);
        Sale edited=sale; edited.setItems({SaleItem(1,20)}); QVERIFY(service.updateSale(edited).isEmpty());
        Product stock; QVERIFY(ProductRepository(db.connection()).find(1,stock,error)); QCOMPARE(stock.quantity(),80);
        Sale loaded; QVERIFY(service.findSale(sale.id(),loaded,error)); QCOMPARE(loaded.items().front().quantity(),20);
        const auto successfulHistory=service.productHistory(1,error).size();
        QSqlQuery query(db.connection());
        QVERIFY(query.exec("CREATE TRIGGER reject_sale BEFORE INSERT ON sale_items BEGIN SELECT RAISE(ABORT,'reject'); END"));
        Sale rejected=edited; rejected.setItems({SaleItem(1,30)});
        QVERIFY(!service.updateSale(rejected).isEmpty());
        QVERIFY(ProductRepository(db.connection()).find(1,stock,error)); QCOMPARE(stock.quantity(),80);
        QVERIFY(service.findSale(sale.id(),loaded,error)); QCOMPARE(loaded.items().front().quantity(),20);
        QCOMPARE(service.productHistory(1,error).size(),successfulHistory);
        QVERIFY(query.exec("DROP TRIGGER reject_sale"));
        QVERIFY(query.exec("CREATE TRIGGER reject_history BEFORE INSERT ON product_history BEGIN SELECT RAISE(ABORT,'reject'); END"));
        QVERIFY(!service.cancelSale(sale.id()).isEmpty());
        QVERIFY(ProductRepository(db.connection()).find(1,stock,error)); QCOMPARE(stock.quantity(),80);
        QVERIFY(service.findSale(sale.id(),loaded,error)); QVERIFY(!loaded.cancelled());
        QVERIFY(query.exec("DROP TRIGGER reject_history"));
        QVERIFY(service.cancelSale(sale.id()).isEmpty());
        QVERIFY(!service.cancelSale(sale.id()).isEmpty());
        QVERIFY(ProductRepository(db.connection()).find(1,stock,error)); QCOMPARE(stock.quantity(),100);
        QVERIFY(service.findSale(sale.id(),loaded,error)); QVERIFY(loaded.cancelled());
        QVERIFY(service.receipt(sale.id(),false,error).contains(QString::fromUtf8("ОТМЕНЕНА")));
        Report report(QDate::currentDate(),QDate::currentDate()); QVERIFY(service.generateReport(report).isEmpty());
        QCOMPARE(report.revenue(),qint64(0)); QVERIFY(!service.updateSale(loaded).isEmpty());
    }
    void saleOwnershipAndDeletedSessions()
    {
        Database db(":memory:"); QVERIFY(db.open().isEmpty());
        WarehouseService admin(db.connection()),cashier(db.connection());
        QVERIFY(admin.login("admin","admin").isEmpty()); QVERIFY(cashier.login("cashier","cashier").isEmpty());
        Sale sale("Admin sale",false); sale.setItems({SaleItem(1,1)}); QVERIFY(admin.registerSale(sale).isEmpty());
        QString error; Sale loaded; QVERIFY(!cashier.findSale(sale.id(),loaded,error)); QVERIFY(!cashier.cancelSale(sale.id()).isEmpty());
        QVERIFY(cashier.sales(error).empty());
        Cashier temporary(0,"temporary",hashPassword("password"),"Temporary"); QVERIFY(admin.saveUser(temporary).isEmpty());
        WarehouseService session(db.connection()); QVERIFY(session.login("temporary","password").isEmpty());
        QVERIFY(admin.deleteUser(temporary.id()).isEmpty()); QVERIFY(!session.hasPermission(Permission::RegisterSale));
        QVERIFY(session.products(error).empty()); QVERIFY(!error.isEmpty());
    }
    void historyDoesNotMixDeletedProductIds()
    {
        Database db(":memory:"); QVERIFY(db.open().isEmpty());
        WarehouseService service(db.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        Product first(0,"First","EXTRA1",3,"Supplier",100,200); QVERIFY(service.saveProduct(first).isEmpty());
        const int removedId=first.id(); QVERIFY(service.deleteProduct(first.id()).isEmpty());
        Product second(0,"Second","EXTRA2",3,"Supplier",100,200); QVERIFY(service.saveProduct(second).isEmpty());
        QVERIFY(second.id()>removedId); QString error;
        QCOMPARE(service.productHistory(removedId,error).size(),size_t(2));
        QCOMPARE(service.productHistory(second.id(),error).size(),size_t(1));
    }
    void validationAndMigration()
    {
        Database db(":memory:"); QVERIFY(db.open().isEmpty());
        WarehouseService service(db.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        Product invalid(0,QString(161,'a'),"SKU",1,"Supplier",0,0); QVERIFY(!service.saveProduct(invalid).isEmpty());
        Cashier bad(0,"x",QString("invalid"),"Name"); QVERIFY(!service.saveUser(bad).isEmpty());
        Report report(QDate::currentDate(),QDate::currentDate()); report.setType(static_cast<ReportType>(99));
        QVERIFY(!service.generateReport(report).isEmpty());
        QSqlQuery query(db.connection());
        QVERIFY(query.exec("INSERT INTO reports(created_at,date_from,date_to,revenue,profit,units_sold) VALUES('2026-01-01T10:00:00','2026-01-01','2026-01-01',100,50,1)"));
        Report legacy; QString error; QVERIFY(service.findReport(query.lastInsertId().toInt(),legacy,error));
        QVERIFY(!legacy.rows().empty()); QCOMPARE(legacy.revenue(),qint64(100));
        QVERIFY(query.exec("SELECT COUNT(*) FROM product_history")); QVERIFY(query.next()); const int count=query.value(0).toInt();
        // Reinitialization must not duplicate baselines or reset business records.
        query.finish(); QVERIFY(db.open().isEmpty());
        QVERIFY(query.exec("SELECT COUNT(*) FROM product_history")); QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(),count);
    }
};
QTEST_GUILESS_MAIN(WorkflowsTest)
#include "tst_workflows.moc"
