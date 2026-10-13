#include "Database.h"
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDateTime>

Database::Database() : Database(QStringLiteral("warehouse.sqlite")) {}
Database::Database(QString filePath)
    : m_filePath(filePath), m_connectionName(QUuid::createUuid().toString()) {}
Database::~Database()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = connection();
            db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}
QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(m_connectionName, false);
}
bool Database::setFilePath(QString value)
{
    if (QSqlDatabase::contains(m_connectionName))
        return false;
    m_filePath = value;
    return true;
}
bool Database::setConnectionName(QString value)
{
    if (QSqlDatabase::contains(m_connectionName) || QSqlDatabase::contains(value) || value.isEmpty())
        return false;
    m_connectionName = value;
    return true;
}
QString hashPassword(const QString &password)
{
    return QString::fromLatin1(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}
QString Database::open()
{
    QSqlDatabase db = QSqlDatabase::contains(m_connectionName)
        ? connection() : QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(m_filePath);
    if (!db.open())
        return db.lastError().text();
    const bool seedUsers = !db.tables().contains("users");
    const bool seedProducts = !db.tables().contains("products");
    QSqlQuery query(db);
    if (!query.exec("PRAGMA foreign_keys = ON"))
        return query.lastError().text();
    if (!db.transaction())
        return db.lastError().text();
    const QStringList statements = {
        "CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY, username TEXT NOT NULL UNIQUE, password_hash TEXT NOT NULL, full_name TEXT NOT NULL, role TEXT NOT NULL CHECK(role IN ('admin','manager','cashier')))",
        "CREATE TABLE IF NOT EXISTS products(id INTEGER PRIMARY KEY, name TEXT NOT NULL, sku TEXT NOT NULL UNIQUE, quantity INTEGER NOT NULL CHECK(quantity>=0), supplier TEXT NOT NULL, delivery_price INTEGER NOT NULL CHECK(delivery_price>=0), sale_price INTEGER NOT NULL CHECK(sale_price>=0))",
        "CREATE TABLE IF NOT EXISTS sales(id INTEGER PRIMARY KEY, created_at TEXT NOT NULL, user_id INTEGER NOT NULL REFERENCES users(id), customer_name TEXT NOT NULL, is_vip INTEGER NOT NULL, discount_name TEXT NOT NULL, total INTEGER NOT NULL CHECK(total>=0))",
        "CREATE TABLE IF NOT EXISTS sale_items(id INTEGER PRIMARY KEY, sale_id INTEGER NOT NULL REFERENCES sales(id), product_id INTEGER NOT NULL REFERENCES products(id), quantity INTEGER NOT NULL CHECK(quantity>0), unit_price INTEGER NOT NULL, line_total INTEGER NOT NULL, delivery_price INTEGER NOT NULL, product_name TEXT NOT NULL, product_sku TEXT NOT NULL)",
        "CREATE TABLE IF NOT EXISTS reports(id INTEGER PRIMARY KEY, created_at TEXT NOT NULL, date_from TEXT NOT NULL, date_to TEXT NOT NULL, revenue INTEGER NOT NULL, profit INTEGER NOT NULL, units_sold INTEGER NOT NULL)"
    };
    for (const QString &sql : statements) {
        if (!query.exec(sql)) {
            const QString error = query.lastError().text();
            db.rollback();
            return error;
        }
    }
    const QStringList upgrades = {
        "sales|status|TEXT NOT NULL DEFAULT 'active'",
        "reports|type|INTEGER NOT NULL DEFAULT 0",
        "reports|title|TEXT NOT NULL DEFAULT ''",
        "reports|payload|TEXT NOT NULL DEFAULT ''"
    };
    for (const QString &upgrade : upgrades) {
        const QStringList parts = upgrade.split('|');
        if (!query.exec("PRAGMA table_info(" + parts.at(0) + ")")) {
            const QString error = query.lastError().text(); db.rollback(); return error;
        }
        bool found = false;
        while (query.next()) if (query.value(1).toString()==parts.at(1)) found = true;
        if (!found && !query.exec("ALTER TABLE " + parts.at(0) + " ADD COLUMN " + parts.at(1) + " " + parts.at(2))) {
            const QString error = query.lastError().text(); db.rollback(); return error;
        }
    }
    if (!query.exec("CREATE TABLE IF NOT EXISTS product_history(id INTEGER PRIMARY KEY, product_id INTEGER NOT NULL, created_at TEXT NOT NULL, actor_name TEXT NOT NULL, action TEXT NOT NULL, quantity_before INTEGER NOT NULL, quantity_after INTEGER NOT NULL, name_before TEXT NOT NULL, name_after TEXT NOT NULL, delivery_before INTEGER NOT NULL, delivery_after INTEGER NOT NULL, price_before INTEGER NOT NULL, price_after INTEGER NOT NULL, sale_id INTEGER NOT NULL DEFAULT 0, notes TEXT NOT NULL)")) {
        const QString error = query.lastError().text(); db.rollback(); return error;
    }
    if (!query.exec("CREATE INDEX IF NOT EXISTS product_history_product ON product_history(product_id,id)")) {
        const QString error = query.lastError().text(); db.rollback(); return error;
    }
    if (seedUsers) {
        const QStringList roles = {"admin", "manager", "cashier"};
        const QStringList names = {
            tr("Администратор"),
            tr("Управител на склада"),
            tr("Касиер")
        };
        for (int i = 0; i < roles.size(); ++i) {
            query.prepare("INSERT INTO users(username,password_hash,full_name,role) VALUES(?,?,?,?)");
            query.addBindValue(roles.at(i)); query.addBindValue(hashPassword(roles.at(i)));
            query.addBindValue(names.at(i)); query.addBindValue(roles.at(i));
            if (!query.exec()) { const QString error = query.lastError().text(); db.rollback(); return error; }
        }
    }
    if (seedProducts) {
        const QStringList names = {
            tr("Молив"),
            tr("Химикал"),
            tr("Тетрадка"),
            tr("Папка"),
            tr("Гума"),
            tr("Линия"),
            tr("Маркер"),
            tr("Лепило"),
            tr("Ножица"),
            tr("Копирна хартия")
        };
        for (int i = 0; i < names.size(); ++i) {
            query.prepare("INSERT INTO products(name,sku,quantity,supplier,delivery_price,sale_price) VALUES(?,?,?,?,?,?)");
            query.addBindValue(names.at(i)); query.addBindValue(QString("P%1").arg(i+1,3,10,QChar('0')));
            query.addBindValue(100);
            query.addBindValue(tr("Демо доставчик"));
            query.addBindValue(50 + i*30); query.addBindValue(100 + i*50);
            if (!query.exec()) { const QString error = query.lastError().text(); db.rollback(); return error; }
        }
    }
    query.prepare("INSERT INTO product_history(product_id,created_at,actor_name,action,quantity_before,quantity_after,name_before,name_after,delivery_before,delivery_after,price_before,price_after,sale_id,notes) SELECT p.id,?,?,?,0,p.quantity,'',p.name,0,p.delivery_price,0,p.sale_price,0,? FROM products p WHERE NOT EXISTS(SELECT 1 FROM product_history h WHERE h.product_id=p.id)");
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(tr("Система")); query.addBindValue(tr("Начална наличност"));
    query.addBindValue(tr("Начално състояние при създаване на историята."));
    if (!query.exec()) { const QString error = query.lastError().text(); db.rollback(); return error; }
    query.prepare("INSERT INTO product_history(product_id,created_at,actor_name,action,quantity_before,quantity_after,name_before,name_after,delivery_before,delivery_after,price_before,price_after,sale_id,notes) SELECT i.product_id,s.created_at,u.full_name,?,-1,-1,i.product_name,i.product_name,i.delivery_price,i.delivery_price,i.unit_price,i.unit_price,s.id,?||CAST(i.quantity AS TEXT)||? FROM sale_items i JOIN sales s ON s.id=i.sale_id JOIN users u ON u.id=s.user_id WHERE NOT EXISTS(SELECT 1 FROM product_history h WHERE h.product_id=i.product_id AND h.sale_id=s.id)");
    query.addBindValue(tr("Архивна продажба")); query.addBindValue(tr("Продадени "));
    query.addBindValue(tr(" бр. Наличностите преди и след продажбата не са записани."));
    if (!query.exec()) { const QString error = query.lastError().text(); db.rollback(); return error; }
    if (!db.commit()) { const QString error = db.lastError().text(); db.rollback(); return error; }
    return {};
}
