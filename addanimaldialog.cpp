#include "addanimaldialog.h"
#include "ui_addanimaldialog.h"
#include "styledmessagebox.h"
#include <QSqlQuery>
#include <QMessageBox>
#include <QDate>

AddAnimalDialog::AddAnimalDialog(int ownerId, const QString& ownerPhone, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddAnimalDialog), m_ownerId(ownerId)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);

    // Pre-fill owner info
    QSqlQuery q;
    q.prepare("SELECT first_name, last_name FROM owners WHERE id = :id");
    q.bindValue(":id", ownerId);
    q.exec();
    QString ownerName = q.next()
                            ? q.value("first_name").toString() + " " + q.value("last_name").toString()
                            : "";
    ui->ownerInfoLabel->setText(ownerName + " | " + ownerPhone);

    ui->birthDateEdit->setDate(QDate::currentDate().addYears(-1));

    applyStyle();
    connect(ui->btnSave,   &QPushButton::clicked, this, &AddAnimalDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddAnimalDialog::~AddAnimalDialog() { delete ui; }

void AddAnimalDialog::applyStyle()
{
    setStyleSheet(R"(
        QDialog { background: #F1F8E9; }
        QWidget#headerWidget { background: #2E7D32; }
        QLabel#dialogTitle { color: white; font-size: 15px; font-weight: 500; background: transparent; }
        QWidget#formWidget, QWidget#footerWidget { background: white; }
        QLabel { font-size: 13px; color: #5F5E5A; }
        QLineEdit, QComboBox, QDoubleSpinBox, QDateEdit {
            border: 1px solid #A5D6A7;
            border-radius: 6px;
            padding: 7px 10px;
            font-size: 13px;
            background: #F9FBF9;
            color: #212121;
            min-height: 36px;
        }
        QLineEdit:focus, QComboBox:focus { border: 1px solid #2E7D32; background: white; }
        QPushButton#btnSave {
            background: #2E7D32; color: white; border: none;
            border-radius: 6px; padding: 8px 20px; font-size: 13px;
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

void AddAnimalDialog::onSaveClicked()
{
    QString name = ui->animalNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "خطا", "لطفاً نام حیوان را وارد کنید.");
        return;
    }

    QString type   = (ui->typeCombo->currentIndex() == 0) ? "dog" : "cat";
    QString gender = (ui->genderCombo->currentIndex() == 0) ? "male" : "female";

    QSqlQuery q;
    q.prepare("INSERT INTO animals (name, type, breed, birth_date, gender, weight, owner_id) "
              "VALUES (:name, :type, :breed, :bdate, :gender, :weight, :owner)");
    q.bindValue(":name",   name);
    q.bindValue(":type",   type);
    q.bindValue(":breed",  ui->breedEdit->text().trimmed());
    q.bindValue(":bdate",  ui->birthDateEdit->date().toString("yyyy-MM-dd"));
    q.bindValue(":gender", gender);
    q.bindValue(":weight", ui->weightSpin->value());
    q.bindValue(":owner",  m_ownerId);

    if (!q.exec()) {
        QMessageBox::critical(this, "خطا", "خطا در ثبت حیوان.");
        return;
    }

    m_savedAnimalId = q.lastInsertId().toInt();
    accept();
}