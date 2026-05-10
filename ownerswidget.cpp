#include "ownerswidget.h"
#include "ui_ownerswidget.h"
#include "addownerdialog.h"
#include "addanimaldialog.h"
#include "addvaccinedialog.h"

#include <QSqlQuery>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QPainter>

class OwnerItemWidget : public QWidget {
public:
    OwnerItemWidget(const QString& fullName, const QString& phone, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(56);
        setLayoutDirection(Qt::RightToLeft);

        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(14, 8, 14, 8);
        lay->setSpacing(10);

        // Initials avatar
        QString initials;
        QStringList parts = fullName.split(' ', Qt::SkipEmptyParts);
        if (!parts.isEmpty()) initials += parts.first().left(1);
        if (parts.size() > 1) initials += " " + parts.last().left(1);

        auto* avatar = new QLabel(initials);
        avatar->setFixedSize(34, 34);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(
            "background:#E8F5E9;color:#2E7D32;"
            "border-radius:17px;font-size:11px;font-weight:500;");

        auto* nameL = new QLabel(fullName);
        nameL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        nameL->setStyleSheet("font-size:13px;font-weight:500;color:#212121;background:transparent;");

        auto* phoneL = new QLabel(phone);
        phoneL->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        phoneL->setStyleSheet("font-size:11px;color:#757575;background:transparent;");

        // Order: avatar (right) | name (middle) | phone (left)
        lay->addWidget(avatar);
        lay->addWidget(nameL);
        lay->addStretch();
        lay->addWidget(phoneL);
    }
};

OwnersWidget::OwnersWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OwnersWidget)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);

    this->setStyleSheet(R"(
        QWidget { background: transparent; }

        QWidget#listPanel {
            background: white;
            border-left: 0.5px solid #E0E0E0;
        }
        QWidget#listHeader {
            background: white;
            border-bottom: 0.5px solid #E0E0E0;
        }
        QLabel#listTitle {
            font-size: 15px;
            font-weight: 500;
            color: #212121;
            background: transparent;
        }
        QLineEdit#searchEdit {
            background: #F5F5F5;
            border: 0.5px solid #E0E0E0;
            border-radius: 6px;
            padding: 6px 10px;
            font-size: 12px;
            color: #212121;
        }
        QListWidget#ownerListWidget {
            background: white;
            border: none;
            outline: none;
        }
        QListWidget#ownerListWidget::item {
            border-bottom: 0.5px solid #F5F5F5;
            padding: 0px;
        }
        QListWidget#ownerListWidget::item:selected {
            background: #E8F5E9;
            border-right: 3px solid #2E7D32;
        }
        QPushButton#btnAddOwner {
            background: #2E7D32;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 12px;
        }
        QPushButton#btnAddOwner:hover { background: #1B5E20; }

        QWidget#profileHeaderWidget { background: #2E7D32; }

        QLabel#ownerNameLabel {
            color: white;
            font-size: 15px;
            font-weight: 500;
            background: transparent;
        }
        QLabel#ownerSubLabel {
            color: rgba(255,255,255,0.7);
            font-size: 12px;
            background: transparent;
        }
        QPushButton#btnEdit {
            background: rgba(255,255,255,0.12);
            color: white;
            border: 0.5px solid rgba(255,255,255,0.3);
            border-radius: 6px;
            padding: 5px 14px;
            font-size: 12px;
        }
        QPushButton#btnEdit:hover { background: rgba(255,255,255,0.2); }

        QPushButton#btnDelete {
            background: rgba(255,100,100,0.15);
            color: #FFCDD2;
            border: 0.5px solid rgba(255,150,150,0.4);
            border-radius: 6px;
            padding: 5px 14px;
            font-size: 12px;
        }
        QPushButton#btnDelete:hover { background: rgba(255,100,100,0.25); }

        QScrollArea { background: transparent; border: none; }
        QWidget#profileScrollContent { background: transparent; }

        QWidget#contactCard, QWidget#animalsCard {
            background: white;
            border: 0.5px solid #E0E0E0;
            border-radius: 10px;
        }
        QLabel#contactCardTitle, QLabel#animalsCardTitle {
            color: #757575;
            font-size: 12px;
            font-weight: 500;
            background: transparent;
        }
        QLabel#placeholderLabel {
            color: #BDBDBD;
            font-size: 14px;
        }
    )");

    ui->profileWidget->hide();

    connect(ui->searchEdit,      &QLineEdit::textChanged,          this, &OwnersWidget::onSearchChanged);
    connect(ui->ownerListWidget, &QListWidget::currentItemChanged, this, &OwnersWidget::onOwnerSelected);
    connect(ui->btnAddOwner,     &QPushButton::clicked,            this, &OwnersWidget::onAddOwnerClicked);
    connect(ui->btnEdit,         &QPushButton::clicked,            this, &OwnersWidget::onEditOwnerClicked);
    connect(ui->btnDelete,       &QPushButton::clicked,            this, &OwnersWidget::onDeleteOwnerClicked);

    loadOwners();
}

