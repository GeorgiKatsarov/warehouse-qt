#include "User.h"
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
