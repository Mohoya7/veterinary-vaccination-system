#include "vaccinationswidget.h"
#include "ui_vaccinationswidget.h"
#include "addvaccinedialog.h"
#include "persiandate.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QScrollBar>
#include <QDialog>
#include <QFrame>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

VaccinationsWidget::VaccinationsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VaccinationsWidget)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);

    // Default date
    ui->dateFromEdit->setDate(QDate::currentDate().addYears(-2));
    ui->dateToEdit->setDate(QDate::currentDate().addYears(2));
    ui->singleDateEdit->setDate(QDate::currentDate());

    applyStyle();
    loadVaccineTypeCombo();

    // Default mode: Time interval
    ui->btnModeRange->setChecked(true);
    ui->btnModeSingle->setChecked(false);
    ui->singleDateEdit->setVisible(false);

    connect(ui->searchEdit,       &QLineEdit::textChanged,
            this, &VaccinationsWidget::onFiltersChanged);
    connect(ui->animalTypeCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VaccinationsWidget::onFiltersChanged);
    connect(ui->statusCombo,      QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VaccinationsWidget::onFiltersChanged);
    connect(ui->vaccineTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VaccinationsWidget::onFiltersChanged);
    connect(ui->dateFromEdit,     &QDateEdit::dateChanged,
            this, &VaccinationsWidget::onFiltersChanged);
    connect(ui->dateToEdit,       &QDateEdit::dateChanged,
            this, &VaccinationsWidget::onFiltersChanged);
    connect(ui->singleDateEdit,   &QDateEdit::dateChanged,
            this, &VaccinationsWidget::onFiltersChanged);
    connect(ui->btnModeSingle,    &QPushButton::clicked,
            this, &VaccinationsWidget::onDateModeChanged);
    connect(ui->btnModeRange,     &QPushButton::clicked,
            this, &VaccinationsWidget::onDateModeChanged);
    connect(ui->btnClearDate,     &QPushButton::clicked,
            this, &VaccinationsWidget::onClearDateClicked);
    connect(ui->btnAddVaccine,    &QPushButton::clicked,
            this, &VaccinationsWidget::onAddVaccineClicked);

    loadData();
}

VaccinationsWidget::~VaccinationsWidget() { delete ui; }

void VaccinationsWidget::loadData()
{
    loadStats();
    loadTable();
}

void VaccinationsWidget::showVaccinationById(int vacId)
{
    // فیلترها رو دست نمی‌زنیم — فقط جدول رو با اون یه رکورد پر می‌کنیم
    auto* tbl = ui->vaccinationsTable;
    tbl->setRowCount(0);

    // حذف دکمه نمایش بیشتر اگه بود
    if (m_loadMoreBtn) {
        auto* cardLayout = qobject_cast<QVBoxLayout*>(ui->tableCard->layout());
        if (cardLayout) cardLayout->removeWidget(m_loadMoreBtn);
        m_loadMoreBtn->deleteLater();
        m_loadMoreBtn = nullptr;
    }

    QSqlQuery q;
    q.prepare(
        "SELECT v.id, a.name AS animal_name, a.type AS animal_type, "
        "CONCAT(o.first_name,' ',o.last_name) AS owner_name, o.phone, "
        "vt.name AS vaccine_name, v.vaccinated_at, v.next_reminder_at "
        "FROM vaccinations v "
        "JOIN animals a        ON v.animal_id        = a.id "
        "JOIN owners  o        ON a.owner_id          = o.id "
        "JOIN vaccine_types vt ON v.vaccine_type_id   = vt.id "
        "WHERE v.id = :id"
        );
    q.bindValue(":id", vacId);
    if (!q.exec() || !q.next()) return;

    QDate today = QDate::currentDate();

    auto makeItem = [](const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        return item;
    };

    tbl->insertRow(0);
    tbl->setRowHeight(0, 44);

    bool    isDog       = (q.value("animal_type").toString() == "dog");
    QDate   vacDate     = q.value("vaccinated_at").toDate();
    QDate   nextDate    = q.value("next_reminder_at").toDate();

    QString statusText, statusBg, statusFg;
    if      (nextDate < today)  { statusText = "تأخیر دارد"; statusBg = "#FFEBEE"; statusFg = "#C62828"; }
    else if (nextDate == today) { statusText = "موعد رسیده"; statusBg = "#FFF9C4"; statusFg = "#F57F17"; }
    else                        { statusText = "تمدید شده";  statusBg = "#E8F5E9"; statusFg = "#2E7D32"; }

    tbl->setItem(0, 0, makeItem(q.value("animal_name").toString()));
    tbl->setCellWidget(0, 1, makeBadge(
                                 isDog ? "سگ" : "گربه",
                                 isDog ? "#E8F5E9" : "#FFF3E0",
                                 isDog ? "#1B5E20" : "#BF360C"));
    tbl->setItem(0, 2, makeItem(q.value("owner_name").toString()));
    auto* phoneItem = new QTableWidgetItem(q.value("phone").toString());
    phoneItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tbl->setItem(0, 3, phoneItem);
    tbl->setItem(0, 4, makeItem(q.value("vaccine_name").toString()));
    tbl->setItem(0, 5, makeItem(PersianDate::toDisplayShort(vacDate)));
    tbl->setItem(0, 6, makeItem(PersianDate::toDisplayShort(nextDate)));
    tbl->setCellWidget(0, 7, makeBadge(statusText, statusBg, statusFg));
    tbl->setCellWidget(0, 8, makeActionButtons(vacId));
}