OwnersWidget::~OwnersWidget() { delete ui; }

void OwnersWidget::loadData() { loadOwners(); }

void OwnersWidget::onSearchChanged(const QString& text)
{
    loadOwners(text.trimmed());
}

void OwnersWidget::loadOwners(const QString& filter)
{
    ui->ownerListWidget->clear();

    QSqlQuery q;
    if (filter.isEmpty()) {
        q.prepare("SELECT id, first_name, last_name, phone FROM owners "
                  "WHERE is_deleted = FALSE ORDER BY first_name");
    } else {
        q.prepare("SELECT id, first_name, last_name, phone FROM owners "
                  "WHERE is_deleted = FALSE AND ("
                  "CONCAT(first_name,' ',last_name) LIKE :f OR phone LIKE :f2) "
                  "ORDER BY first_name");
        q.bindValue(":f",  "%" + filter + "%");
        q.bindValue(":f2", "%" + filter + "%");
    }
    q.exec();

    while (q.next()) {
        int     id    = q.value("id").toInt();
        QString name  = q.value("first_name").toString() + " " + q.value("last_name").toString();
        QString phone = q.value("phone").toString();

        auto* item = new QListWidgetItem(ui->ownerListWidget);
        item->setData(Qt::UserRole, id);
        item->setSizeHint(QSize(0, 56));

        auto* widget = new OwnerItemWidget(name, phone, ui->ownerListWidget);
        ui->ownerListWidget->setItemWidget(item, widget);

        if (id == m_selectedOwnerId)
            ui->ownerListWidget->setCurrentItem(item);
    }
}

void OwnersWidget::onOwnerSelected(QListWidgetItem* current, QListWidgetItem*)
{
    if (!current) { clearProfile(); return; }
    showOwnerProfile(current->data(Qt::UserRole).toInt());
}

