#ifndef DATABASE_H
#define DATABASE_H
#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include "persiandate.h"

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

    // Create a new file number
    // Format: YYYY + TT + SSSSS
    // YYYY = Solar Year | TT = animal_type_id (01, 02, ...) | SSSSS = 5-digit sequential number
    static QString generateFileNumber(int animalTypeId) {
        // Solar year from PersianDate
        int jy, jm, jd;
        PersianDate::toJalali(QDate::currentDate(), jy, jm, jd);

        QString typeCode = QString("%1").arg(animalTypeId, 2, 10, QChar('0'));
        QString yearStr  = QString::number(jy);

        // Finding the largest ordinal number this year for this type
        QSqlQuery q;
        q.prepare(
            "SELECT MAX(CAST(SUBSTRING(file_number, 7, 5) AS UNSIGNED)) AS max_seq "
            "FROM animals "
            "WHERE SUBSTRING(file_number, 1, 4) = :year "
            "  AND SUBSTRING(file_number, 5, 2) = :type "
            "  AND is_deleted = FALSE"
            );
        q.bindValue(":year", yearStr);
        q.bindValue(":type", typeCode);
        q.exec();

        int nextSeq = 1;
        if (q.next() && !q.value("max_seq").isNull()) {
            nextSeq = q.value("max_seq").toInt() + 1;
        }

        return QString("%1%2%3")
            .arg(yearStr)
            .arg(typeCode)
            .arg(nextSeq, 5, 10, QChar('0'));
    }

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase m_db;
};
#endif // DATABASE_H