//  applyStyle
void VaccinationsWidget::applyStyle()
{
    // Custom combo
    QString comboStyle = R"(
    QComboBox {
        background: #FBFFF7;
        border: 1px solid #A5D6A7;
        border-radius: 8px;
        padding: 5px 10px 5px 32px;
        font-size: 13px;
        color: #2E7D32;
        font-weight: 500;
        min-height: 32px;
    }
    QComboBox:hover {
        background: #E8F5E9;
        border-color: #2E7D32;
    }
    QComboBox:focus {
        border-color: #2E7D32;
        background: #E8F5E9;
    }
    QComboBox::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: left center;
        width: 28px;
        border: none;
        background: transparent;
    }
    QComboBox::down-arrow {
        image: url(:/icons/chevron-down.svg);
        width: 16px;
        height: 16px;
    }
    QComboBox QAbstractItemView {
        background: white;
        border: 1px solid #C8E6C9;
        border-radius: 8px;
        selection-background-color: #E8F5E9;
        selection-color: #1B5E20;
        font-size: 13px;
        padding: 4px;
        outline: none;
    }
    QComboBox QAbstractItemView::item {
        padding: 8px 12px;
        min-height: 30px;
        border-radius: 4px;
        color: #212121;
    }
    QComboBox QAbstractItemView::item:selected {
        background: #E8F5E9;
        color: #2E7D32;
    }
)";

    QString dateStyle = R"(
    QDateEdit {
        background: #FBFFF7;
        border: 1px solid #A5D6A7;
        border-radius: 8px;
        padding: 5px 10px 5px 32px;
        font-size: 13px;
        color: #2E7D32;
        font-weight: 500;
        min-height: 32px;
    }
    QDateEdit:hover {
        background: #E8F5E9;
        border-color: #2E7D32;
    }
    QDateEdit:focus {
        border-color: #2E7D32;
        background: #E8F5E9;
    }
    QDateEdit::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: left center;
        width: 28px;
        border: none;
        background: transparent;
    }
    QDateEdit::down-arrow {
        image: url(:/icons/chevron-down.svg);
        width: 16px;
        height: 16px;
    }
    QCalendarWidget QWidget {
        background: white;
        color: #212121;
        font-size: 12px;
    }
    QCalendarWidget QToolButton {
        background: #E8F5E9;
        color: #2E7D32;
        border: none;
        border-radius: 4px;
        font-size: 12px;
        font-weight: 500;
        padding: 4px 8px;
    }
    QCalendarWidget QToolButton:hover { background: #C8E6C9; }
    QCalendarWidget QAbstractItemView:enabled {
        color: #212121;
        selection-background-color: #2E7D32;
        selection-color: white;
    }
    QCalendarWidget QAbstractItemView:disabled { color: #BDBDBD; }
)";

    ui->animalTypeCombo->setStyleSheet(comboStyle);
    ui->statusCombo->setStyleSheet(comboStyle);
    ui->vaccineTypeCombo->setStyleSheet(comboStyle);
    ui->dateFromEdit->setStyleSheet(dateStyle);
    ui->dateToEdit->setStyleSheet(dateStyle);
    ui->singleDateEdit->setStyleSheet(dateStyle);


    ;

    setStyleSheet(R"(
        QWidget { background: transparent; }

        QWidget#filterCard {
            background: white;
            border: 0.5px solid #E0E0E0;
            border-radius: 10px;
        }
        QLabel#pageTitle {
            font-size: 14px;
            font-weight: 500;
            color: #212121;
        }
        QLabel#dateModeLabel, QLabel#dateToLabel {
            font-size: 12px;
            color: #757575;
        }
        QLineEdit#searchEdit {
            background: white;
            border: 1px solid #C8E6C9;
            border-radius: 8px;
            padding: 5px 12px;
            font-size: 12px;
            color: #212121;
            min-height: 32px;
        }
        QLineEdit#searchEdit:focus { border-color: #2E7D32; }

        QFrame#filterDivider { color: #F0F0F0; }

        QPushButton#btnModeSingle, QPushButton#btnModeRange {
            background: #F5F5F5;
            border: 1px solid #C8E6C9;
            font-size: 12px;
            color: #757575;
            padding: 0 12px;
            min-height: 32px;
            border-radius: 0px;
        }
        QPushButton#btnModeSingle {
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
            border-left: none;
        }
        QPushButton#btnModeRange {
            border-top-left-radius: 8px;
            border-bottom-left-radius: 8px;
        }
        QPushButton#btnModeSingle:checked,
        QPushButton#btnModeRange:checked {
            background: #2E7D32;
            color: white;
            border-color: #2E7D32;
        }

        QPushButton#btnClearDate {
            background: white;
            border: 1px solid #C8E6C9;
            border-radius: 8px;
            font-size: 11px;
            color: #757575;
            padding: 0 10px;
            min-height: 32px;
        }
        QPushButton#btnClearDate:hover { border-color: #E53935; color: #E53935; }

        QPushButton#btnAddVaccine {
            background: #2E7D32;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 500;
            padding: 0 16px;
            min-height: 36px;
        }
        QPushButton#btnAddVaccine:hover { background: #1B5E20; }

        /* Statistics cards */
        QWidget#statCardTotal {
            background: white;
            border: 0.5px solid #E0E0E0;
            border-radius: 10px;
        }
        QWidget#statCardToday {
            background: white;
            border: 0.5px solid #F9A825;
            border-radius: 10px;
        }
        QWidget#statCardOverdue {
            background: white;
            border: 0.5px solid #FFCDD2;
            border-radius: 10px;
        }
        QLabel#lblTotalTitle, QLabel#lblTodayTitle, QLabel#lblOverdueTitle {
            font-size: 11px; color: #757575; background: transparent;
        }
        QLabel#lblTotalCount  { font-size:22px; font-weight:500; color:#2E7D32;  background:transparent; }
        QLabel#lblTodayCount  { font-size:22px; font-weight:500; color:#F9A825;  background:transparent; }
        QLabel#lblOverdueCount{ font-size:22px; font-weight:500; color:#C62828;  background:transparent; }

        /* table */
        QWidget#tableCard {
            background: white;
            border: 0.5px solid #E0E0E0;
            border-radius: 10px;
        }
        QTableWidget {
            background: white;
            border: none;
            gridline-color: #F5F5F5;
            font-size: 12px;
            outline: none;
        }
        QTableWidget::item {
            padding: 8px 10px;
            color: #212121;
            border-bottom: 0.5px solid #F5F5F5;
        }
        QTableWidget::item:selected {
            background: #E8F5E9;
            color: #212121;
        }
        QHeaderView::section {
            background: #FAFAFA;
            color: #757575;
            font-size: 12px;
            font-weight: 500;
            border: none;
            border-bottom: 0.5px solid #E0E0E0;
            padding: 8px 10px;
        }
    )");

    // label align right
    ui->pageTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->dateModeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblTotalTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblTotalCount->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblTodayTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblTodayCount->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblOverdueTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblOverdueCount->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