void OwnersWidget::showOwnerProfile(int ownerId)
{
    m_selectedOwnerId = ownerId;

    QSqlQuery q;
    q.prepare("SELECT first_name, last_name, phone, phone_secondary, address, notes "
              "FROM owners WHERE id = :id");
    q.bindValue(":id", ownerId);
    q.exec();
    if (!q.next()) return;

    QString fullName = q.value("first_name").toString() + " " + q.value("last_name").toString();

    // Avatar initials
    QString initials;
    QStringList parts = fullName.split(' ', Qt::SkipEmptyParts);
    if (!parts.isEmpty()) initials += parts.first().left(1);
    if (parts.size() > 1) initials += " " + parts.last().left(1);

    ui->ownerAvatarLabel->setText(initials);
    ui->ownerAvatarLabel->setStyleSheet(
        "background:rgba(255,255,255,0.18);color:white;"
        "border-radius:25px;font-size:16px;font-weight:500;");
    ui->ownerNameLabel->setText(fullName);

    // Animal count
    QSqlQuery countQ;
    countQ.prepare("SELECT COUNT(*) FROM animals WHERE owner_id = :id AND is_deleted = FALSE");
    countQ.bindValue(":id", ownerId);
    countQ.exec();
    int animalCount = countQ.next() ? countQ.value(0).toInt() : 0;
    ui->ownerSubLabel->setText(QString::number(animalCount) + " حیوان ثبت شده");

    // Contact rows
    clearContactRows();
    addContactRow("شماره اول", q.value("phone").toString());
    if (!q.value("phone_secondary").toString().isEmpty())
        addContactRow("شماره دوم", q.value("phone_secondary").toString());
    if (!q.value("address").toString().isEmpty())
        addContactRow("آدرس", q.value("address").toString());
    if (!q.value("notes").toString().isEmpty())
        addContactRow("یادداشت", q.value("notes").toString());

    loadAnimals(ownerId);

    ui->placeholderWidget->hide();
    ui->profileWidget->show();
}

void OwnersWidget::clearProfile()
{
    m_selectedOwnerId = -1;
    ui->profileWidget->hide();
    ui->placeholderWidget->show();
}

