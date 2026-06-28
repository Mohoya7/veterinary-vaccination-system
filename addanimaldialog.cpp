#include "addanimaldialog.h"
#include "ui_addanimaldialog.h"
#include "styledmessagebox.h"
#include "database.h"
#include "animaltypeinfo.h"
#include "pagedirtytracker.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDate>
#include <QVBoxLayout>
#include <QSet>

// ── Add Mode ───────────────────────────────────────────────────────────────
AddAnimalDialog::AddAnimalDialog(int ownerId, const QString& ownerPhone, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddAnimalDialog), m_ownerId(ownerId)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    if (auto* lo = qobject_cast<QHBoxLayout*>(ui->btnSave->parentWidget()->layout()))
        lo->setDirection(QBoxLayout::RightToLeft);
    setWindowTitle("افزودن حیوان جدید");

    // Owner info
    QSqlQuery q;
    q.prepare("SELECT first_name, last_name FROM owners WHERE id = :id");
    q.bindValue(":id", ownerId);
    q.exec();
    QString ownerName = q.next()
                            ? q.value("first_name").toString() + " " + q.value("last_name").toString()
                            : "";
    ui->ownerInfoLabel->setText(ownerName + " | " + ownerPhone);

    // Load animal type combo from database
    loadTypeCombo();

    // Embed PersianDatePicker in container
    m_birthPicker = new PersianDatePicker(this);
    m_birthPicker->setDate(QDate::currentDate());
    auto* lay = new QVBoxLayout(ui->birthDateContainer);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_birthPicker);

    applyStyle();
    connect(ui->btnSave,   &QPushButton::clicked, this, &AddAnimalDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

// ── Edit Mode ───────────────────────────────────────────────────────────────
AddAnimalDialog::AddAnimalDialog(int ownerId, const QString& ownerPhone,
                                 int animalId, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddAnimalDialog),
    m_ownerId(ownerId), m_animalId(animalId)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    if (auto* lo = qobject_cast<QHBoxLayout*>(ui->btnSave->parentWidget()->layout()))
        lo->setDirection(QBoxLayout::RightToLeft);
    setWindowTitle("ویرایش اطلاعات حیوان");
    ui->dialogTitle->setText("ویرایش اطلاعات حیوان");

    // Owner info
    QSqlQuery q;
    q.prepare("SELECT first_name, last_name FROM owners WHERE id = :id");
    q.bindValue(":id", ownerId);
    q.exec();
    QString ownerName = q.next()
                            ? q.value("first_name").toString() + " " + q.value("last_name").toString()
                            : "";
    ui->ownerInfoLabel->setText(ownerName + " | " + ownerPhone);

    loadTypeCombo();

    // Embed PersianDatePicker
    m_birthPicker = new PersianDatePicker(this);
    auto* lay = new QVBoxLayout(ui->birthDateContainer);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_birthPicker);

    // Load existing data
    loadAnimalData(animalId);

    applyStyle();
    connect(ui->btnSave,   &QPushButton::clicked, this, &AddAnimalDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddAnimalDialog::~AddAnimalDialog() { delete ui; }

// ── loadTypeCombo ─────────────────────────────────────────────────────────────
void AddAnimalDialog::loadTypeCombo()
{
    ui->typeCombo->clear();
    QSqlQuery q;
    q.exec("SELECT id, name FROM animal_types ORDER BY id");
    while (q.next()) {
        ui->typeCombo->addItem(q.value("name").toString(),
                               q.value("id").toInt());
    }
}

// ── loadAnimalData (Edit mode) ─────────────────────────────────────────────
void AddAnimalDialog::loadAnimalData(int animalId)
{
    QSqlQuery q;
    q.prepare("SELECT name, animal_type_id, breed, birth_date, gender "
              "FROM animals WHERE id = :id");
    q.bindValue(":id", animalId);
    q.exec();
    if (!q.next()) return;

    ui->animalNameEdit->setText(q.value("name").toString());
    ui->breedEdit->setText(q.value("breed").toString());

    // Type
    int typeId = q.value("animal_type_id").toInt();
    m_originalAnimalTypeId = typeId;
    for (int i = 0; i < ui->typeCombo->count(); i++) {
        if (ui->typeCombo->itemData(i).toInt() == typeId) {
            ui->typeCombo->setCurrentIndex(i);
            break;
        }
    }

    // Birth date
    QDate bd = q.value("birth_date").toDate();
    if (bd.isValid())
        m_birthPicker->setDate(bd);
    else
        m_birthPicker->setDate(QDate::currentDate());

    // Gender
    ui->genderCombo->setCurrentIndex(
        q.value("gender").toString() == "male" ? 0 : 1);
}

// ── applyStyle ────────────────────────────────────────────────────────────────
void AddAnimalDialog::applyStyle()
{
    setStyleSheet(R"(
        QDialog { background: #F1F8E9; }
        QWidget#headerWidget { background: #2E7D32; }
        QLabel#dialogTitle {
            color: white; font-size: 15px; font-weight: 500; background: transparent;
        }
        QWidget#formWidget, QWidget#footerWidget { background: white; }
        QLabel { font-size: 13px; color: #5F5E5A; }
        QLineEdit, QComboBox {
            border: 1px solid #A5D6A7;
            border-radius: 6px;
            padding: 7px 10px;
            font-size: 13px;
            background: #F9FBF9;
            color: #212121;
            min-height: 36px;
        }
        QLineEdit:focus, QComboBox:focus {
            border: 1px solid #2E7D32; background: white;
        }
        QPushButton#btnSave {
            background: #2E7D32; color: white; border: none;
            border-radius: 6px; padding: 8px 20px; font-size: 13px;
        }
        QPushButton#btnSave:hover { background: #1B5E20; }
        QPushButton#btnCancel {
            background: white; color: #757575;
            border: 0.5px solid #E0E0E0; border-radius: 6px;
            padding: 8px 20px; font-size: 13px;
        }
        QPushButton#btnCancel:hover { background: #F5F5F5; }
        QWidget#footerWidget { border-top: 0.5px solid #E0E0E0; }
    )");
}