//  loadVaccineTypeCombo
void VaccinationsWidget::loadVaccineTypeCombo()
{
    int animalTypeIdx = ui->animalTypeCombo->currentIndex();
    QString animalType;
    if      (animalTypeIdx == 1) animalType = "dog";
    else if (animalTypeIdx == 2) animalType = "cat";

    ui->vaccineTypeCombo->blockSignals(true);
    ui->vaccineTypeCombo->clear();
    ui->vaccineTypeCombo->addItem("همه واکسن‌ها", -1);

    QSqlQuery q;
    if (animalType.isEmpty()) {
        q.prepare("SELECT id, name FROM vaccine_types ORDER BY name");
    } else {
        q.prepare(
            "SELECT id, name FROM vaccine_types "

            "ORDER BY name"
            );
        q.bindValue(":type", animalType);
    }
    q.exec();
    while (q.next())
        ui->vaccineTypeCombo->addItem(q.value("name").toString(), q.value("id").toInt());

    ui->vaccineTypeCombo->blockSignals(false);
}


//  Event handlers
void VaccinationsWidget::onDateModeChanged()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    m_rangeMode = (btn == ui->btnModeRange);
    ui->btnModeRange->setChecked(m_rangeMode);
    ui->btnModeSingle->setChecked(!m_rangeMode);

    ui->singleDateEdit->setVisible(!m_rangeMode);
    ui->dateFromEdit->setVisible(m_rangeMode);
    ui->dateToLabel->setVisible(m_rangeMode);
    ui->dateToEdit->setVisible(m_rangeMode);

    onFiltersChanged();
}