void OwnersWidget::clearContactRows()
{
    QLayout* lay = ui->contactRowsContainer->layout();
    while (lay->count() > 0) {
        QLayoutItem* item = lay->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}

void OwnersWidget::addContactRow(const QString& label, const QString& value)
{
    auto* row = new QWidget;
    row->setStyleSheet("background: transparent;");
    auto* rowLay = new QHBoxLayout(row);
    rowLay->setContentsMargins(0, 7, 0, 7);
    rowLay->setSpacing(8);

    // Label on right, value on left (RTL layout)
    auto* labelL = new QLabel(label);
    labelL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    labelL->setStyleSheet("font-size:12px;color:#757575;background:transparent;");

    auto* valueL = new QLabel(value);
    valueL->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    valueL->setStyleSheet("font-size:12px;font-weight:500;color:#212121;background:transparent;");

    rowLay->addWidget(labelL);
    rowLay->addStretch();
    rowLay->addWidget(valueL);

    auto* divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: #F0F0F0;");

    ui->contactRowsContainer->layout()->addWidget(row);
    ui->contactRowsContainer->layout()->addWidget(divider);
}

void OwnersWidget::clearAnimals()
{
    QLayout* lay = ui->animalsContainer->layout();
    while (lay->count() > 0) {
        QLayoutItem* item = lay->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}

void OwnersWidget::loadAnimals(int ownerId)
{
    clearAnimals();

    QSqlQuery q;
    q.prepare("SELECT id, name, type FROM animals WHERE owner_id = :id AND is_deleted = FALSE");
    q.bindValue(":id", ownerId);
    q.exec();

    auto* gridLay = qobject_cast<QGridLayout*>(ui->animalsContainer->layout());
    int col = 0, row = 0;

    while (q.next()) {
        int     animalId = q.value("id").toInt();
        QString name     = q.value("name").toString();
        bool    isDog    = (q.value("type").toString() == "dog");
        QString typeStr  = isDog ? "سگ" : "گربه";

        auto* chip = new QPushButton;
        chip->setLayoutDirection(Qt::LeftToRight);
        chip->setFixedHeight(38);
        chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        chip->setText(name + "  |  " + typeStr);
        chip->setStyleSheet(
            "QPushButton {"
            "  background: #F1F8E9;"
            "  border: 0.5px solid #E0E0E0;"
            "  border-radius: 6px;"
            "  font-size: 12px;"
            "  font-weight: 500;"
            "  color: #212121;"
            "  text-align: left;"
            "  padding: 0 10px;"
            "}"
            "QPushButton:hover { border-color: #2E7D32; }"
            );

        connect(chip, &QPushButton::clicked, this, [this, animalId]() {
            emit navigateToAnimal(animalId);
        });

        gridLay->addWidget(chip, row, col);
        col++;
        if (col > 1) { col = 0; row++; }
    }

    if (gridLay->count() == 0) {
        auto* emptyL = new QLabel("حیوانی ثبت نشده");
        emptyL->setAlignment(Qt::AlignCenter);
        emptyL->setStyleSheet("color:#BDBDBD;font-size:12px;");
        gridLay->addWidget(emptyL, 0, 0, 1, 2);
    }
}

void OwnersWidget::onAddOwnerClicked()
{
    AddOwnerDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    int     newOwnerId  = dlg.savedOwnerId();
    QString ownerPhone  = dlg.savedPhone();

    loadOwners();

    for (int i = 0; i < ui->ownerListWidget->count(); i++) {
        if (ui->ownerListWidget->item(i)->data(Qt::UserRole).toInt() == newOwnerId) {
            ui->ownerListWidget->setCurrentRow(i);
            break;
        }
    }

    auto reply = QMessageBox::question(this, "افزودن حیوان",
                                       "آیا می‌خواهید برای این صاحب حیوان اضافه کنید؟",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    AddAnimalDialog animalDlg(newOwnerId, ownerPhone, this);
    if (animalDlg.exec() != QDialog::Accepted) {
        showOwnerProfile(newOwnerId);
        return;
    }

    int newAnimalId = animalDlg.savedAnimalId();
    showOwnerProfile(newOwnerId);

    auto reply2 = QMessageBox::question(this, "افزودن واکسن",
                                        "آیا می‌خواهید برای این حیوان واکسن اضافه کنید؟",
                                        QMessageBox::Yes | QMessageBox::No);
    if (reply2 != QMessageBox::Yes) return;

    AddVaccineDialog vaccineDlg(newAnimalId, this);
    vaccineDlg.exec();
}

void OwnersWidget::onEditOwnerClicked()
{
    if (m_selectedOwnerId < 0) return;
    AddOwnerDialog dlg(m_selectedOwnerId, this);
    if (dlg.exec() != QDialog::Accepted) return;
    loadOwners();
    showOwnerProfile(m_selectedOwnerId);
}

void OwnersWidget::onDeleteOwnerClicked()
{
    if (m_selectedOwnerId < 0) return;

    QSqlQuery countQ;
    countQ.prepare("SELECT COUNT(*) FROM animals WHERE owner_id = :id AND is_deleted = FALSE");
    countQ.bindValue(":id", m_selectedOwnerId);
    countQ.exec();
    int animalCount = countQ.next() ? countQ.value(0).toInt() : 0;

    QString msg = "آیا مطمئن هستید؟";
    if (animalCount > 0)
        msg += QString("\n\nاین صاحب دارای %1 حیوان ثبت شده است که همراه با واکسن‌هایشان حذف خواهند شد.").arg(animalCount);

    if (QMessageBox::warning(this, "حذف صاحب", msg,
                             QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    QSqlQuery q;
    q.prepare("UPDATE reminder_followups rf "
              "JOIN vaccinations v ON rf.vaccination_id = v.id "
              "JOIN animals a ON v.animal_id = a.id "
              "SET rf.is_resolved = TRUE WHERE a.owner_id = :id");
    q.bindValue(":id", m_selectedOwnerId); q.exec();

    q.prepare("UPDATE vaccinations v JOIN animals a ON v.animal_id = a.id "
              "SET v.is_deleted = TRUE WHERE a.owner_id = :id");
    q.bindValue(":id", m_selectedOwnerId); q.exec();

    q.prepare("UPDATE animals SET is_deleted = TRUE WHERE owner_id = :id");
    q.bindValue(":id", m_selectedOwnerId); q.exec();

    q.prepare("UPDATE owners SET is_deleted = TRUE WHERE id = :id");
    q.bindValue(":id", m_selectedOwnerId); q.exec();

    clearProfile();
    loadOwners();
}