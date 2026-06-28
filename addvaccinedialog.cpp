#include "addvaccinedialog.h"
#include "ui_addvaccinedialog.h"
#include "persiandatepicker.h"
#include "styledmessagebox.h"
#include "database.h"
#include "pagedirtytracker.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDate>
#include <QVBoxLayout>

AddVaccineDialog::AddVaccineDialog(int animalId, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddVaccineDialog), m_animalId(animalId)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    setWindowTitle("افزودن واکسن");

    QSqlQuery q;
    q.prepare("SELECT a.name, at.id AS type_id, at.name AS type_name, "
              "CONCAT(o.first_name,' ',o.last_name) AS owner_name "
              "FROM animals a "
              "JOIN owners o ON a.owner_id = o.id "
              "JOIN animal_types at ON a.animal_type_id = at.id "
              "WHERE a.id = :id");
    q.bindValue(":id", animalId);
    q.exec();
    if (q.next()) {
        m_animalTypeId = q.value("type_id").toInt();
        QString typeStr = q.value("type_name").toString();
        ui->animalInfoLabel->setText(
            q.value("name").toString() + " | " + typeStr + " | " + q.value("owner_name").toString());
    }

    loadVaccineTypes();
    applyStyle();

    // Embed PersianDatePicker in container
    m_datePicker = new PersianDatePicker(this);
    m_datePicker->setDate(QDate::currentDate());
    ui->vaccinatedAtLayout->addWidget(m_datePicker);

    connect(ui->vaccineTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddVaccineDialog::onVaccineTypeChanged);
    connect(ui->btnSave,   &QPushButton::clicked, this, &AddVaccineDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddVaccineDialog::AddVaccineDialog(int animalId, int vaccinationId, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddVaccineDialog),
    m_animalId(animalId), m_vaccinationId(vaccinationId)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    setWindowTitle("ویرایش واکسن");
    ui->dialogTitle->setText("ویرایش واکسن");
    ui->btnSave->setText("ذخیره تغییرات");

    QSqlQuery q;
    q.prepare("SELECT a.name, at.id AS type_id, at.name AS type_name, "
              "CONCAT(o.first_name,' ',o.last_name) AS owner_name "
              "FROM animals a "
              "JOIN owners o ON a.owner_id = o.id "
              "JOIN animal_types at ON a.animal_type_id = at.id "
              "WHERE a.id = :id");
    q.bindValue(":id", animalId);
    q.exec();
    if (q.next()) {
        m_animalTypeId = q.value("type_id").toInt();
        QString typeStr = q.value("type_name").toString();
        ui->animalInfoLabel->setText(
            q.value("name").toString() + " | " + typeStr + " | " + q.value("owner_name").toString());
    }

    loadVaccineTypes();
    applyStyle();

    // Embed PersianDatePicker in container
    m_datePicker = new PersianDatePicker(this);
    ui->vaccinatedAtLayout->addWidget(m_datePicker);

    // Load existing data — date is set directly on the picker
    loadExistingData(vaccinationId);

    connect(ui->vaccineTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddVaccineDialog::onVaccineTypeChanged);
    connect(ui->btnSave,   &QPushButton::clicked, this, &AddVaccineDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddVaccineDialog::~AddVaccineDialog() { delete ui; }

void AddVaccineDialog::prefillFrom(int vaccineTypeId, int reminderDays)
{
    for (int i = 0; i < ui->vaccineTypeCombo->count(); i++) {
        if (ui->vaccineTypeCombo->itemData(i).toInt() == vaccineTypeId) {
            ui->vaccineTypeCombo->setCurrentIndex(i);
            break;
        }
    }
    // مقدار واقعی رکورد قبلی را می‌گذاریم، نه پیش‌فرضِ خودِ نوع واکسن
    // (که onVaccineTypeChanged با تغییر ایندکس بالا ممکن است جای آن گذاشته باشد)
    ui->reminderDaysSpin->setValue(reminderDays);
}

void AddVaccineDialog::loadVaccineTypes()
{
    QSqlQuery q;
    q.prepare(
        "SELECT vt.id, vt.name, vt.default_reminder_days "
        "FROM vaccine_types vt "
        "JOIN vaccine_type_animals vta ON vta.vaccine_type_id = vt.id "
        "WHERE vta.animal_type_id = :atid "
        "ORDER BY vt.name");
    q.bindValue(":atid", m_animalTypeId);
    q.exec();

    while (q.next()) {
        int id   = q.value("id").toInt();
        int days = q.value("default_reminder_days").toInt();
        ui->vaccineTypeCombo->addItem(q.value("name").toString(), id);
        m_typeIdToDefaultDays[id] = days;
    }

    if (ui->vaccineTypeCombo->count() > 0) {
        int firstId = ui->vaccineTypeCombo->currentData().toInt();
        ui->reminderDaysSpin->setValue(m_typeIdToDefaultDays.value(firstId, 365));
    }
}

void AddVaccineDialog::loadExistingData(int vaccinationId)
{
    QSqlQuery q;
    q.prepare("SELECT vaccine_type_id, vaccinated_at, reminder_days, notes "
              "FROM vaccinations WHERE id = :id");
    q.bindValue(":id", vaccinationId);
    q.exec();
    if (!q.next()) return;

    // First read the date and set it on the picker — before anything else
    QDate vacDate = QDate::fromString(q.value("vaccinated_at").toString(), "yyyy-MM-dd");
    if (!vacDate.isValid())
        vacDate = q.value("vaccinated_at").toDate();
    if (m_datePicker && vacDate.isValid())
        m_datePicker->setDate(vacDate);

    int typeId = q.value("vaccine_type_id").toInt();
    m_oldVaccineTypeId = typeId; // برای استفاده در onVaccinationEdited بعد از ذخیره
    for (int i = 0; i < ui->vaccineTypeCombo->count(); i++) {
        if (ui->vaccineTypeCombo->itemData(i).toInt() == typeId) {
            ui->vaccineTypeCombo->setCurrentIndex(i);
            break;
        }
    }
    ui->reminderDaysSpin->setValue(q.value("reminder_days").toInt());
    ui->notesEdit->setText(q.value("notes").toString());
}

void AddVaccineDialog::onVaccineTypeChanged(int index)
{
    // Only auto-fill days in add mode
    if (m_vaccinationId >= 0) return;
    int typeId = ui->vaccineTypeCombo->itemData(index).toInt();
    ui->reminderDaysSpin->setValue(m_typeIdToDefaultDays.value(typeId, 365));
}

void AddVaccineDialog::applyStyle()
{
    setStyleSheet(R"(
        QDialog { background: #F1F8E9; }
        QWidget#headerWidget { background: #2E7D32; }
        QLabel#dialogTitle {
            color: white; font-size: 15px; font-weight: 500; background: transparent;
        }
        QWidget#formWidget, QWidget#footerWidget { background: white; }
        QLabel { font-size: 13px; color: #5F5E5A; }
        QLabel#animalInfoLabel {
            font-size: 12px;
            color: #2E7D32;
            font-weight: 500;
            background: #E8F5E9;
            border-radius: 6px;
            padding: 6px 10px;
            border: 0.5px solid #A5D6A7;
        }
        QLineEdit, QComboBox, QSpinBox, QDateEdit {
            border: 1px solid #A5D6A7;
            border-radius: 6px;
            padding: 7px 10px;
            font-size: 13px;
            background: #F9FBF9;
            color: #212121;
            min-height: 36px;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus {
            border: 1px solid #2E7D32;
            background: white;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox::down-arrow {
            width: 12px;
            height: 12px;
        }
        QComboBox QAbstractItemView {
            background: white;
            border: 0.5px solid #A5D6A7;
            border-radius: 6px;
            selection-background-color: #E8F5E9;
            selection-color: #212121;
            font-size: 13px;
            padding: 4px;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            border: none;
            width: 20px;
            background: transparent;
        }
        QPushButton#btnSave {
            background: #2E7D32; color: white; border: none;
            border-radius: 6px; padding: 8px 20px; font-size: 13px; font-weight: 500;
        }
        QPushButton#btnSave:hover { background: #1B5E20; }
        QPushButton#btnCancel {
            background: white; color: #757575;
            border: 0.5px solid #E0E0E0; border-radius: 6px; padding: 8px 20px; font-size: 13px;
        }
        QPushButton#btnCancel:hover { background: #F5F5F5; }
        QWidget#footerWidget { border-top: 0.5px solid #E0E0E0; }
    )");
}

void AddVaccineDialog::onSaveClicked()
{
    if (ui->vaccineTypeCombo->count() == 0) {
        StyledMessageBox::warning(this, "خطا", "هیچ نوع واکسنی موجود نیست.");
        return;
    }

    int   vaccineTypeId = ui->vaccineTypeCombo->currentData().toInt();
    QDate vaccinatedAt  = m_datePicker->date();

    if (!vaccinatedAt.isValid()) {
        StyledMessageBox::warning(this, "خطا", "لطفاً تاریخ تزریق را انتخاب کنید.");
        return;
    }
    if (vaccinatedAt > QDate::currentDate()) {
        StyledMessageBox::warning(this, "خطا", "تاریخ تزریق نمی‌تواند در آینده باشد.");
        return;
    }

    int   reminderDays = ui->reminderDaysSpin->value();
    QDate nextReminder = vaccinatedAt.addDays(reminderDays);

    // ── محدودیت یکتایی: همان حیوان + همان نوع واکسن نباید دو بار در
    // یک روز ثبت شود. در حالت ویرایش، خود رکورد فعلی از این چک مستثنا
    // می‌شود (excludeVacId) تا با خودش تداخل پیدا نکند.
    int excludeId = (m_vaccinationId < 0) ? -1 : m_vaccinationId;
    if (Database::vaccinationDateConflictExists(
            m_animalId, vaccineTypeId, vaccinatedAt.toString("yyyy-MM-dd"), excludeId)) {
        StyledMessageBox::warning(this, "خطا",
                                  "واکسنی از این نوع قبلاً برای این حیوان در همین تاریخ ثبت شده است.");
        return;
    }

    QSqlQuery q;

    if (m_vaccinationId < 0) {
        q.prepare("INSERT INTO vaccinations "
                  "(animal_id, vaccine_type_id, vaccinated_at, reminder_days, next_reminder_at, notes) "
                  "VALUES (:animal, :vtype, :vat, :days, :next, :notes)");
        q.bindValue(":animal", m_animalId);
    } else {
        q.prepare("UPDATE vaccinations SET "
                  "vaccine_type_id=:vtype, vaccinated_at=:vat, "
                  "reminder_days=:days, next_reminder_at=:next, "
                  "notes=:notes, updated_at=NOW() WHERE id=:id");
        q.bindValue(":id", m_vaccinationId);
    }

    q.bindValue(":vtype", vaccineTypeId);
    q.bindValue(":vat",   vaccinatedAt.toString("yyyy-MM-dd"));
    q.bindValue(":days",  reminderDays);
    q.bindValue(":next",  nextReminder.toString("yyyy-MM-dd"));
    q.bindValue(":notes", ui->notesEdit->text().trimmed());

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا", "خطا در ثبت واکسن:\n" + q.lastError().text());
        return;
    }

    if (m_vaccinationId < 0) {
        // ── Add state: New vaccine registered ─
        // The new record itself is created with the default is_renewed=0 (database column).
        // It only needs to convert the previous boundary (if it existed) to is_renewed=1
        // and resolve its corresponding reminder.
        int newVacId = q.lastInsertId().toInt();
        Database::onVaccinationAdded(m_animalId, vaccineTypeId, newVacId);

    } else {
        // ── Edit mode: Existing vaccine edited ─
        // animal_id never changes in this dialog (always m_animalId),
        // but vaccine_type_id may have changed. m_oldVaccineTypeId
        // It was filled in before this UPDATE in loadExistingData.
        Database::onVaccinationEdited(
            m_vaccinationId,
            m_animalId, m_oldVaccineTypeId,
            m_animalId, vaccineTypeId);
    }

    // این یک نقطه، هم Add هم Edit واکسن را پوشش می‌دهد (هر دو از همینجا رد می‌شوند).
    // صفحه‌ای که خودش این دیالوگ را باز کرده (Animals یا Vaccinations) بلافاصله
    // خودش را رفرش می‌کند؛ بقیه فقط موقع بازدید بعدی.
    PageDirtyTracker::instance().markDirty(
        {AppPage::Dashboard, AppPage::Animals, AppPage::Vaccinations, AppPage::Reminders});

    accept();
}