void VaccinationsWidget::onClearDateClicked()
{
    ui->dateFromEdit->setDate(QDate::currentDate().addYears(-2));
    ui->dateToEdit->setDate(QDate::currentDate().addYears(2));
    ui->singleDateEdit->setDate(QDate::currentDate());
    loadTable();
}

void VaccinationsWidget::onFiltersChanged()
{
    if (sender() == ui->animalTypeCombo)
        loadVaccineTypeCombo();
    loadStats();
    loadTable();
}

//  loadStats
void VaccinationsWidget::loadStats()
{
    QSqlQuery q;
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    q.exec("SELECT COUNT(*) FROM vaccinations");
    if (q.next()) ui->lblTotalCount->setText(q.value(0).toString());

    q.prepare(
        "SELECT COUNT(DISTINCT v.id) FROM vaccinations v "
        "JOIN reminder_followups rf ON rf.vaccination_id=v.id "
        "WHERE v.next_reminder_at=:today AND rf.is_resolved=FALSE"
        );
    q.bindValue(":today", today);
    q.exec();
    if (q.next()) ui->lblTodayCount->setText(q.value(0).toString());

    q.prepare(
        "SELECT COUNT(DISTINCT v.id) FROM vaccinations v "
        "JOIN reminder_followups rf ON rf.vaccination_id=v.id "
        "WHERE v.next_reminder_at < :today AND rf.is_resolved=FALSE"
        );
    q.bindValue(":today", today);
    q.exec();
    if (q.next()) ui->lblOverdueCount->setText(q.value(0).toString());
}

//  buildWhereClause — بدون LIMIT، مشترک بین loadTable و PDF
QString VaccinationsWidget::buildWhereClause() const
{
    QString search       = ui->searchEdit->text().trimmed();
    int     animalTypeIdx = ui->animalTypeCombo->currentIndex();
    int     statusIdx    = ui->statusCombo->currentIndex();
    int     vaccineTypeId = ui->vaccineTypeCombo->currentData().toInt();

    QString where = "WHERE TRUE ";

    if (!search.isEmpty())
        where += "AND (a.name LIKE :search "
                 "OR o.phone LIKE :search2 "
                 "OR CONCAT(o.first_name,' ',o.last_name) LIKE :search3) ";

    if      (animalTypeIdx == 1) where += "AND a.type = 'dog' ";
    else if (animalTypeIdx == 2) where += "AND a.type = 'cat' ";

    if (vaccineTypeId > 0)
        where += "AND v.vaccine_type_id = :vtid ";

    if (!m_rangeMode)
        where += "AND v.vaccinated_at = :single ";
    else
        where += "AND v.vaccinated_at BETWEEN :dfrom AND :dto ";

    // وضعیت — در کوئری اصلی فیلتر می‌کنیم تا pagination درست باشه
    QDate today = QDate::currentDate();
    if (statusIdx == 1)      // موعد رسیده
        where += QString("AND v.next_reminder_at = '%1' ").arg(today.toString("yyyy-MM-dd"));
    else if (statusIdx == 2) // تمدید شده
        where += QString("AND v.next_reminder_at > '%1' ").arg(today.toString("yyyy-MM-dd"));
    else if (statusIdx == 3) // تأخیر دارد
        where += QString("AND v.next_reminder_at < '%1' ").arg(today.toString("yyyy-MM-dd"));

    return where;
}

