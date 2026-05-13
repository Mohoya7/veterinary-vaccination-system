#include "addvaccinedialog.h"
#include "ui_addvaccinedialog.h"
#include <QSqlQuery>
#include <QMessageBox>
#include <QDate>

AddVaccineDialog::AddVaccineDialog(int animalId, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddVaccineDialog), m_animalId(animalId)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    setWindowTitle("افزودن واکسن");

    QSqlQuery q;
    q.prepare("SELECT a.name, a.type, CONCAT(o.first_name,' ',o.last_name) AS owner_name "
              "FROM animals a JOIN owners o ON a.owner_id = o.id WHERE a.id = :id");
    q.bindValue(":id", animalId);
    q.exec();
    if (q.next()) {
        m_animalType = q.value("type").toString();
        QString typeStr = (m_animalType == "dog") ? "سگ" : "گربه";
        ui->animalInfoLabel->setText(
            q.value("name").toString() + " | " + typeStr + " | " + q.value("owner_name").toString());
    }

    ui->vaccinatedAtEdit->setDate(QDate::currentDate());
    loadVaccineTypes();
    applyStyle();

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
    q.prepare("SELECT a.name, a.type, CONCAT(o.first_name,' ',o.last_name) AS owner_name "
              "FROM animals a JOIN owners o ON a.owner_id = o.id WHERE a.id = :id");
    q.bindValue(":id", animalId);
    q.exec();
    if (q.next()) {
        m_animalType = q.value("type").toString();
        QString typeStr = (m_animalType == "dog") ? "سگ" : "گربه";
        ui->animalInfoLabel->setText(
            q.value("name").toString() + " | " + typeStr + " | " + q.value("owner_name").toString());
    }

    loadVaccineTypes();
    loadExistingData(vaccinationId);
    applyStyle();

    connect(ui->vaccineTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddVaccineDialog::onVaccineTypeChanged);
    connect(ui->btnSave,   &QPushButton::clicked, this, &AddVaccineDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddVaccineDialog::~AddVaccineDialog() { delete ui; }

void AddVaccineDialog::loadVaccineTypes()
{
    QSqlQuery q;
    q.prepare("SELECT id, name, default_reminder_days FROM vaccine_types "
              "WHERE (animal_type = :type OR animal_type = 'both') AND is_active = TRUE");
    q.bindValue(":type", m_animalType);
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

    int typeId = q.value("vaccine_type_id").toInt();
    for (int i = 0; i < ui->vaccineTypeCombo->count(); i++) {
        if (ui->vaccineTypeCombo->itemData(i).toInt() == typeId) {
            ui->vaccineTypeCombo->setCurrentIndex(i);
            break;
        }
    }
    ui->vaccinatedAtEdit->setDate(q.value("vaccinated_at").toDate());
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
        QMessageBox::warning(this, "خطا", "نوع واکسنی موجود نیست.");
        return;
    }

    int   vaccineTypeId  = ui->vaccineTypeCombo->currentData().toInt();
    QDate vaccinatedAt   = ui->vaccinatedAtEdit->date();
    int   reminderDays   = ui->reminderDaysSpin->value();
    QDate nextReminder   = vaccinatedAt.addDays(reminderDays);

    QSqlQuery q;

    if (m_vaccinationId < 0) {
        // Insert
        q.prepare("INSERT INTO vaccinations "
                  "(animal_id, vaccine_type_id, vaccinated_at, reminder_days, next_reminder_at, notes) "
                  "VALUES (:animal, :vtype, :vat, :days, :next, :notes)");
        q.bindValue(":animal", m_animalId);
    } else {
        // Update
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
        QMessageBox::critical(this, "خطا", "خطا در ثبت واکسن.");
        return;
    }

    accept();
}