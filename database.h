#ifndef DATABASE_H
#define DATABASE_H

#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

class Database
{
public:
    static Database& instance();

    bool connect(const QString& host,
                 const QString& dbName,
                 const QString& username,
                 const QString& password,
                 int port = 3306);

    bool isConnected() const;
    QSqlDatabase& db();

    static QString hashPassword(const QString& password) {
        return QString(QCryptographicHash::hash(
                           password.toUtf8(),
                           QCryptographicHash::Sha256
                           ).toHex());
    }

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase m_db;
};

#endif // DATABASE_H