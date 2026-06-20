#ifndef DATABASE_H
#define DATABASE_H
#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QList>
#include <QPair>
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

    // ─────────────────────────────────────────────────────────────────────
    // منطق وضعیت واکسن (is_renewed) — تک‌نقطه‌ی تغییر برای کل پروژه
    //
    // قانون: به ازای هر (animal_id, vaccine_type_id)، در میان رکوردهای
    // مرتب‌شده بر اساس تاریخ تزریق (قدیم→جدید)، فقط آخرین (جدیدترین)
    // رکورد باید is_renewed=0 باشد؛ همه‌ی رکوردهای قبل از آن (چه دقیقاً
    // قبلی، چه چند رکورد قبل‌تر) باید is_renewed=1 باشند — چون منطقاً
    // یک رکورد جدیدتر از خودشان در همان زنجیره وجود دارد.
    //
    // این قانون مستقل از این است که insert/edit/delete در کجای زنجیره
    // (ابتدا، وسط، یا انتها) رخ داده باشد. به همین دلیل، ساده‌ترین و
    // قابل‌اعتمادترین پیاده‌سازی این است که بعد از هر تغییر، کل گروه
    // (فقط همان animal_id+vaccine_type_id، نه کل دیتابیس) از نو
    // بازآرایی شود — نه این‌که فقط "مرز" حدس زده شود.
    //
    // این سه تابع عمومی، نقاط ورودی هستند که باید بعد از هر
    // Add / Edit / Delete واکسن صدا زده شوند.
    // ─────────────────────────────────────────────────────────────────────

    // بعد از INSERT یک واکسن جدید صدا زده می‌شود.
    static void onVaccinationAdded(int animalId, int vaccineTypeId, int /*newVaccinationId*/)
    {
        recalculateGroup(animalId, vaccineTypeId);
    }

    // بعد از UPDATE یک واکسن موجود صدا زده می‌شود.
    // پارامترهای old* باید قبل از اجرای UPDATE از دیتابیس خوانده شده باشند.
    static void onVaccinationEdited(int /*vaccinationId*/,
                                    int oldAnimalId, int oldVaccineTypeId,
                                    int newAnimalId, int newVaccineTypeId)
    {
        bool groupChanged = (oldAnimalId != newAnimalId) || (oldVaccineTypeId != newVaccineTypeId);

        if (groupChanged) {
            // این رکورد از گروه قدیمی خارج شده (نوع واکسن عوض شده)؛
            // گروه قدیمی باید بدون این رکورد بازآرایی شود.
            recalculateGroup(oldAnimalId, oldVaccineTypeId);
        }

        // گروه جدید (که این رکورد الان عضوش است) باید بازآرایی شود.
        // اگر گروه عوض نشده باشد، oldGroup == newGroup و همین یک
        // فراخوانی برای جابه‌جایی تاریخ هم کافی است.
        recalculateGroup(newAnimalId, newVaccineTypeId);
    }

    // قبل از DELETE باید animal_id و vaccine_type_id رکورد خوانده شده
    // باشند (چون بعد از حذف دیگر در دسترس نیستند). این تابع بعد از
    // اجرای موفق DELETE صدا زده می‌شود.
    static void onVaccinationDeleted(int animalId, int vaccineTypeId, bool /*wasRenewed*/)
    {
        // صرف‌نظر از این‌که رکورد حذف‌شده جدیدترین بود یا نه، بازآرایی
        // کامل گروه همیشه نتیجه‌ی درست می‌دهد (و چون رکورد دیگر در
        // دیتابیس وجود ندارد، خودش را شامل نمی‌شود).
        recalculateGroup(animalId, vaccineTypeId);
    }

    // ─────────────────────────────────────────────────────────────────────
    // بررسی یکتایی تاریخ تزریق در یک گروه — قبل از INSERT/UPDATE باید
    // صدا زده شود تا از ثبت دو واکسن همان نوع برای همان حیوان در یک
    // روز جلوگیری شود.
    //
    // excludeVacId: در حالت ویرایش، id خود رکورد را پاس بده تا با خودش
    // تداخل نداشته باشد. در حالت افزودن، -1 پاس بده.
    // ─────────────────────────────────────────────────────────────────────
    static bool vaccinationDateConflictExists(int animalId, int vaccineTypeId,
                                              const QString& vaccinatedAtYmd,
                                              int excludeVacId)
    {
        QSqlQuery q;
        q.prepare(
            "SELECT id FROM vaccinations "
            "WHERE animal_id = :aid AND vaccine_type_id = :vtid "
            "  AND vaccinated_at = :vat AND id != :exId "
            "LIMIT 1");
        q.bindValue(":aid",  animalId);
        q.bindValue(":vtid", vaccineTypeId);
        q.bindValue(":vat",  vaccinatedAtYmd);
        q.bindValue(":exId", excludeVacId);
        q.exec();
        return q.next();
    }

