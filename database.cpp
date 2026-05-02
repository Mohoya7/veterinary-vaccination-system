#include "Database.h"

Database& Database::instance()
{
    static Database instance;
    return instance;
}

bool Database::connect(const QString& host,
                       const QString& dbName,
                       const QString& username,
                       const QString& password,
                       int port)
{
    m_db = QSqlDatabase::addDatabase("QMYSQL");
    m_db.setHostName(host);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(username);
    m_db.setPassword(password);
    m_db.setPort(port);

    if (!m_db.open()) {
        qDebug() << "Database connection error:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "Database connected successfully.";
    return true;
}

bool Database::isConnected() const
{
    return m_db.isOpen();
}

QSqlDatabase& Database::db()
{
    return m_db;
}