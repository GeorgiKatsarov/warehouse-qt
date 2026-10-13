#include "Discount.h"
#include <QtTest>
#include <memory>
#include <limits>

class DiscountsTest : public QObject
{
    Q_OBJECT
public:
    DiscountsTest() = default;
    explicit DiscountsTest(QObject *parent) : QObject(parent) {}
private slots:
    void quantity_data()
    {
        QTest::addColumn<int>("quantity"); QTest::addColumn<qint64>("expected");
        QTest::newRow("9") << 9 << qint64(9000);
        QTest::newRow("10") << 10 << qint64(9500);
        QTest::newRow("49") << 49 << qint64(46550);
        QTest::newRow("50") << 50 << qint64(45000);
    }
    void quantity()
    {
        QFETCH(int, quantity); QFETCH(qint64, expected);
        QCOMPARE(QuantityDiscount().apply(1000, quantity, false, QDate()), expected);
    }
    void otherStrategies()
    {
        QCOMPARE(NoDiscount().apply(123, 3, true, QDate()), qint64(369));
        QCOMPARE(VipDiscount().apply(123, 3, true, QDate()), qint64(332));
        QCOMPARE(VipDiscount().apply(123, 3, false, QDate()), qint64(369));
        QCOMPARE(NoDiscount().apply(std::numeric_limits<qint64>::max(), 2, false, QDate()), qint64(-1));
    }
    void seasonal_data()
    {
        QTest::addColumn<QDate>("date"); QTest::addColumn<qint64>("expected");
        QTest::newRow("before") << QDate(2026,1,9) << qint64(1000);
        QTest::newRow("start") << QDate(2026,1,10) << qint64(800);
        QTest::newRow("inside") << QDate(2026,1,15) << qint64(800);
        QTest::newRow("end") << QDate(2026,1,20) << qint64(800);
        QTest::newRow("after") << QDate(2026,1,21) << qint64(1000);
    }
    void seasonal()
    {
        QFETCH(QDate, date); QFETCH(qint64, expected);
        QCOMPARE(SeasonalDiscount(20,QDate(2026,1,10),QDate(2026,1,20)).apply(1000,1,false,date), expected);
    }
};
QTEST_GUILESS_MAIN(DiscountsTest)
#include "tst_discounts.moc"