private:
    // بازآرایی کامل یک گروه (animal_id, vaccine_type_id): رکوردها بر
    // اساس تاریخ تزریق (و در صورت تساوی، id) مرتب می‌شوند؛ فقط آخرین
    // (جدیدترین) رکورد is_renewed=0 می‌شود، بقیه is_renewed=1.
    // reminder_followups متناظر هرکدام هم همگام می‌شود: رکوردهای
    // renewed=1 → resolved=TRUE، رکورد renewed=0 → resolved=FALSE
    // (چون شاید قبلاً resolved بوده و الان باید دوباره فعال شود).
    static void recalculateGroup(int animalId, int vaccineTypeId)
    {
        QSqlQuery q;
        q.prepare(
            "SELECT id, is_renewed FROM vaccinations "
            "WHERE animal_id = :aid AND vaccine_type_id = :vtid "
            "ORDER BY vaccinated_at ASC, id ASC");
        q.bindValue(":aid",  animalId);
        q.bindValue(":vtid", vaccineTypeId);
        q.exec();

        QList<QPair<int,bool>> rows; // id, currentIsRenewed
        while (q.next())
            rows.append({q.value("id").toInt(), q.value("is_renewed").toBool()});

        if (rows.isEmpty()) return; // گروه خالی شده (مثلاً آخرین رکورد حذف شد)

        for (int i = 0; i < rows.size(); ++i) {
            int  vacId           = rows[i].first;
            bool currentRenewed  = rows[i].second;
            bool shouldBeRenewed = (i != rows.size() - 1); // همه به‌جز آخری

            if (currentRenewed == shouldBeRenewed)
                continue; // وضعیتش از قبل درست است، دست نمی‌زنیم

            QSqlQuery upd;
            upd.prepare("UPDATE vaccinations SET is_renewed = :r WHERE id = :id");
            upd.bindValue(":r",  shouldBeRenewed);
            upd.bindValue(":id", vacId);
            upd.exec();

            QSqlQuery rf;
            if (shouldBeRenewed) {
                rf.prepare(
                    "UPDATE reminder_followups SET is_resolved = TRUE, followed_up_at = NOW() "
                    "WHERE vaccination_id = :id AND is_resolved = FALSE");
            } else {
                rf.prepare(
                    "UPDATE reminder_followups SET is_resolved = FALSE, followed_up_at = NULL "
                    "WHERE vaccination_id = :id");
            }
            rf.bindValue(":id", vacId);
            rf.exec();
        }
    }

public:
    // ─────────────────────────────────────────────────────────────────────
    // Common SQL statement to calculate vaccine display status — on all pages
    // (vaccinationswidget, animalswidget, reminderswidget) the same string
    // is used so that the display logic is never written in two different places.
    //
    // Prerequisite: The query must have an alias v for the vaccinations table.
    // ─────────────────────────────────────────────────────────────────────
    static QString vaccinationStatusCase()
    {
        return
            "CASE "
            "  WHEN v.is_renewed = 1 THEN 'renewed' "
            "  WHEN v.next_reminder_at < CURDATE() THEN 'overdue' "
            "  WHEN v.next_reminder_at = CURDATE() THEN 'due' "
            "  ELSE 'pending' "
            "END";
    }

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase m_db;
};
#endif // DATABASE_H