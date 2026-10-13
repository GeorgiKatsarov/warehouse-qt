#include "Environment.h"
#include <QtTest>

/// Checks the initial project and the SQLite plugin.
class EnvironmentTest : public QObject
{
    Q_OBJECT
public:
    /// Constructs the test without a parent.
    EnvironmentTest() = default;
    /// Constructs the test with Qt parent ownership.
    explicit EnvironmentTest(QObject *parent) : QObject(parent) {}

private slots:
    void smoke() { QVERIFY(true); }
    void sqliteDriver() { QVERIFY(sqliteDriverAvailable()); }
};

QTEST_GUILESS_MAIN(EnvironmentTest)
#include "tst_environment.moc"