void VaccinationsWidget::bindWhereParams(QSqlQuery& q) const
{
    QString search       = ui->searchEdit->text().trimmed();
    int     vaccineTypeId = ui->vaccineTypeCombo->currentData().toInt();

    if (!search.isEmpty()) {
        QString like = "%" + search + "%";
        q.bindValue(":search",  like);
        q.bindValue(":search2", like);
        q.bindValue(":search3", like);
    }
    if (vaccineTypeId > 0)
        q.bindValue(":vtid", vaccineTypeId);

    if (!m_rangeMode)
        q.bindValue(":single", ui->singleDateEdit->date().toString("yyyy-MM-dd"));
    else {
        q.bindValue(":dfrom", ui->dateFromEdit->date().toString("yyyy-MM-dd"));
        q.bindValue(":dto",   ui->dateToEdit->date().toString("yyyy-MM-dd"));
    }
}

//  loadTable — reset + first page
void VaccinationsWidget::loadTable()
{
    auto* tbl = ui->vaccinationsTable;
    tbl->setRowCount(0);
    tbl->setColumnCount(9);
    tbl->setLayoutDirection(Qt::RightToLeft);

    QStringList headers = {
        "حیوان","نوع","صاحب","شماره",
        "نوع واکسن","تاریخ تزریق","یادآوری بعدی","وضعیت",""
    };
    tbl->setHorizontalHeaderLabels(headers);

    auto* hv = tbl->horizontalHeader();
    hv->setLayoutDirection(Qt::RightToLeft);
    hv->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hv->setSectionResizeMode(QHeaderView::Stretch);
    hv->setSectionResizeMode(8, QHeaderView::Fixed);
    tbl->setColumnWidth(8, 150);

    tbl->verticalHeader()->setVisible(false);
    tbl->setShowGrid(false);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setFocusPolicy(Qt::NoFocus);
    tbl->setAlternatingRowColors(false);

    tbl->verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical {
            background: transparent; width: 6px;
            border-radius: 3px; margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: rgba(100,170,80,180);
            border-radius: 3px; min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: rgba(46,125,50,220); }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical { background: transparent; }
    )");

    tbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // حذف دکمه نمایش بیشتر قدیمی — دقیقاً مثل reminders
    if (m_loadMoreBtn) {
        auto* cardLayout = qobject_cast<QVBoxLayout*>(ui->tableCard->layout());
        if (cardLayout) cardLayout->removeWidget(m_loadMoreBtn);
        m_loadMoreBtn->deleteLater();
        m_loadMoreBtn = nullptr;
    }

    m_offset = 0;
    appendRows(0);
}

