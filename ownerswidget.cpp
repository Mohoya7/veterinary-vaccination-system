#include "ownerswidget.h"
#include "animaltypeinfo.h"
#include "ui_ownerswidget.h"
#include "addownerdialog.h"
#include "addanimaldialog.h"
#include "addvaccinedialog.h"
#include "styledmessagebox.h"
#include "session.h"
#include "pagedirtytracker.h"

#include <QSqlQuery>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollBar>
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

    // Hide delete button for non-admin users
    if (!Session::instance().isAdmin()) {
        ui->btnDelete->hide();
    }

    // Green scrollbar for owner list
    ui->ownerListWidget->verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical {
            background: #F5F5F5;
            width: 9px;
            border-radius: 3px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #A5D6A7;
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: #2E7D32; }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical { background: transparent; }
    )");

    // ── Debounce فیلد سرچ (300ms) ───────────────────────────────────────
    m_searchDebounce = new QTimer(this);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(300);
    connect(m_searchDebounce, &QTimer::timeout, this, [this]() {
        loadOwners(ui->searchEdit->text().trimmed());
    });
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        m_searchDebounce->start();
    });
    connect(ui->ownerListWidget, &QListWidget::currentItemChanged, this, &OwnersWidget::onOwnerSelected);
    connect(ui->btnAddOwner,     &QPushButton::clicked,            this, &OwnersWidget::onAddOwnerClicked);
    connect(ui->btnEdit,         &QPushButton::clicked,            this, &OwnersWidget::onEditOwnerClicked);
    connect(ui->btnDelete,       &QPushButton::clicked,            this, &OwnersWidget::onDeleteOwnerClicked);

    loadOwners();
}

OwnersWidget::~OwnersWidget() { delete ui; }

void OwnersWidget::loadData() { loadOwners(); }

// ─── Reload با حفظ فیلتر/سرچ/offset/پروفایل فعلی ────────────────────────
void OwnersWidget::reloadPreservingState()
{
    QString filter   = ui->searchEdit->text().trimmed();
    int savedOwnerId = m_selectedOwnerId; // باید قبل از clear() ذخیره شود؛ همان دلیل AnimalsWidget

    ui->ownerListWidget->clear();
    if (m_loadMoreBtn) {
        auto* lay = qobject_cast<QVBoxLayout*>(ui->listPanel->layout());
        if (lay) lay->removeWidget(m_loadMoreBtn);
        m_loadMoreBtn->deleteLater();
        m_loadMoreBtn = nullptr;
    }
    m_currentOffset = 0;
    appendOwners(filter, 0); // همیشه فقط صفحه‌ی اول (۵۰ تا)

    if (savedOwnerId > 0) {
        QSqlQuery q;
        q.prepare("SELECT id FROM owners WHERE id = :id");
        q.bindValue(":id", savedOwnerId);
        q.exec();
        if (q.next()) {
            showOwnerProfile(savedOwnerId);
        } else {
            clearProfile();
        }
    }
}

void OwnersWidget::showOwnerById(int ownerId)
{
    // اول با فیلتر/سرچ فعلی چک می‌کنیم — شاید با همون فیلتر هم دیده شود
    for (int i = 0; i < ui->ownerListWidget->count(); i++) {
        if (ui->ownerListWidget->item(i)->data(Qt::UserRole).toInt() == ownerId) {
            ui->ownerListWidget->setCurrentRow(i);
            return;
        }
    }

    // پیدا نشد → فیلتر/سرچ باید پاک شود (هم در UI هم واقعاً در کوئری)
    // چون صاحبی که لینک شدیم باید دیده شود. سیگنال‌ها موقتاً بلاک می‌شوند
    // چون خودمان همین پایین دستی ریلود می‌کنیم.
    ui->searchEdit->blockSignals(true);
    ui->searchEdit->clear();
    ui->searchEdit->blockSignals(false);

    ui->ownerListWidget->clear();
    if (m_loadMoreBtn) {
        auto* lay = qobject_cast<QVBoxLayout*>(ui->listPanel->layout());
        if (lay) lay->removeWidget(m_loadMoreBtn);
        m_loadMoreBtn->deleteLater();
        m_loadMoreBtn = nullptr;
    }
    m_currentOffset = 0;

    // تا جایی که لازم است صفحه‌بندی را جلو می‌بریم تا صاحب پیدا شود
    bool found = false;
    for (int page = 0; page < 200 && !found; ++page) {
        int before = m_currentOffset;
        appendOwners("", m_currentOffset);
        for (int i = 0; i < ui->ownerListWidget->count(); i++) {
            if (ui->ownerListWidget->item(i)->data(Qt::UserRole).toInt() == ownerId) {
                ui->ownerListWidget->setCurrentRow(i);
                found = true;
                break;
            }
        }
        if (m_currentOffset == before) break;
        if (!m_loadMoreBtn) break;
    }

    if (!found)
        showOwnerProfile(ownerId);
}

