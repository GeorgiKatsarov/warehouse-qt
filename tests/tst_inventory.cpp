#include "Product.h"
#include <QtTest>

class InventoryTest : public QObject
{
    Q_OBJECT
public:
    InventoryTest() = default;
    explicit InventoryTest(QObject *parent) : QObject(parent) {}
private slots:
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