//  appendRows — یک صفحه اضافه می‌کند
void VaccinationsWidget::appendRows(int offset)
{
    auto* tbl = ui->vaccinationsTable;

    QString sql =
        "SELECT DISTINCT v.id, a.name AS animal_name, a.type AS animal_type, "
        "CONCAT(o.first_name,' ',o.last_name) AS owner_name, o.phone, "
        "vt.name AS vaccine_name, v.vaccinated_at, v.next_reminder_at "
        "FROM vaccinations v "
        "JOIN animals a      ON v.animal_id        = a.id "
        "JOIN owners  o      ON a.owner_id          = o.id "
        "JOIN vaccine_types vt ON v.vaccine_type_id = vt.id "
        + buildWhereClause()
        + "ORDER BY v.vaccinated_at DESC "
          "LIMIT :limit OFFSET :offset";

    QSqlQuery q;
    q.prepare(sql);
    bindWhereParams(q);
    q.bindValue(":limit",  kPageSize + 1); // یه تا اضافه تا بفهمیم صفحه بعدی هست
    q.bindValue(":offset", offset);
    if (!q.exec()) return;

    QDate today    = QDate::currentDate();
    int   startRow = tbl->rowCount();
    int   fetched  = 0;

    auto makeItem = [](const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        return item;
    };

    while (q.next()) {
        fetched++;
        if (fetched > kPageSize) break; // رکورد اضافه رو نمایش نمی‌دیم

        int     vacId       = q.value("id").toInt();
        QString animalName  = q.value("animal_name").toString();
        QString animalType  = q.value("animal_type").toString();
        QString ownerName   = q.value("owner_name").toString();
        QString phone       = q.value("phone").toString();
        QString vaccineName = q.value("vaccine_name").toString();
        QDate   vacDate     = q.value("vaccinated_at").toDate();
        QDate   nextDate    = q.value("next_reminder_at").toDate();

        QString statusText, statusBg, statusFg;
        if (nextDate < today) {
            statusText = "تأخیر دارد"; statusBg = "#FFEBEE"; statusFg = "#C62828";
        } else if (nextDate == today) {
            statusText = "موعد رسیده"; statusBg = "#FFF9C4"; statusFg = "#F57F17";
        } else {
            statusText = "تمدید شده";  statusBg = "#E8F5E9"; statusFg = "#2E7D32";
        }

        int row = startRow + (fetched - 1);
        tbl->insertRow(row);
        tbl->setRowHeight(row, 44);

        bool isDog = (animalType == "dog");

        tbl->setItem(row, 0, makeItem(animalName));
        tbl->setCellWidget(row, 1, makeBadge(
                                       isDog ? "سگ" : "گربه",
                                       isDog ? "#E8F5E9" : "#FFF3E0",
                                       isDog ? "#1B5E20" : "#BF360C"));
        tbl->setItem(row, 2, makeItem(ownerName));
        auto* phoneItem = new QTableWidgetItem(phone);
        phoneItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tbl->setItem(row, 3, phoneItem);
        tbl->setItem(row, 4, makeItem(vaccineName));
        tbl->setItem(row, 5, makeItem(PersianDate::toDisplayShort(vacDate)));
        tbl->setItem(row, 6, makeItem(PersianDate::toDisplayShort(nextDate)));
        tbl->setCellWidget(row, 7, makeBadge(statusText, statusBg, statusFg));
        tbl->setCellWidget(row, 8, makeActionButtons(vacId));
    }

    bool hasMore = (fetched > kPageSize);
    if (hasMore)
        m_offset = offset + kPageSize;
    else
        m_offset = offset + fetched;

    // مدیریت دکمه نمایش بیشتر — دقیقاً مثل reminders
    auto* cardLayout = qobject_cast<QVBoxLayout*>(ui->tableCard->layout());

    if (hasMore) {
        if (!m_loadMoreBtn) {
            m_loadMoreBtn = new QPushButton(
                QString("نمایش بیشتر  (در حال نمایش %1 ردیف)").arg(m_offset));
            m_loadMoreBtn->setStyleSheet(R"(
                QPushButton {
                    background: #F1F8E9;
                    border: none;
                    border-top: 0.5px solid #C8E6C9;
                    border-bottom-left-radius: 10px;
                    border-bottom-right-radius: 10px;
                    color: #2E7D32;
                    font-size: 12px;
                    font-weight: 500;
                    padding: 10px;
                }
                QPushButton:hover { background: #E8F5E9; }
                QPushButton:pressed { background: #C8E6C9; }
            )");
            if (cardLayout) cardLayout->addWidget(m_loadMoreBtn);
            connect(m_loadMoreBtn, &QPushButton::clicked, this, [this]() {
                appendRows(m_offset);
            });
        } else {
            m_loadMoreBtn->setText(
                QString("نمایش بیشتر  (در حال نمایش %1 ردیف)").arg(m_offset));
        }
    } else {
        if (m_loadMoreBtn) {
            if (cardLayout) cardLayout->removeWidget(m_loadMoreBtn);
            m_loadMoreBtn->deleteLater();
            m_loadMoreBtn = nullptr;
        }
    }
}


//  makeBadge
QWidget* VaccinationsWidget::makeBadge(const QString& text,
                                       const QString& bg,
                                       const QString& fg)
{
    auto* container = new QWidget;
    container->setLayoutDirection(Qt::RightToLeft);
    container->setStyleSheet("background: transparent;");
    auto* lay = new QHBoxLayout(container);
    lay->setContentsMargins(8, 2, 8, 2);
    lay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* lbl = new QLabel(text);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lbl->setStyleSheet(QString(
                           "background:%1; color:%2; border-radius:4px;"
                           "font-size:11px; font-weight:500; padding:2px 8px;"
                           ).arg(bg, fg));

    lay->addWidget(lbl);
    lay->addStretch();
    return container;
}

//  makeActionButtons
QWidget* VaccinationsWidget::makeActionButtons(int vaccinationId)
{
    auto* container = new QWidget;
    container->setStyleSheet("background: transparent;");
    auto* lay = new QHBoxLayout(container);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(6);
    lay->setAlignment(Qt::AlignCenter);

    // Edit button
    auto* btnEdit = new QPushButton("ویرایش");
    btnEdit->setFixedHeight(27);
    btnEdit->setStyleSheet(R"(
        QPushButton {
            background: white;
            border: 0.5px solid #E0E0E0;
            border-radius: 6px;
            font-size: 11px;
            color: #555555;
            padding: 0 10px;
        }
        QPushButton:hover { border-color: #2E7D32; color: #2E7D32; }
    )");

    // delete button
    auto* btnDel = new QPushButton("حذف");
    btnDel->setFixedHeight(27);
    btnDel->setStyleSheet(R"(
        QPushButton {
            background: #FFEBEE;
            border: 0.5px solid #FFCDD2;
            border-radius: 6px;
            font-size: 11px;
            color: #E53935;
            padding: 0 10px;
        }
        QPushButton:hover {
            background: #FFCDD2;
            border-color: #E53935;
        }
    )");

    connect(btnEdit, &QPushButton::clicked, this, [this, vaccinationId]() {
        QSqlQuery q;
        q.prepare("SELECT animal_id FROM vaccinations WHERE id=:id");
        q.bindValue(":id", vaccinationId);
        q.exec();
        if (!q.next()) return;
        int animalId = q.value("animal_id").toInt();
        AddVaccineDialog dlg(animalId, this);
        if (dlg.exec() == QDialog::Accepted) loadData();
    });

    connect(btnDel, &QPushButton::clicked, this, [this, vaccinationId]() {
        auto reply = QMessageBox::question(
            this, "حذف واکسن",
            "آیا مطمئن هستید که می‌خواهید این واکسن را حذف کنید؟",
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply != QMessageBox::Yes) return;

        QSqlQuery q;
        q.prepare("UPDATE reminder_followups SET is_resolved=TRUE WHERE vaccination_id=:id");
        q.bindValue(":id", vaccinationId); q.exec();

        q.prepare("DELETE FROM vaccinations WHERE id=:id");
        q.bindValue(":id", vaccinationId); q.exec();

        loadData();
    });

    lay->addWidget(btnEdit);
    lay->addWidget(btnDel);
    return container;
}

//  showAnimalPickerDialog
int VaccinationsWidget::showAnimalPickerDialog()
{
    QDialog picker(this);
    picker.setWindowTitle("انتخاب حیوان");
    picker.setLayoutDirection(Qt::LeftToRight);
    picker.setFixedWidth(480);
    picker.setMinimumHeight(500);
    picker.setStyleSheet("QDialog { background: #F9FBF9; }");

    auto* mainLay = new QVBoxLayout(&picker);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);

    // Header
    auto* header = new QWidget;
    header->setObjectName("pickerHeader");
    header->setStyleSheet(
        "QWidget#pickerHeader {"
        "  background:#2E7D32;"
        "  border-top-left-radius:12px;"
        "  border-top-right-radius:12px;"
        "}"
        );
    header->setFixedHeight(64);
    auto* headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(20, 0, 20, 0);
    headerLay->setDirection(QBoxLayout::RightToLeft);

    auto* headerTitle = new QLabel("انتخاب حیوان برای ثبت واکسن");
    headerTitle->setStyleSheet(
        "color:white; font-size:14px; font-weight:bold; background:transparent;"
        );
    headerTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    headerLay->addWidget(headerTitle);
    headerLay->addStretch();

    // ── Body ──
    auto* body = new QWidget;
    body->setStyleSheet("QWidget { background:white; }");
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(20, 20, 20, 16);
    bodyLay->setSpacing(12);

    auto* hint = new QLabel("نام حیوان، نام صاحب یا شماره تماس را جستجو کنید:");
    hint->setStyleSheet("font-size:12px; color:#757575; background:transparent;");
    hint->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* searchLine = new QLineEdit;
    searchLine->setPlaceholderText("جستجو...");
    searchLine->setLayoutDirection(Qt::RightToLeft);
    searchLine->setStyleSheet(R"(
        QLineEdit {
            border: 1px solid #C8E6C9;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            background: #F9FBF9;
            color: #212121;
            min-height: 36px;
        }
        QLineEdit:focus { border-color:#2E7D32; background:white; }
    )");

    auto* listW = new QListWidget;
    listW->setMinimumHeight(260);
    listW->setLayoutDirection(Qt::RightToLeft);
    listW->setStyleSheet(R"(
    QListWidget {
        border: 1px solid #E0E0E0;
        border-radius: 8px;
        background: white;
        font-size: 13px;
        outline: none;
    }
    QListWidget::item {
        padding: 12px 14px;
        border-bottom: 0.5px solid #F5F5F5;
    }
    QListWidget::item:hover    { background: #F5F5F5; }
    QListWidget::item:selected { background: #E8F5E9; color: #212121; }

    QScrollBar:vertical {
        background: #F5F5F5;
        width: 6px;
        border-radius: 3px;
        margin: 4px 2px;
    }
    QScrollBar::handle:vertical {
        background: #A5D6A7;
        border-radius: 3px;
        min-height: 30px;
    }
    QScrollBar::handle:vertical:hover {
        background: #2E7D32;
    }
    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical { height: 0px; }
    QScrollBar::add-page:vertical,
    QScrollBar::sub-page:vertical { background: transparent; }
)");


    bodyLay->addWidget(hint);
    bodyLay->addWidget(searchLine);
    bodyLay->addWidget(listW, 1);

    // ── Footer ──
    auto* footer = new QWidget;
    footer->setStyleSheet(
        "QWidget { background:#FAFAFA; border-top:1px solid #E0E0E0; }"
        );
    footer->setFixedHeight(60);
    auto* footerLay = new QHBoxLayout(footer);
    footerLay->setContentsMargins(20, 0, 20, 0);
    footerLay->setSpacing(10);
    footerLay->setDirection(QBoxLayout::RightToLeft);

    auto* btnConfirm = new QPushButton("انتخاب حیوان");
    btnConfirm->setFixedHeight(36);
    btnConfirm->setEnabled(false);
    btnConfirm->setStyleSheet(R"(
        QPushButton {
            background:#2E7D32; color:white;
            border:none; border-radius:8px;
            font-size:13px; font-weight:bold; padding:0 20px;
        }
        QPushButton:hover   { background:#388E3C; }
        QPushButton:pressed { background:#1B5E20; }
        QPushButton:disabled{ background:#BDBDBD; }
    )");

    auto* btnCancel = new QPushButton("انصراف");
    btnCancel->setFixedHeight(36);
    btnCancel->setStyleSheet(R"(
        QPushButton {
            background:white; color:#757575;
            border:1px solid #E0E0E0; border-radius:8px;
            font-size:13px; padding:0 20px;
        }
        QPushButton:hover { background:#F5F5F5; }
    )");

    footerLay->addWidget(btnConfirm);
    footerLay->addWidget(btnCancel);
    footerLay->addStretch();

    mainLay->addWidget(header);
    mainLay->addWidget(body, 1);
    mainLay->addWidget(footer);

    // ── loading animals ──
    auto loadAnimals = [&](const QString& filter) {
        listW->clear();
        QSqlQuery q;
        q.prepare(
            "SELECT a.id, a.name, a.type, "
            "CONCAT(o.first_name,' ',o.last_name) AS owner, o.phone "
            "FROM animals a JOIN owners o ON a.owner_id=o.id "
            "WHERE TRUE "
            "AND (a.name LIKE :f OR o.first_name LIKE :f2 "
            "     OR o.last_name LIKE :f3 OR o.phone LIKE :f4) "
            "ORDER BY a.name LIMIT 60"
            );
        QString f = filter.isEmpty() ? "%" : "%" + filter + "%";
        q.bindValue(":f",  f); q.bindValue(":f2", f);
        q.bindValue(":f3", f); q.bindValue(":f4", f);
        q.exec();

        while (q.next()) {
            bool    isDog   = (q.value("type").toString() == "dog");
            QString typeStr = isDog ? "سگ 🐕" : "گربه 🐈";
            QString display = q.value("name").toString()
                              + "   |   " + typeStr
                              + "   |   " + q.value("owner").toString()
                              + "   |   " + q.value("phone").toString();

            auto* item = new QListWidgetItem(display);
            item->setData(Qt::UserRole, q.value("id").toInt());
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            listW->addItem(item);
        }
    };

    loadAnimals("");

    connect(searchLine, &QLineEdit::textChanged,
            [&](const QString& t){ loadAnimals(t); });

    connect(listW, &QListWidget::itemSelectionChanged, [&](){
        btnConfirm->setEnabled(listW->currentItem() != nullptr);
    });
    connect(listW,      &QListWidget::itemDoubleClicked, &picker, &QDialog::accept);
    connect(btnConfirm, &QPushButton::clicked, &picker, [&](){
        if (listW->currentItem()) picker.accept();
    });
    connect(btnCancel, &QPushButton::clicked, &picker, &QDialog::reject);

    if (picker.exec() != QDialog::Accepted) return -1;
    if (!listW->currentItem()) return -1;
    return listW->currentItem()->data(Qt::UserRole).toInt();
}

//  onAddVaccineClicked
void VaccinationsWidget::onAddVaccineClicked()
{
    int animalId = showAnimalPickerDialog();
    if (animalId < 0) return;

    AddVaccineDialog dlg(animalId, this);
    if (dlg.exec() != QDialog::Accepted) return;

    QMessageBox::information(this, "موفق", "واکسن با موفقیت ثبت شد.");
    loadData();
}