// ── onSaveClicked ─────────────────────────────────────────────────────────────
void AddAnimalDialog::onSaveClicked()
{
    QString name = ui->animalNameEdit->text().trimmed();
    if (name.isEmpty()) {
        StyledMessageBox::warning(this, "Error", "Please enter the animal's name.");
        return;
    }

    QDate birthDate = m_birthPicker->date();
    if (!birthDate.isValid()) {
        StyledMessageBox::warning(this, "Error", "Please select the birth date.");
        return;
    }
    if (birthDate > QDate::currentDate()) {
        StyledMessageBox::warning(this, "Error", "Birth date cannot be in the future.");
        return;
    }

    int     animalTypeId = ui->typeCombo->currentData().toInt();
    QString gender       = (ui->genderCombo->currentIndex() == 0) ? "male" : "female";

    QSqlQuery q;

    if (m_animalId < 0) {
        // ── Add Mode ──────────────────────────────────────────────────────
        QString fileNumber = Database::generateFileNumber(animalTypeId);

        q.prepare("INSERT INTO animals "
                  "(file_number, name, animal_type_id, breed, birth_date, gender, owner_id) "
                  "VALUES (:fnum, :name, :atid, :breed, :bdate, :gender, :owner)");
        q.bindValue(":fnum",  fileNumber);
        q.bindValue(":name",  name);
        q.bindValue(":atid",  animalTypeId);
        q.bindValue(":breed", ui->breedEdit->text().trimmed());
        q.bindValue(":bdate", birthDate.toString("yyyy-MM-dd"));
        q.bindValue(":gender", gender);
        q.bindValue(":owner", m_ownerId);

        if (!q.exec()) {
            StyledMessageBox::error(this, "Error", "Error in registering the animal:\n" + q.lastError().text());
            return;
        }
        m_savedAnimalId  = q.lastInsertId().toInt();
        m_savedFileNumber = fileNumber;

    } else {
        // ── Edit Mode ──────────────────────────────────────────────────────
        q.prepare("UPDATE animals SET "
                  "name=:name, animal_type_id=:atid, breed=:breed, "
                  "birth_date=:bdate, gender=:gender, updated_at=NOW() "
                  "WHERE id=:id");
        q.bindValue(":name",  name);
        q.bindValue(":atid",  animalTypeId);
        q.bindValue(":breed", ui->breedEdit->text().trimmed());
        q.bindValue(":bdate", birthDate.toString("yyyy-MM-dd"));
        q.bindValue(":gender", gender);
        q.bindValue(":id",    m_animalId);

        if (!q.exec()) {
            StyledMessageBox::error(this, "Error", "Error in updating the animal:\n" + q.lastError().text());
            return;
        }
        m_savedAnimalId = m_animalId;
    }

    // ── واکسن‌های ناهم‌خوان با نوع جدید (فقط در حالت ویرایش، وقتی نوع واقعاً عوض شده) ──
    // طبق تصمیم: اگر نوع حیوان عوض شود، واکسن‌هایی که برای نوع جدید تعریف
    // نشده‌اند همیشه حذف می‌شوند — فقط هشدار داده می‌شود، انصراف/نگه‌داشتن
    // وجود ندارد.
    if (m_animalId >= 0 && m_originalAnimalTypeId != -1 && m_originalAnimalTypeId != animalTypeId) {
        QSqlQuery cq;
        cq.prepare(
            "SELECT v.id, v.vaccine_type_id, vt.name "
            "FROM vaccinations v "
            "JOIN vaccine_types vt ON vt.id = v.vaccine_type_id "
            "WHERE v.animal_id = :aid "
            "AND v.vaccine_type_id NOT IN ("
            "  SELECT vaccine_type_id FROM vaccine_type_animals WHERE animal_type_id = :newType"
            ")");
        cq.bindValue(":aid", m_animalId);
        cq.bindValue(":newType", animalTypeId);
        cq.exec();

        QList<QPair<int,int>> toDelete; // id, vaccine_type_id
        QStringList incompatibleNames;
        while (cq.next()) {
            toDelete.append({cq.value("id").toInt(), cq.value("vaccine_type_id").toInt()});
            QString n = cq.value("name").toString();
            if (!incompatibleNames.contains(n))
                incompatibleNames << n;
        }

        if (!toDelete.isEmpty()) {
            StyledMessageBox::warning(
                this, "حذف واکسن‌های ناهم‌خوان",
                QString("این حیوان %1 رکورد واکسیناسیون از نوع‌هایی دارد که برای «%2» تعریف "
                        "نشده‌اند (%3).\nبا تأیید این پیام، نوع حیوان تغییر می‌کند و این رکوردها "
                        "حذف می‌شوند.")
                    .arg(toDelete.size())
                    .arg(ui->typeCombo->currentText())
                    .arg(incompatibleNames.join("، ")));

            {
                QSet<int> affectedVaccineTypes;
                for (const auto& pair : toDelete) {
                    QSqlQuery dq;
                    dq.prepare("DELETE FROM vaccinations WHERE id = :id");
                    dq.bindValue(":id", pair.first);
                    dq.exec();
                    affectedVaccineTypes.insert(pair.second);
                }
                // زنجیره‌ی is_renewed هر گروه را به‌روزرسانی می‌کنیم
                // (اگر گروه کاملاً خالی شده باشد، خودش بدون کاری برمی‌گردد)
                for (int vtId : affectedVaccineTypes)
                    Database::onVaccinationDeleted(m_animalId, vtId, false);
            }
        }
    }

    // Clear AnimalTypeInfo cache if type has changed
    AnimalTypeInfo::clearCache();

    // این صفحه (Animals) که خودش دیالوگ را باز کرده، خودش بلافاصله
    // showAnimalById/loadAnimals را صدا می‌زند؛ بقیه‌ی صفحات که اسم/نوع/
    // تعداد حیوان را نمایش می‌دهند، فقط هنگام بازدید بعدی رفرش می‌شوند.
    PageDirtyTracker::instance().markDirty(
        {AppPage::Dashboard, AppPage::Animals, AppPage::Owners,
         AppPage::Vaccinations, AppPage::Reminders});

    accept();
}