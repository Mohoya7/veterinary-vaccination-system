#include "addownerdialog.h"
#include "ui_addownerdialog.h"
#include "styledmessagebox.h"
#include <QSqlQuery>
#include <QMessageBox>

AddOwnerDialog::AddOwnerDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddOwnerDialog)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    if (auto* lo = qobject_cast<QHBoxLayout*>(ui->btnSave->parentWidget()->layout()))
        lo->setDirection(QBoxLayout::RightToLeft);
    applyStyle();
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
    q.prepare("SELECT first_name, last_name, phone, phone_secondary, address, notes FROM owners WHERE id = :id");
    q.bindValue(":id", ownerId);
    q.exec();
    if (!q.next()) return;

    ui->firstNameEdit->setText(q.value("first_name").toString());
    ui->lastNameEdit->setText(q.value("last_name").toString());
    ui->phoneEdit->setText(q.value("phone").toString());
    ui->phone2Edit->setText(q.value("phone_secondary").toString());
    ui->addressEdit->setText(q.value("address").toString());
    ui->notesEdit->setText(q.value("notes").toString());
}

void AddOwnerDialog::onSaveClicked()
{
    QString firstName = ui->firstNameEdit->text().trimmed();
    QString lastName  = ui->lastNameEdit->text().trimmed();
    QString phone     = ui->phoneEdit->text().trimmed();

    if (firstName.isEmpty() || lastName.isEmpty() || phone.isEmpty()) {
        QMessageBox::warning(this, "خطا", "لطفاً فیلدهای ستاره‌دار را پر کنید.");
        return;
    }

    QSqlQuery q;

    if (m_ownerId < 0) {
        // Insert
        q.prepare("INSERT INTO owners (first_name, last_name, phone, phone_secondary, address, notes) "
                  "VALUES (:fn, :ln, :p, :p2, :addr, :notes)");
    } else {
        // Update
        q.prepare("UPDATE owners SET first_name=:fn, last_name=:ln, phone=:p, "
                  "phone_secondary=:p2, address=:addr, notes=:notes, updated_at=NOW() "
                  "WHERE id = :id");
        q.bindValue(":id", m_ownerId);
    }

    q.bindValue(":fn",    firstName);
    q.bindValue(":ln",    lastName);
    q.bindValue(":p",     phone);
    q.bindValue(":p2",    ui->phone2Edit->text().trimmed());
    q.bindValue(":addr",  ui->addressEdit->text().trimmed());
    q.bindValue(":notes", ui->notesEdit->text().trimmed());

    if (!q.exec()) {
        QMessageBox::critical(this, "خطا", "خطا در ذخیره اطلاعات.");
        return;
    }

    m_savedOwnerId = (m_ownerId < 0) ? q.lastInsertId().toInt() : m_ownerId;
    m_savedPhone   = phone;
    accept();
}