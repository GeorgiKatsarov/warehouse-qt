#include "Database.h"
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>

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
    if (seedUsers) {
        const QStringList roles = {"admin", "manager", "cashier"};
        const QStringList names = {
            QCoreApplication::translate("Database", "Администратор"),
            QCoreApplication::translate("Database", "Управител на склада"),
            QCoreApplication::translate("Database", "Касиер")
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
            QCoreApplication::translate("Database", "Молив"),
            QCoreApplication::translate("Database", "Химикал"),
            QCoreApplication::translate("Database", "Тетрадка"),
            QCoreApplication::translate("Database", "Папка"),
            QCoreApplication::translate("Database", "Гума"),
            QCoreApplication::translate("Database", "Линия"),
            QCoreApplication::translate("Database", "Маркер"),
            QCoreApplication::translate("Database", "Лепило"),
            QCoreApplication::translate("Database", "Ножица"),
            QCoreApplication::translate("Database", "Копирна хартия")
        };
        for (int i = 0; i < names.size(); ++i) {
            query.prepare("INSERT INTO products(name,sku,quantity,supplier,delivery_price,sale_price) VALUES(?,?,?,?,?,?)");
            query.addBindValue(names.at(i)); query.addBindValue(QString("P%1").arg(i+1,3,10,QChar('0')));
            query.addBindValue(100);
            query.addBindValue(QCoreApplication::translate("Database", "Демо доставчик"));
            query.addBindValue(50 + i*30); query.addBindValue(100 + i*50);
            if (!query.exec()) { const QString error = query.lastError().text(); db.rollback(); return error; }
        }
    }
    if (!db.commit()) { const QString error = db.lastError().text(); db.rollback(); return error; }
    return {};
}