void OwnersWidget::loadOwners(const QString& filter)
{
    m_currentOffset = 0;
    ui->ownerListWidget->clear();

    if (m_loadMoreBtn) {
        auto* lay = qobject_cast<QVBoxLayout*>(ui->listPanel->layout());
        if (lay) lay->removeWidget(m_loadMoreBtn);
        m_loadMoreBtn->deleteLater();
        m_loadMoreBtn = nullptr;
    }

    appendOwners(filter, 0);
}

void OwnersWidget::appendOwners(const QString& filter, int offset)
{
    QString sql =
        "SELECT DISTINCT o.id, o.first_name, o.last_name, o.phone "
        "FROM owners o "
        "LEFT JOIN animals a ON a.owner_id = o.id "
        "WHERE TRUE ";

    if (!filter.isEmpty())
        sql += "AND (CONCAT(o.first_name,' ',o.last_name) LIKE :f "
               "OR o.phone LIKE :f2 "
               "OR o.phone_secondary LIKE :f3 "
               "OR a.file_number LIKE :f4 "
               "OR a.name LIKE :f5) ";

    sql += QString("ORDER BY o.first_name LIMIT %1 OFFSET %2")
               .arg(m_pageSize + 1)
               .arg(offset);

    QSqlQuery q;
    q.prepare(sql);
    if (!filter.isEmpty()) {
        QString like = "%" + filter + "%";
        q.bindValue(":f",  like);
        q.bindValue(":f2", like);
        q.bindValue(":f3", like);
        q.bindValue(":f4", like);
        q.bindValue(":f5", like);
    }
    q.exec();

    int fetched = 0;
    while (q.next()) {
        fetched++;
        if (fetched > m_pageSize) break;

        int id        = q.value("id").toInt();
        QString name  = q.value("first_name").toString()
                       + " " + q.value("last_name").toString();
        QString phone = q.value("phone").toString();

        auto* item = new QListWidgetItem(ui->ownerListWidget);
        item->setData(Qt::UserRole, id);
        item->setSizeHint(QSize(0, 56));

        auto* widget = new OwnerItemWidget(name, phone, ui->ownerListWidget);
        ui->ownerListWidget->setItemWidget(item, widget);

        if (id == m_selectedOwnerId)
            ui->ownerListWidget->setCurrentItem(item);
    }

    bool hasMore = (fetched > m_pageSize);
    if (hasMore)
        m_currentOffset = offset + m_pageSize;
    else
        m_currentOffset = offset + fetched;

    auto* panelLayout = qobject_cast<QVBoxLayout*>(ui->listPanel->layout());

    if (hasMore) {
        if (!m_loadMoreBtn) {
            m_loadMoreBtn = new QPushButton(
                QString("نمایش بیشتر  (در حال نمایش %1 ردیف)").arg(m_currentOffset));
            m_loadMoreBtn->setStyleSheet(R"(
                QPushButton {
                    background: #F1F8E9;
                    border: none;
                    border-top: 0.5px solid #C8E6C9;
                    color: #2E7D32;
                    font-size: 12px;
                    font-weight: 500;
                    padding: 10px;
                }
                QPushButton:hover { background: #E8F5E9; }
                QPushButton:pressed { background: #C8E6C9; }
            )");
            int insertIdx = panelLayout ? panelLayout->count() - 1 : -1;
            if (panelLayout && insertIdx >= 0)
                panelLayout->insertWidget(insertIdx, m_loadMoreBtn);
            else if (panelLayout)
                panelLayout->addWidget(m_loadMoreBtn);
            connect(m_loadMoreBtn, &QPushButton::clicked, this, [this]() {
                appendOwners(ui->searchEdit->text().trimmed(), m_currentOffset);
            });
        } else {
            m_loadMoreBtn->setText(
                QString("نمایش بیشتر  (در حال نمایش %1 ردیف)").arg(m_currentOffset));
        }
    } else {
        if (m_loadMoreBtn) {
            if (panelLayout) panelLayout->removeWidget(m_loadMoreBtn);
            m_loadMoreBtn->deleteLater();
            m_loadMoreBtn = nullptr;
        }
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
    q.prepare("SELECT first_name, last_name, phone, phone_secondary, address, notes, gender "
              "FROM owners WHERE id = :id");
    q.bindValue(":id", ownerId);
    q.exec();
    if (!q.next()) return;

    QString fullName = q.value("first_name").toString() + " " + q.value("last_name").toString();

    QString initials;
    QStringList parts = fullName.split(' ', Qt::SkipEmptyParts);
    if (!parts.isEmpty()) initials += parts.first().left(1);
    if (parts.size() > 1) initials += " " + parts.last().left(1);

    ui->ownerAvatarLabel->setText(initials);
    ui->ownerAvatarLabel->setStyleSheet(
        "background:rgba(255,255,255,0.18);color:white;"
        "border-radius:25px;font-size:16px;font-weight:500;");
    ui->ownerNameLabel->setText(fullName);

    QSqlQuery countQ;
    countQ.prepare("SELECT COUNT(*) FROM animals WHERE owner_id = :id");
    countQ.bindValue(":id", ownerId);
    countQ.exec();
    int animalCount = countQ.next() ? countQ.value(0).toInt() : 0;
    ui->ownerSubLabel->setText(QString::number(animalCount) + " حیوان ثبت شده");

    clearContactRows();
    addContactRow("شماره اول", q.value("phone").toString());
    if (!q.value("phone_secondary").toString().isEmpty())
        addContactRow("شماره دوم", q.value("phone_secondary").toString());
    if (!q.value("address").toString().isEmpty())
        addContactRow("آدرس", q.value("address").toString());
    if (!q.value("notes").toString().isEmpty())
        addContactRow("یادداشت", q.value("notes").toString());
    addContactRow("جنسیت", q.value("gender").toString());

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

    auto* labelL = new QLabel(label);
    labelL->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    labelL->setStyleSheet("font-size:12px;font-weight:500;color:#757575;background:transparent;");

    auto* valueL = new QLabel(value);
    valueL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    valueL->setWordWrap(true);
    valueL->setStyleSheet("font-size:12px;color:#212121;background:transparent;");

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
    q.prepare("SELECT id, name, animal_type_id FROM animals WHERE owner_id = :id");
    q.bindValue(":id", ownerId);
    q.exec();

    auto* gridLay = qobject_cast<QGridLayout*>(ui->animalsContainer->layout());
    int col = 0, row = 0;

    while (q.next()) {
        int     animalId = q.value("id").toInt();
        QString name     = q.value("name").toString();
        auto    ti       = AnimalTypeInfo::get(q.value("animal_type_id").toInt());

        auto* chip = new QPushButton;
        chip->setLayoutDirection(Qt::LeftToRight);
        chip->setFixedHeight(38);
        chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        chip->setText(name + "  |  " + ti.name);
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

    showOwnerProfile(newOwnerId);

    bool addAnimal = StyledMessageBox::question(
        this, "افزودن صاحب با موفقیت انجام شد.",
        "آیا می‌خواهید برای این صاحب حیوان اضافه کنید؟");
    if (!addAnimal) return;

    AddAnimalDialog animalDlg(newOwnerId, ownerPhone, this);
    if (animalDlg.exec() != QDialog::Accepted) {
        showOwnerProfile(newOwnerId);
        return;
    }

    int newAnimalId = animalDlg.savedAnimalId();
    showOwnerProfile(newOwnerId);

    bool addVaccine = StyledMessageBox::question(
        this, "افزودن حیوان با موفقیت انجام شد.",
        "آیا می‌خواهید برای این حیوان واکسن اضافه کنید؟");
    if (!addVaccine) return;

    AddVaccineDialog vaccineDlg(newAnimalId, this);
    if (vaccineDlg.exec() == QDialog::Accepted)
        showOwnerProfile(newOwnerId);
}
void OwnersWidget::onEditOwnerClicked()
{
    if (m_selectedOwnerId < 0) return;
    AddOwnerDialog dlg(m_selectedOwnerId, this);
    if (dlg.exec() != QDialog::Accepted) return;
    reloadPreservingState();
}

void OwnersWidget::onDeleteOwnerClicked()
{
    if (m_selectedOwnerId < 0) return;

    QSqlQuery countQ;
    countQ.prepare("SELECT COUNT(*) FROM animals WHERE owner_id = :id");
    countQ.bindValue(":id", m_selectedOwnerId);
    countQ.exec();
    int animalCount = countQ.next() ? countQ.value(0).toInt() : 0;

    QString msg = "آیا مطمئن هستید؟";
    if (animalCount > 0)
        msg += QString("\n\nاین صاحب دارای %1 حیوان ثبت شده است که همراه با واکسن‌هایشان حذف خواهند شد.").arg(animalCount);

    if (!StyledMessageBox::question(this, "حذف صاحب", msg)) return;

    QSqlQuery q;
    // CASCADE: delete owner → animals → vaccinations → reminder_followups auto-deleted
    q.prepare("DELETE FROM owners WHERE id = :id");
    q.bindValue(":id", m_selectedOwnerId); q.exec();

    // حیوانات و واکسن‌های این صاحب هم با CASCADE حذف شدند
    PageDirtyTracker::instance().markDirty(
        {AppPage::Dashboard, AppPage::Animals, AppPage::Vaccinations, AppPage::Reminders});

    reloadPreservingState(); // فیلتر/سرچ فعلی حفظ می‌شود؛ پروفایل چون دیگر وجود ندارد خودکار پاک می‌شود
}