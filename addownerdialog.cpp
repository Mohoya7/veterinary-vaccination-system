#include "addownerdialog.h"
#include "ui_addownerdialog.h"
#include "styledmessagebox.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QRegularExpressionValidator>

AddOwnerDialog::AddOwnerDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddOwnerDialog)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    if (auto* lo = qobject_cast<QHBoxLayout*>(ui->btnSave->parentWidget()->layout()))
        lo->setDirection(QBoxLayout::RightToLeft);
    applyStyle();

    auto* phoneValidator = new QRegularExpressionValidator(QRegularExpression("^[0-9]{0,11}$"), this);
    ui->phoneEdit->setValidator(phoneValidator);
    ui->phoneEdit->setMaxLength(11);

    auto* phone2Validator = new QRegularExpressionValidator(QRegularExpression("^[0-9]{0,11}$"), this);
    ui->phone2Edit->setValidator(phone2Validator);
    ui->phone2Edit->setMaxLength(11);

    connect(ui->btnSave,   &QPushButton::clicked, this, &AddOwnerDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddOwnerDialog::AddOwnerDialog(int ownerId, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddOwnerDialog), m_ownerId(ownerId)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    if (auto* lo = qobject_cast<QHBoxLayout*>(ui->btnSave->parentWidget()->layout()))
        lo->setDirection(QBoxLayout::RightToLeft);
    setWindowTitle("ویرایش اطلاعات صاحب");
    ui->dialogTitle->setText("ویرایش اطلاعات صاحب");
    applyStyle();

    auto* phoneValidator = new QRegularExpressionValidator(QRegularExpression("^[0-9]{0,11}$"), this);
    ui->phoneEdit->setValidator(phoneValidator);
    ui->phoneEdit->setMaxLength(11);

    auto* phone2Validator = new QRegularExpressionValidator(QRegularExpression("^[0-9]{0,11}$"), this);
    ui->phone2Edit->setValidator(phone2Validator);
    ui->phone2Edit->setMaxLength(11);

    loadOwnerData(ownerId);
    connect(ui->btnSave,   &QPushButton::clicked, this, &AddOwnerDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

AddOwnerDialog::~AddOwnerDialog() { delete ui; }

void AddOwnerDialog::applyStyle()
{
    setStyleSheet(R"(
        QDialog { background: #F1F8E9; }
        QWidget#headerWidget {
            background: #2E7D32;
            border-radius: 0px;
        }
        QLabel#dialogTitle {
            color: white;
            font-size: 15px;
            font-weight: 500;
            background: transparent;
        }
        QWidget#formWidget, QWidget#footerWidget { background: white; }
        QLabel {
            font-size: 13px;
            color: #5F5E5A;
        }
        QLineEdit {
            border: 1px solid #A5D6A7;
            border-radius: 6px;
            padding: 7px 10px;
            font-size: 13px;
            background: #F9FBF9;
            color: #212121;
            min-height: 36px;
        }
        QLineEdit:focus {
            border: 1px solid #2E7D32;
            background: white;
        }
        QComboBox {
            border: 1px solid #A5D6A7;
            border-radius: 6px;
            padding: 7px 10px;
            font-size: 13px;
            background: #F9FBF9;
            color: #212121;
            min-height: 36px;
        }
        QComboBox:focus {
            border: 1px solid #2E7D32;
            background: white;
        }
        QComboBox::drop-down { border: none; width: 24px; }
        QComboBox QAbstractItemView {
            background: white;
            border: 1px solid #A5D6A7;
            border-radius: 6px;
            selection-background-color: #E8F5E9;
            selection-color: #212121;
            font-size: 13px;
        }
        QPushButton#btnSave {
            background: #2E7D32;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 20px;
            font-size: 13px;
        }
        QPushButton#btnSave:hover { background: #1B5E20; }
        QPushButton#btnCancel {
            background: white;
            color: #757575;
            border: 0.5px solid #E0E0E0;
            border-radius: 6px;
            padding: 8px 20px;
            font-size: 13px;
        }
        QPushButton#btnCancel:hover { background: #F5F5F5; }
        QWidget#footerWidget {
            border-top: 0.5px solid #E0E0E0;
        }
    )");
}

void AddOwnerDialog::loadOwnerData(int ownerId)
{
    QSqlQuery q;
    q.prepare("SELECT first_name, last_name, gender, phone, phone_secondary, address, notes FROM owners WHERE id = :id");
    q.bindValue(":id", ownerId);
    q.exec();
    if (!q.next()) return;

    ui->firstNameEdit->setText(q.value("first_name").toString());
    ui->lastNameEdit->setText(q.value("last_name").toString());
    ui->phoneEdit->setText(q.value("phone").toString());
    ui->phone2Edit->setText(q.value("phone_secondary").toString());
    ui->addressEdit->setText(q.value("address").toString());
    ui->notesEdit->setText(q.value("notes").toString());

    ui->genderCombo->setCurrentIndex(q.value("gender").toString() == "مرد" ? 0 : 1);
}

void AddOwnerDialog::onSaveClicked()
{
    QString firstName = ui->firstNameEdit->text().trimmed();
    QString lastName  = ui->lastNameEdit->text().trimmed();
    QString phone     = ui->phoneEdit->text().trimmed();

    if (firstName.isEmpty() || lastName.isEmpty() || phone.isEmpty()) {
        StyledMessageBox::warning(this, "خطا", "لطفاً فیلدهای ستاره‌دار را پر کنید.");
        return;
    }

    if (phone.length() != 11) {
        StyledMessageBox::warning(this, "خطا", "شماره تلفن باید ۱۱ رقم باشد.");
        return;
    }

    QString phone2 = ui->phone2Edit->text().trimmed();
    if (!phone2.isEmpty() && phone2.length() != 11) {
        StyledMessageBox::warning(this, "خطا", "شماره تلفن دوم باید ۱۱ رقم باشد.");
        return;
    }

    QString gender = (ui->genderCombo->currentIndex() == 0) ? "مرد" : "زن";

    QSqlQuery q;

    if (m_ownerId < 0) {
        q.prepare("INSERT INTO owners (first_name, last_name, gender, phone, phone_secondary, address, notes) "
                  "VALUES (:fn, :ln, :gender, :p, :p2, :addr, :notes)");
    } else {
        q.prepare("UPDATE owners SET first_name=:fn, last_name=:ln, gender=:gender, phone=:p, "
                  "phone_secondary=:p2, address=:addr, notes=:notes, updated_at=NOW() "
                  "WHERE id = :id");
        q.bindValue(":id", m_ownerId);
    }

    q.bindValue(":fn",     firstName);
    q.bindValue(":ln",     lastName);
    q.bindValue(":gender", gender);
    q.bindValue(":p",      phone);
    q.bindValue(":p2",     phone2.isEmpty() ? QVariant() : phone2);
    q.bindValue(":addr",   ui->addressEdit->text().trimmed());
    q.bindValue(":notes",  ui->notesEdit->text().trimmed());

    if (!q.exec()) {
        if (q.lastError().nativeErrorCode() == "1062") {
            StyledMessageBox::warning(this, "خطا", "این شماره تلفن قبلاً برای صاحب دیگری ثبت شده است.");
        } else {
            StyledMessageBox::error(this, "خطا", "خطا در ذخیره اطلاعات:\n" + q.lastError().text());
        }
        return;
    }

    m_savedOwnerId = (m_ownerId < 0) ? q.lastInsertId().toInt() : m_ownerId;
    m_savedPhone   = phone;
    accept();
}