#include "Product.h"
#include "Database.h"
#include "WarehouseService.h"
#include <QSqlQuery>
#include <QTemporaryDir>
#include <limits>
#include <QtTest>

class InventoryTest : public QObject
{
    Q_OBJECT
public:
    InventoryTest() = default;
    explicit InventoryTest(QObject *parent) : QObject(parent) {}
private slots:
    void persistentDataAndEmptyReports()
    {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        const QString path = directory.path()+"/warehouse.sqlite";
        {
            Database database(path); QVERIFY(database.open().isEmpty());
            WarehouseService service(database.connection()); QVERIFY(service.login("admin","admin").isEmpty());
            QString error;
            for (const Product &product : service.products(error))
                QVERIFY(service.deleteProduct(product.id()).isEmpty());
            Report empty(QDate::currentDate().addYears(-1),QDate::currentDate().addYears(-1));
            QVERIFY(service.generateReport(empty).isEmpty());
            QCOMPARE(empty.revenue(),qint64(0)); QCOMPARE(empty.profit(),qint64(0)); QCOMPARE(empty.unitsSold(),qint64(0));
            Report invalid(QDate::currentDate(),QDate::currentDate().addDays(-1));
            QVERIFY(!service.generateReport(invalid).isEmpty());
        }
        {
            Database database(path); QVERIFY(database.open().isEmpty());
            QString error;
            QVERIFY(ProductRepository(database.connection()).all(error).empty());
            QCOMPARE(UserRepository(database.connection()).all(error).size(),size_t(3));
            QCOMPARE(SaleRepository(database.connection()).reports(error).size(),size_t(1));
            Product noSupplier(0,"Product","SKU",2,QString(),50,100);
            QVERIFY(ProductRepository(database.connection()).save(noSupplier).isEmpty());
        }
    }
    void largeTotalsAreRejected()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        const qint64 maximum = std::numeric_limits<qint64>::max();
        Product first(0,"First","BIG1",2,"Supplier",maximum,1);
        Product second(0,"Second","BIG2",2,"Supplier",maximum,1);
        QVERIFY(service.saveProduct(first).isEmpty()); QVERIFY(service.saveProduct(second).isEmpty());
        Sale sale("Client",false); sale.setItems({SaleItem(first.id(),1),SaleItem(second.id(),1)});
        QVERIFY(!service.registerSale(sale).isEmpty());
        first.setDeliveryPrice(1); first.setSalePrice(maximum);
        QVERIFY(service.saveProduct(first).isEmpty());
        sale.setItems({SaleItem(first.id(),2)});
        QVERIFY(!service.registerSale(sale).isEmpty());
        QString error; Product current;
        QVERIFY(ProductRepository(database.connection()).find(first.id(),current,error)); QCOMPARE(current.quantity(),2);
    }
    void salesAreAtomic()
    {
        Database database(":memory:");
        QVERIFY2(database.open().isEmpty(), "Database initialization failed");
        WarehouseService service(database.connection());
        QVERIFY(service.login("admin", "admin").isEmpty());
        QString error;
        auto products = service.products(error);
        QVERIFY(error.isEmpty());
        QVERIFY(products.size() == 10);
        const Product product = products.front();
        Sale sale("<Client>", false);
        sale.setItems({SaleItem(product.id(), 2)});
        QVERIFY(service.registerSale(sale).isEmpty());
        QVERIFY(sale.id() > 0);
        Product current;
        QVERIFY(ProductRepository(database.connection()).find(product.id(), current, error));
        QCOMPARE(current.quantity(), product.quantity() - 2);
        const QString receipt = service.receipt(sale.id(), false, error);
        QVERIFY(error.isEmpty());
        QVERIFY(receipt.contains("&lt;Client&gt;"));
        QVERIFY(receipt.contains(QString::fromUtf8("€")));
        QVERIFY(!receipt.contains(QString::fromUtf8("лв.")));
        QVERIFY(!service.registerSale(sale).isEmpty());

        Sale tooMany("Client", false);
        tooMany.setItems({SaleItem(product.id(), current.quantity() + 1)});
        QVERIFY(!service.registerSale(tooMany).isEmpty());
        QCOMPARE(tooMany.id(), 0);
        const int remaining = current.quantity();
        QVERIFY(ProductRepository(database.connection()).find(product.id(), current, error));
        QCOMPARE(current.quantity(), remaining);

        QSqlQuery query(database.connection());
        QVERIFY(query.exec("CREATE TRIGGER reject_item BEFORE INSERT ON sale_items BEGIN SELECT RAISE(ABORT,'test rollback'); END"));
        Sale rejected("Client", false);
        rejected.setItems({SaleItem(product.id(), 1)});
        QVERIFY(!service.registerSale(rejected).isEmpty());
        QCOMPARE(rejected.id(), 0);
        QVERIFY(ProductRepository(database.connection()).find(product.id(), current, error));
        QCOMPARE(current.quantity(), remaining);
        QVERIFY(query.exec("SELECT COUNT(*) FROM sales"));
        QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(), 1);
        QVERIFY(query.exec("DROP TRIGGER reject_item"));

        Report report(QDate::currentDate(), QDate::currentDate());
        QVERIFY(service.generateReport(report).isEmpty());
        QCOMPARE(report.revenue(), sale.total());
        QCOMPARE(report.profit(), sale.total() - 2 * product.deliveryPrice());
        QCOMPARE(report.unitsSold(), qint64(2));
        QCOMPARE(service.reports(error).size(), size_t(1));
        QVERIFY(!service.deleteProduct(product.id()).isEmpty());
    }
    void invalidCart()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("cashier","cashier").isEmpty());
        Sale sale("Client", false);
        QVERIFY(!service.registerSale(sale).isEmpty());
        sale.setItems({SaleItem(1,0)});
        QVERIFY(!service.registerSale(sale).isEmpty());
        sale.setItems({SaleItem(1,1),SaleItem(1,1)});
        QVERIFY(!service.registerSale(sale).isEmpty());
        sale.setItems({SaleItem(9999,1)});
        QVERIFY(!service.registerSale(sale).isEmpty());
    }
    void saleSnapshotsSurviveProductEdits()
    {
        Database database(":memory:"); QVERIFY(database.open().isEmpty());
        WarehouseService service(database.connection()); QVERIFY(service.login("admin","admin").isEmpty());
        QString error;
        Product product;
        QVERIFY(ProductRepository(database.connection()).find(1,product,error));
        Sale sale("Client",false); sale.setItems({SaleItem(1,2)});
        QVERIFY(service.registerSale(sale).isEmpty());
        const QString original = service.receipt(sale.id(),false,error);
        product.setName("Changed"); product.setSku("CHANGED"); product.setDeliveryPrice(9000);
        product.setSalePrice(10000);
        QVERIFY(service.saveProduct(product).isEmpty());
        QCOMPARE(service.receipt(sale.id(),false,error),original);
        Report report(QDate::currentDate(),QDate::currentDate());
        QVERIFY(service.generateReport(report).isEmpty());
        QCOMPARE(report.revenue(),qint64(200)); QCOMPARE(report.profit(),qint64(100));
    }
    void uniqueSkuAndDatabaseIsolation()
    {
        Database first(":memory:"), second(":memory:");
        QVERIFY(first.open().isEmpty()); QVERIFY(second.open().isEmpty());
        Product duplicate(0,"Duplicate","P001",1,"Supplier",1,2);
        QVERIFY(!ProductRepository(first.connection()).save(duplicate).isEmpty());
        Product unique(0,"Unique","UNIQUE",1,"Supplier",1,2);
        QVERIFY(ProductRepository(first.connection()).save(unique).isEmpty());
        QString error;
        QCOMPARE(ProductRepository(first.connection()).all(error).size(), size_t(11));
        QCOMPARE(ProductRepository(second.connection()).all(error).size(), size_t(10));
    }
    void validation_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("sku");
        QTest::addColumn<int>("quantity");
        QTest::addColumn<qint64>("delivery");
        QTest::addColumn<qint64>("price");
        QTest::addColumn<bool>("valid");
        QTest::newRow("valid") << QString("Молив") << QString("A") << 2 << qint64(10) << qint64(20) << true;
        QTest::newRow("empty-name") << QString(" ") << QString("A") << 2 << qint64(10) << qint64(20) << false;
        QTest::newRow("empty-sku") << QString("Молив") << QString() << 2 << qint64(10) << qint64(20) << false;
        QTest::newRow("negative-stock") << QString("Молив") << QString("A") << -1 << qint64(10) << qint64(20) << false;
        QTest::newRow("negative-delivery") << QString("Молив") << QString("A") << 2 << qint64(-1) << qint64(20) << false;
        QTest::newRow("negative-price") << QString("Молив") << QString("A") << 2 << qint64(10) << qint64(-1) << false;
    }
    void validation()
    {
        QFETCH(QString, name); QFETCH(QString, sku); QFETCH(int, quantity);
        QFETCH(qint64, delivery); QFETCH(qint64, price); QFETCH(bool, valid);
        const Product product(0, name, sku, quantity, "", delivery, price);
        QCOMPARE(product.validate().isEmpty(), valid);
    }
};
QTEST_GUILESS_MAIN(InventoryTest)
#include "tst_inventory.moc"
