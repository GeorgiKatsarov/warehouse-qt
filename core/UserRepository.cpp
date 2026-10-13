#include "UserRepository.h"
#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QRegularExpression>

namespace {
std::shared_ptr<User> readUser(const QSqlQuery &query)
{
    const int id = query.value(0).toInt();
    const QString username = query.value(1).toString(), hash = query.value(2).toString(),
                  fullName = query.value(3).toString(), role = query.value(4).toString();
    if (role == "admin") return std::make_shared<Admin>(id,username,hash,fullName);
    if (role == "manager") return std::make_shared<WarehouseManager>(id,username,hash,fullName);
    if (role == "cashier") return std::make_shared<Cashier>(id,username,hash,fullName);
    return {};
}
}
std::vector<std::shared_ptr<User>> UserRepository::all(QString &error) const
{
    error.clear();
    std::vector<std::shared_ptr<User>> users;
    QSqlQuery query(m_database);
    if (!query.exec("SELECT id,username,password_hash,full_name,role FROM users ORDER BY username")) {
        error = query.lastError().text(); return users;
    }
    while (query.next()) users.push_back(readUser(query));
    return users;
}
std::shared_ptr<User> UserRepository::login(QString username, QString password, QString &error) const
{
    error.clear();
    QSqlQuery query(m_database);
    query.prepare("SELECT id,username,password_hash,full_name,role FROM users WHERE username=? AND password_hash=?");
    query.addBindValue(username.trimmed()); query.addBindValue(hashPassword(password));
    if (!query.exec()) { error = query.lastError().text(); return {}; }
    if (!query.next()) {
        error = tr("Невалидно потребителско име или парола.");
        return {};
    }
    return readUser(query);
}
QString UserRepository::save(User &user)
{
    if (user.id()<0 || !QRegularExpression("^[A-Za-z0-9_.-]{3,64}$").match(user.username().trimmed()).hasMatch()
        || user.fullName().trimmed().isEmpty() || user.fullName().size()>160
        || !QRegularExpression("^[0-9a-f]{64}$").match(user.passwordHash()).hasMatch())
        return tr("Името е задължително (до 160 знака). Потребителското име трябва да е 3–64 латински букви, цифри, точка, тире или долна черта. Необходима е валидна парола.");
    QSqlQuery query(m_database);
    query.prepare(user.id() == 0
        ? "INSERT INTO users(username,password_hash,full_name,role) VALUES(?,?,?,?)"
        : "UPDATE users SET username=?,password_hash=?,full_name=?,role=? WHERE id=?");
    query.addBindValue(user.username().trimmed()); query.addBindValue(user.passwordHash());
    query.addBindValue(user.fullName().trimmed()); query.addBindValue(user.roleName());
    if (user.id() != 0) query.addBindValue(user.id());
    if (!query.exec()) return query.lastError().text();
    if (user.id() == 0) user.setId(query.lastInsertId().toInt());
    else if (query.numRowsAffected() != 1)
        return tr("Потребителят не е намерен.");
    return {};
}
QString UserRepository::remove(int id)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM users WHERE id=?"); query.addBindValue(id);
    if (!query.exec()) return query.lastError().text();
    if (query.numRowsAffected() != 1)
        return tr("Потребителят не е намерен.");
    return {};
}
std::shared_ptr<User> UserRepository::find(int id, QString &error) const
{
    error.clear(); QSqlQuery query(m_database);
    query.prepare("SELECT id,username,password_hash,full_name,role FROM users WHERE id=?"); query.addBindValue(id);
    if (!query.exec()) { error = query.lastError().text(); return {}; }
    if (!query.next()) { error = tr("Потребителят не е намерен."); return {}; }
    return readUser(query);
}
