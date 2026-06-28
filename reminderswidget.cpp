#include "reminderswidget.h"
#include "animaltypeinfo.h"
#include "styledmessagebox.h"
#include "pagedirtytracker.h"
#include <QRadioButton>
#include <QButtonGroup>
#include "ui_reminderswidget.h"
#include "addvaccinedialog.h"
#include "persiandate.h"
#include "session.h"

#include <QSqlQuery>
#include <QDate>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QScrollBar>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <QPdfWriter>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QTimer>
#include <QContextMenuEvent>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

RemindersWidget::RemindersWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RemindersWidget)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);

    applyStyle();
    loadAnimalTypeCombo();   // قبلاً هاردکد در .ui بود؛ حالا از animal_types خوانده می‌شود

    // ── Debounce فیلد سرچ (300ms) ───────────────────────────────────────
    m_searchDebounce = new QTimer(this);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(300);
    connect(m_searchDebounce, &QTimer::timeout, this, &RemindersWidget::onFiltersChanged);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        m_searchDebounce->start();
    });
    connect(ui->animalTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                loadVaccineTypeCombo();
                onFiltersChanged();
            });
    connect(ui->vaccineTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RemindersWidget::onFiltersChanged);

    // ── Create a date filter widget ─
    buildFilterDateWidget();

    // Initial load of vaccines
    loadVaccineTypeCombo();
    connect(ui->followUpCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RemindersWidget::onFiltersChanged);
    connect(ui->responseCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RemindersWidget::onFiltersChanged);
    connect(ui->btnExportPdf,    &QPushButton::clicked,
            this, &RemindersWidget::onExportPdfClicked);

    connect(ui->btnModeToday, &QPushButton::clicked, this, [this]() {
        ui->btnModeToday->setChecked(true);
        ui->btnModeOverdue->setChecked(false);
        ui->filterDateContainer->setVisible(false);
        onFiltersChanged();
    });

    connect(ui->btnModeOverdue, &QPushButton::clicked, this, [this]() {
        ui->btnModeOverdue->setChecked(true);
        ui->btnModeToday->setChecked(false);
        ui->filterDateContainer->setVisible(true);
        onFiltersChanged();
    });

    loadData();
}

RemindersWidget::~RemindersWidget() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────
// Public
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::loadData()
{
    loadStats();
    loadTable();
}

// ─── Reload با حفظ فیلتر/سرچ/offset فعلی ─────────────────────────────────
void RemindersWidget::reloadPreservingState()
{
    loadAnimalTypeCombo();
    loadVaccineTypeCombo();
    loadStats();
    loadTable(); // فقط صفحه‌ی اول
}

// ─────────────────────────────────────────────────────────────────────────────
// Style
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::applyStyle()
{
    QString comboStyle = R"(
        QComboBox {
            background: #FBFFF7;
            border: 1px solid #A5D6A7;
            border-radius: 8px;
            padding: 5px 10px 5px 32px;
            font-size: 12px;
            color: #2E7D32;
            font-weight: 500;
            min-height: 32px;
        }
        QComboBox:hover { background: #E8F5E9; border-color: #2E7D32; }
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
            font-size: 12px;
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

    ui->animalTypeCombo->setStyleSheet(comboStyle);
    ui->followUpCombo->setStyleSheet(comboStyle);
    ui->responseCombo->setStyleSheet(comboStyle);
    ui->vaccineTypeCombo->setStyleSheet(comboStyle);

    setStyleSheet(R"(
        QWidget { background: transparent; }

        QWidget#filterCard {
            background: white;
            border: 0.5px solid #E0E0E0;
            border-radius: 10px;
        }
        QLabel#pageTitle {
            font-size: 15px;
            font-weight: 600;
            color: #212121;
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

        QSpinBox#overdueDaysSpin {
            background: #F1F8E9;
            border: 1.5px solid #A5D6A7;
            border-radius: 8px;
            padding: 5px 10px;
            font-size: 12px;
            color: #2E7D32;
            font-weight: 500;
            min-height: 32px;
            max-width: 80px;
        }
        QLabel#overdueDaysLabel {
            font-size: 12px;
            color: #757575;
        }
        QFrame#filterDivider { color: #F0F0F0; }

        QPushButton#btnModeToday, QPushButton#btnModeOverdue {
            background: #F5F5F5;
            border: 1px solid #C8E6C9;
            font-size: 12px;
            color: #757575;
            min-height: 34px;
            padding: 0 16px;
        }
        QPushButton#btnModeToday {
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
            border-left: none;
        }
        QPushButton#btnModeOverdue {
            border-top-left-radius: 8px;
            border-bottom-left-radius: 8px;
        }
        QPushButton#btnModeToday:checked, QPushButton#btnModeOverdue:checked {
            background: #2E7D32;
            color: white;
            border-color: #2E7D32;
        }

        QPushButton#btnExportPdf {
            background: white;
            border: 1px solid #C8E6C9;
            border-radius: 8px;
            font-size: 12px;
            color: #2E7D32;
            font-weight: 500;
            padding: 0 16px;
            min-height: 36px;
        }
        QPushButton#btnExportPdf:hover { background: #E8F5E9; }

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
        QWidget#statCardDone {
            background: white;
            border: 0.5px solid #C8E6C9;
            border-radius: 10px;
        }
        QLabel#lblTodayTitle, QLabel#lblOverdueTitle, QLabel#lblDoneTitle {
            font-size: 11px; color: #757575; background: transparent;
        }
        QLabel#lblTodayCount {
            font-size: 22px; font-weight: 500; color: #F9A825; background: transparent;
        }
        QLabel#lblOverdueCount {
            font-size: 22px; font-weight: 500; color: #C62828; background: transparent;
        }
        QLabel#lblDoneCount {
            font-size: 22px; font-weight: 500; color: #2E7D32; background: transparent;
        }
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
            padding: 8px;
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
        QCheckBox { spacing: 0px; }
        QCheckBox::indicator {
            width: 18px; height: 18px;
            border: 1.5px solid #A5D6A7;
            border-radius: 4px;
            background: white;
        }
        QCheckBox::indicator:checked {
            background: #2E7D32;
            border-color: #2E7D32;
            image: url(:/icons/check.svg);
        }
        QCheckBox::indicator:hover { border-color: #2E7D32; }
    )");

    ui->pageTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblTodayTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblTodayCount->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblOverdueTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblOverdueCount->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblDoneTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->lblDoneCount->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

// ── loadAnimalTypeCombo ──────────────────────────────────────────────────────
// قبلاً هاردکد در .ui بود (همه/سگ/گربه)؛ حالا از animal_types خوانده می‌شود
void RemindersWidget::loadAnimalTypeCombo()
{
    int savedTypeId = ui->animalTypeCombo->count() > 0
                          ? ui->animalTypeCombo->currentData().toInt()
                          : -1;

    ui->animalTypeCombo->blockSignals(true);
    ui->animalTypeCombo->clear();
    ui->animalTypeCombo->addItem("همه انواع", -1);

    QSqlQuery typeQ;
    typeQ.exec("SELECT id, name FROM animal_types ORDER BY id");
    while (typeQ.next())
        ui->animalTypeCombo->addItem(typeQ.value("name").toString(), typeQ.value("id").toInt());

    int idx = ui->animalTypeCombo->findData(savedTypeId);
    ui->animalTypeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    ui->animalTypeCombo->blockSignals(false);
}

// ── loadVaccineTypeCombo ─────────────────────────────────────────────────────
void RemindersWidget::loadVaccineTypeCombo()
{
    int animalTypeId = ui->animalTypeCombo->count() > 0
                           ? ui->animalTypeCombo->currentData().toInt()
                           : -1;

    ui->vaccineTypeCombo->blockSignals(true);
    ui->vaccineTypeCombo->clear();
    ui->vaccineTypeCombo->addItem("همه واکسن‌ها", -1);

    QSqlQuery q;
    if (animalTypeId == -1) {
        q.prepare("SELECT id, name FROM vaccine_types ORDER BY name");
    } else {
        q.prepare(
            "SELECT DISTINCT vt.id, vt.name "
            "FROM vaccine_types vt "
            "JOIN vaccine_type_animals vta ON vta.vaccine_type_id = vt.id "
            "WHERE vta.animal_type_id = :atid "
            "ORDER BY vt.name");
        q.bindValue(":atid", animalTypeId);
    }
    q.exec();
    while (q.next())
        ui->vaccineTypeCombo->addItem(q.value("name").toString(),
                                      q.value("id").toInt());
    ui->vaccineTypeCombo->blockSignals(false);
}

// ── buildFilterDateWidget ─────────────────────────────────────────────────────
void RemindersWidget::buildFilterDateWidget()
{

    m_filterDateWidget = new QWidget(this);
    m_filterDateWidget->setStyleSheet("background:transparent;");
    auto* mainLay = new QHBoxLayout(m_filterDateWidget);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(6);

    // ── Combo mode selection (based on day/date range) ─
    m_subModeCombo = new QComboBox;
    m_subModeCombo->addItem("بر اساس روز", 0);   // index 0 = day
    m_subModeCombo->addItem("بازه تاریخ",  1);   // index 1 = date picker
    m_subModeCombo->setLayoutDirection(Qt::RightToLeft);
    m_subModeCombo->setStyleSheet(R"(
        QComboBox {
            background: #FBFFF7;
            border: 1px solid #A5D6A7;
            border-radius: 8px;
            padding: 5px 10px 5px 32px;
            font-size: 12px;
            color: #2E7D32;
            font-weight: 500;
            min-height: 22px;
        }
        QComboBox:hover { background: #E8F5E9; border-color: #2E7D32; }
        QComboBox::drop-down {
            subcontrol-origin: padding; subcontrol-position: left center;
            width: 28px; border: none; background: transparent;
        }
        QComboBox::down-arrow {
            image: url(:/icons/chevron-down.svg); width: 16px; height: 16px;
        }
        QComboBox QAbstractItemView {
            background: white; border: 1px solid #C8E6C9; border-radius: 8px;
            selection-background-color: #E8F5E9; selection-color: #1B5E20;
            font-size: 12px; padding: 4px; outline: none;
        }
        QComboBox QAbstractItemView::item {
            padding: 8px 12px; min-height: 30px; border-radius: 4px; color: #212121;
        }
        QComboBox QAbstractItemView::item:selected { background: #E8F5E9; color: #2E7D32; }
    )");
    mainLay->addWidget(m_subModeCombo);

    // ── Manual column (based on day) ──────────────────────────────────────────
    auto* manualWidget = new QWidget;
    manualWidget->setStyleSheet("background:transparent;");
    auto* manualLay = new QHBoxLayout(manualWidget);
    manualLay->setContentsMargins(0, 0, 0, 0);
    manualLay->setSpacing(6);

    m_daysSpin = new QSpinBox;
    m_daysSpin->setRange(1, 365);
    m_daysSpin->setValue(7);
    m_daysSpin->setFixedHeight(32);
    m_daysSpin->setFixedWidth(65);
    m_daysSpin->setAlignment(Qt::AlignCenter);
    m_daysSpin->setStyleSheet(R"(
        QSpinBox {
            border: 1px solid #A5D6A7; border-radius: 6px;
            padding: 4px 6px; font-size: 13px;
            background: #F9FBF9; color: #2E7D32; font-weight: 500;
        }
        QSpinBox:focus { border-color: #2E7D32; }
        QSpinBox::up-button, QSpinBox::down-button {
            border: none; width: 16px; background: transparent;
        }
    )");

    // ── toggle button Future/Past ─────────────────────────────────────────
    m_dirBtn = new QPushButton("روز آینده");
    m_dirBtn->setProperty("isFuture", true);   // Default: next day
    m_dirBtn->setFixedHeight(32);
    m_dirBtn->setFixedWidth(88);
    m_dirBtn->setCursor(Qt::PointingHandCursor);
    m_dirBtn->setStyleSheet(R"(
        QPushButton {
            background: #F1F8E9;
            border: 1px solid #A5D6A7;
            border-radius: 6px;
            font-size: 12px;
            color: #2E7D32;
            font-weight: 500;
            qproperty-alignment: AlignCenter;
        }
        QPushButton:hover   { background: #E8F5E9; border-color: #2E7D32; }
        QPushButton:pressed { background: #C8E6C9; }
    )");
    // Click → Change state
    connect(m_dirBtn, &QPushButton::clicked, this, [this]() {
        bool wasFuture = m_dirBtn->property("isFuture").toBool();
        m_dirBtn->setProperty("isFuture", !wasFuture);
        m_dirBtn->setText(!wasFuture ? "روز آینده" : "روز گذشته");
        onFiltersChanged();
    });

    manualLay->addWidget(m_daysSpin);
    manualLay->addWidget(m_dirBtn);
    mainLay->addWidget(manualWidget);

    // ── Date range column ──────────────────────────────────────────────────
    auto* rangeWidget = new QWidget;
    rangeWidget->setStyleSheet("background:transparent;min-height: 32px;");
    rangeWidget->setVisible(false);
    auto* rangeLay = new QHBoxLayout(rangeWidget);
    rangeLay->setContentsMargins(0, 0, 0, 0);
    rangeLay->setSpacing(6);

    m_pickerFrom = new PersianDatePicker(rangeWidget);
    m_pickerFrom->setDate(QDate::currentDate());
    m_pickerFrom->setFixedWidth(150);

    auto* toLbl = new QLabel("تا");
    toLbl->setStyleSheet("font-size:12px;color:#757575;background:transparent;");
    toLbl->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    toLbl->setFixedWidth(20);

    m_pickerTo = new PersianDatePicker(rangeWidget);
    m_pickerTo->setDate(QDate::currentDate());
    m_pickerTo->setFixedWidth(150);

    rangeLay->setAlignment(Qt::AlignVCenter);
    rangeLay->addWidget(m_pickerFrom);
    rangeLay->addWidget(toLbl);
    rangeLay->addWidget(m_pickerTo);
    mainLay->addWidget(rangeWidget);

    mainLay->addStretch();

    // ── Placement in container ─────────────────────────────────────────────
    if (ui->filterDateContainer->layout())
        delete ui->filterDateContainer->layout();
    auto* containerLay = new QVBoxLayout(ui->filterDateContainer);
    containerLay->setContentsMargins(0, 0, 0, 0);
    containerLay->setAlignment(Qt::AlignVCenter);
    containerLay->addWidget(m_filterDateWidget);

    // ── signals ───────────────────────────────────────────────────────────
    connect(m_subModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, manualWidget, rangeWidget](int idx) {
                m_subManualMode = (idx == 0);
                manualWidget->setVisible(m_subManualMode);
                rangeWidget->setVisible(!m_subManualMode);
                onFiltersChanged();
            });
    connect(m_daysSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RemindersWidget::onFiltersChanged);
    connect(m_pickerFrom, &PersianDatePicker::dateChanged,
            this, [this](const QDate&) { onFiltersChanged(); });
    connect(m_pickerTo, &PersianDatePicker::dateChanged,
            this, [this](const QDate&) { onFiltersChanged(); });
}

void RemindersWidget::onFiltersChanged()
{
    loadStats();
    loadTable(); // reset from page 0
}

void RemindersWidget::onLoadMoreClicked()
{
    appendRows(m_offset);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::loadStats()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery q;

    q.prepare("SELECT COUNT(DISTINCT v.id) FROM vaccinations v "
              "JOIN reminder_followups rf ON rf.vaccination_id = v.id "
              "WHERE v.next_reminder_at = :today "
              "AND rf.is_resolved = FALSE");
    q.bindValue(":today", today);
    q.exec();
    if (q.next()) ui->lblTodayCount->setText(q.value(0).toString());

    q.prepare("SELECT COUNT(DISTINCT v.id) FROM vaccinations v "
              "JOIN reminder_followups rf ON rf.vaccination_id = v.id "
              "WHERE v.next_reminder_at < :today "
              "AND rf.is_resolved = FALSE");
    q.bindValue(":today", today);
    q.exec();
    if (q.next()) ui->lblOverdueCount->setText(q.value(0).toString());

    q.prepare("SELECT COUNT(DISTINCT v.id) FROM vaccinations v "
              "JOIN reminder_followups rf ON rf.vaccination_id = v.id "
              "WHERE v.next_reminder_at = :today "
              "AND rf.is_followed_up = TRUE");
    q.bindValue(":today", today);
    q.exec();
    if (q.next()) ui->lblDoneCount->setText(q.value(0).toString());
}

// ─────────────────────────────────────────────────────────────────────────────
// WHERE clause builder (shared by table & PDF queries)
// ─────────────────────────────────────────────────────────────────────────────

QString RemindersWidget::buildWhereClause() const
{
    bool    modeToday    = ui->btnModeToday->isChecked();
    QString search       = ui->searchEdit->text().trimmed();
    int     animalTypeId = ui->animalTypeCombo->currentData().toInt();
    int     followUpIdx  = ui->followUpCombo->currentIndex();
    int     responseIdx  = ui->responseCombo->currentIndex();

    QString w = "WHERE rf.is_resolved = FALSE ";

    if (modeToday)
        w += "AND v.next_reminder_at = :today ";
    else
        w += "AND v.next_reminder_at >= :overdueFrom "
             "AND v.next_reminder_at <= :overdueTo ";

    if (!search.isEmpty())
        w += "AND (a.name LIKE :search OR o.phone LIKE :search2 "
             "OR CONCAT(o.first_name,' ',o.last_name) LIKE :search3 "
             "OR a.file_number LIKE :search4 "
             "OR o.phone_secondary LIKE :search5) ";

    if (animalTypeId != -1)
        w += "AND a.animal_type_id = :atype ";

    // فیلتر نوع واکسن
    int vaccineTypeId = ui->vaccineTypeCombo->currentData().toInt();
    if (vaccineTypeId > 0)
        w += "AND v.vaccine_type_id = :vtid ";

    if (followUpIdx == 1)      w += "AND rf.is_followed_up = FALSE ";
    else if (followUpIdx == 2) w += "AND rf.is_followed_up = TRUE ";

    if (responseIdx == 1)      w += "AND rf.owner_responded IS NULL ";
    else if (responseIdx == 2) w += "AND rf.owner_responded = TRUE ";
    else if (responseIdx == 3) w += "AND rf.owner_responded = FALSE ";

    return w;
}

void RemindersWidget::bindWhereParams(QSqlQuery& q) const
{
    bool    modeToday = ui->btnModeToday->isChecked();
    QString today     = QDate::currentDate().toString("yyyy-MM-dd");
    QString search    = ui->searchEdit->text().trimmed();
    q.bindValue(":today", today);

    int vaccineTypeId = ui->vaccineTypeCombo->currentData().toInt();
    if (vaccineTypeId > 0)
        q.bindValue(":vtid", vaccineTypeId);

    int animalTypeId = ui->animalTypeCombo->currentData().toInt();
    if (animalTypeId != -1)
        q.bindValue(":atype", animalTypeId);

    if (!modeToday) {
        if (m_subManualMode) {
            int days = m_daysSpin ? m_daysSpin->value() : 7;
            // from property instead of currentIndex
            bool isFuture = m_dirBtn ? m_dirBtn->property("isFuture").toBool() : true;
            if (isFuture) {
                // from today until n days later
                q.bindValue(":overdueFrom", today);
                q.bindValue(":overdueTo",
                            QDate::currentDate().addDays(days).toString("yyyy-MM-dd"));
            } else {
                // from n days ago to today
                q.bindValue(":overdueFrom",
                            QDate::currentDate().addDays(-days).toString("yyyy-MM-dd"));
                q.bindValue(":overdueTo", today);
            }
        } else {
            QDate from = m_pickerFrom ? m_pickerFrom->date() : QDate::currentDate();
            QDate to   = m_pickerTo   ? m_pickerTo->date()   : QDate::currentDate();
            q.bindValue(":overdueFrom", from.toString("yyyy-MM-dd"));
            q.bindValue(":overdueTo",   to.toString("yyyy-MM-dd"));
        }
    }
    if (!search.isEmpty()) {
        q.bindValue(":search",  "%" + search + "%");
        q.bindValue(":search2", "%" + search + "%");
        q.bindValue(":search3", "%" + search + "%");
        q.bindValue(":search4", "%" + search + "%");
        q.bindValue(":search5", "%" + search + "%");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Table — initial load (reset)
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::loadTable()
{
    auto* tbl = ui->remindersTable;
    tbl->setRowCount(0);
    tbl->setColumnCount(12); // +1: دکمه‌ی تمدید سریع واکسن
    tbl->setLayoutDirection(Qt::RightToLeft);

    QStringList headers = {
        "#", "شماره پرونده", "حیوان", "نوع", "صاحب", "شماره",
        "نوع واکسن", "تاریخ تزریق", "موعد یادآوری", "پیگیری شد", "پاسخ صاحب", "افزودن واکسن"
    };
    tbl->setHorizontalHeaderLabels(headers);

    auto* hv = tbl->horizontalHeader();
    hv->setLayoutDirection(Qt::RightToLeft);
    hv->setDefaultAlignment(Qt::AlignCenter);
    hv->setSectionResizeMode(QHeaderView::Stretch);
    hv->setSectionResizeMode(0,  QHeaderView::Fixed); // row number
    hv->setSectionResizeMode(9,  QHeaderView::Fixed); // پیگیری شد
    hv->setSectionResizeMode(10, QHeaderView::Fixed); // پاسخ صاحب
    tbl->setColumnWidth(0,  45);
    tbl->setColumnWidth(9,  110);
    tbl->setColumnWidth(10, 140);

    tbl->verticalHeader()->setVisible(false);
    tbl->setShowGrid(false);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setFocusPolicy(Qt::NoFocus);
    tbl->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tbl->viewport()->installEventFilter(this);
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

    // Remove old "Show More" button if it exists
    if (m_loadMoreBtn) {
        auto* outer = qobject_cast<QVBoxLayout*>(this->layout());
        if (outer) outer->removeWidget(m_loadMoreBtn);
        m_loadMoreBtn->deleteLater();
        m_loadMoreBtn = nullptr;
    }

    m_offset = 0;
    appendRows(0);
}


// ─────────────────────────────────────────────────────────────────────────────
// Table — append next page
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// eventFilter — handles clicks on animal/owner name labels in table
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// showToast — پیام کوچک "کپی شد" در پایین صفحه برای ~1.5 ثانیه
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::showToast(const QString& message)
{
    if (auto* old = findChild<QLabel*>("toastLabel"))
        old->deleteLater();

    auto* toast = new QLabel(message, this);
    toast->setObjectName("toastLabel");
    toast->setAlignment(Qt::AlignCenter);
    toast->setStyleSheet(R"(
        QLabel {
            background: rgba(33,33,33,210);
            color: white;
            border-radius: 8px;
            font-size: 12px;
            padding: 8px 20px;
        }
    )");
    toast->adjustSize();
    toast->setFixedHeight(36);

    int x = (width()  - toast->width())  / 2;
    int y =  height() - toast->height() - 20;
    toast->move(x, y);
    toast->raise();
    toast->show();

    QTimer::singleShot(1500, toast, &QLabel::deleteLater);
}

// ─────────────────────────────────────────────────────────────────────────────
// eventFilter
// ─────────────────────────────────────────────────────────────────────────────

bool RemindersWidget::eventFilter(QObject* obj, QEvent* ev)
{
    // ── viewport جدول: کلیک چپ → کپی مقدار ستون ─────────────────────────────
    if (obj == ui->remindersTable->viewport()
        && ev->type() == QEvent::MouseButtonPress)
    {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (me->button() == Qt::RightButton || me->button() == Qt::LeftButton) {
            auto* tbl = ui->remindersTable;
            int col = tbl->columnAt(me->pos().x());
            int row = tbl->rowAt(me->pos().y());
            if (row < 0 || col < 0)
                return QWidget::eventFilter(obj, ev);

            // ستون‌های کپی‌پذیر: 1=شماره پرونده، 5=شماره، 6=نوع واکسن، 7=تاریخ تزریق، 8=موعد یادآوری
            QString copyText;
            if (col == 1 || col == 5 || col == 6 || col == 7 || col == 8) {
                if (auto* lbl = qobject_cast<QLabel*>(tbl->cellWidget(row, col)))
                    copyText = lbl->text();
                else if (auto* item = tbl->item(row, col))
                    copyText = item->text();
            }

            if (!copyText.isEmpty()) {
                QApplication::clipboard()->setText(copyText);
                showToast("کپی شد ✓");
                return true;
            }
        }
        return QWidget::eventFilter(obj, ev);
    }

    // ── لیبل‌های لینک‌دار (حیوان / صاحب) ────────────────────────────────────
    auto* lbl = qobject_cast<QLabel*>(obj);
    if (!lbl) return QWidget::eventFilter(obj, ev);

    bool isLink = lbl->property("isAnimalLink").toBool()
                  || lbl->property("isOwnerLink").toBool();
    if (!isLink) return QWidget::eventFilter(obj, ev);

    if (ev->type() == QEvent::Enter) {
        lbl->setStyleSheet("color: #212121; text-decoration: underline; background: transparent;");
        return false;
    }
    if (ev->type() == QEvent::Leave) {
        lbl->setStyleSheet("color: #212121; background: transparent;");
        return false;
    }
    if (ev->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (me->button() == Qt::RightButton) {
            // ستون حیوان و صاحب: چپ کلیک → کپی (چون لینک navigate دارن)
            QApplication::clipboard()->setText(lbl->property("copyValue").toString());
            showToast("کپی شد ✓");
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void RemindersWidget::appendRows(int offset)
{
    QString sql =
        "SELECT DISTINCT rf.id AS rf_id, "
        "a.id AS animal_id, a.name AS animal_name, a.file_number, a.animal_type_id, "
        "o.id AS owner_id, "
        "CONCAT(o.first_name,' ',o.last_name) AS owner_name, o.phone, "
        "vt.name AS vaccine_name, v.vaccine_type_id, v.reminder_days, "
        "v.vaccinated_at, v.next_reminder_at, "
        "rf.is_followed_up, rf.owner_responded "
        "FROM reminder_followups rf "
        "JOIN vaccinations v ON rf.vaccination_id = v.id "
        "JOIN animals a ON v.animal_id = a.id "
        "JOIN owners o ON a.owner_id = o.id "
        "JOIN vaccine_types vt ON v.vaccine_type_id = vt.id "
        + buildWhereClause()
        + "ORDER BY v.next_reminder_at DESC, a.name ASC "
          "LIMIT :limit OFFSET :offset";

    QSqlQuery q;
    q.prepare(sql);
    bindWhereParams(q);
    q.bindValue(":limit",  kPageSize + 1);
    q.bindValue(":offset", offset);

    if (!q.exec()) return;

    auto* tbl = ui->remindersTable;
    int row = tbl->rowCount();
    int fetched = 0;

    auto makeItem = [](const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    while (q.next()) {
        fetched++;
        if (fetched > kPageSize) break;

        int     rfId         = q.value("rf_id").toInt();
        int     animalId     = q.value("animal_id").toInt();
        int     ownerId      = q.value("owner_id").toInt();
        QString animalName   = q.value("animal_name").toString();
        QString fileNumber   = q.value("file_number").toString();
        auto    ti           = AnimalTypeInfo::get(q.value("animal_type_id").toInt());
        QString ownerName    = q.value("owner_name").toString();
        QString phone        = q.value("phone").toString();
        QString vaccineName  = q.value("vaccine_name").toString();
        int     vaccineTypeId= q.value("vaccine_type_id").toInt();
        int     reminderDays = q.value("reminder_days").toInt();
        QDate   vacDate      = q.value("vaccinated_at").toDate();
        QDate   reminderDate = q.value("next_reminder_at").toDate();
        bool    isFollowedUp = q.value("is_followed_up").toBool();
        QVariant ownerResp   = q.value("owner_responded");

        tbl->insertRow(row);
        tbl->setRowHeight(row, 46);

        // Col 0: row number
        tbl->setItem(row, 0, makeItem(QString::number(row + 1)));

        // Col 1: file number
        tbl->setItem(row, 1, makeItem(fileNumber));

        // Col 2: animal name — clickable
        auto* animalLbl = new QLabel(animalName);
        animalLbl->setAlignment(Qt::AlignCenter);
        animalLbl->setCursor(Qt::PointingHandCursor);
        animalLbl->setStyleSheet(
            "color: #212121; background: transparent;");
        animalLbl->installEventFilter(this);
        animalLbl->setProperty("animalId",      animalId);
        animalLbl->setProperty("isAnimalLink",  true);
        {
            auto* w   = new QWidget;
            auto* lay = new QVBoxLayout(w);
            lay->setContentsMargins(4, 0, 4, 0);
            lay->addWidget(animalLbl);
            w->setStyleSheet("background: transparent;");
            tbl->setCellWidget(row, 2, w);
        }

        // Col 3: animal type badge
        tbl->setCellWidget(row, 3, makeBadge(ti.name, ti.badgeBg, ti.badgeFg));

        // Col 4: owner name — clickable
        auto* ownerLbl = new QLabel(ownerName);
        ownerLbl->setAlignment(Qt::AlignCenter);
        ownerLbl->setCursor(Qt::PointingHandCursor);
        ownerLbl->setStyleSheet(
            "color: #212121; background: transparent;");
        ownerLbl->installEventFilter(this);
        ownerLbl->setProperty("ownerId",       ownerId);
        ownerLbl->setProperty("isOwnerLink",   true);
        {
            auto* w   = new QWidget;
            auto* lay = new QVBoxLayout(w);
            lay->setContentsMargins(4, 0, 4, 0);
            lay->addWidget(ownerLbl);
            w->setStyleSheet("background: transparent;");
            tbl->setCellWidget(row, 4, w);
        }

        // Col 5: phone
        tbl->setItem(row, 5, makeItem(phone));

        // Col 6: vaccine name
        tbl->setItem(row, 6, makeItem(vaccineName));

        // Col 7: vaccination date
        tbl->setItem(row, 7, makeItem(PersianDate::toDisplayShort(vacDate)));

        // Col 8: reminder date
        tbl->setItem(row, 8, makeItem(PersianDate::toDisplayShort(reminderDate)));

        // Col 9: follow-up checkbox
        auto* cbContainer = new QWidget;
        cbContainer->setStyleSheet("background: transparent;");
        auto* cbLay = new QHBoxLayout(cbContainer);
        cbLay->setContentsMargins(0, 0, 0, 0);
        cbLay->setAlignment(Qt::AlignCenter);
        auto* cb = new QCheckBox;
        cb->setChecked(isFollowedUp);
        cbLay->addWidget(cb);
        connect(cb, &QCheckBox::toggled, this, [this, rfId](bool checked) {
            onFollowUpChanged(rfId, checked);
        });
        tbl->setCellWidget(row, 9, cbContainer);

        // Col 10: owner response combo
        int currentResp = ownerResp.isNull() ? 0 : (ownerResp.toBool() ? 1 : 2);
        tbl->setCellWidget(row, 10, makeResponseCombo(rfId, currentResp));

        // Col 11: دکمه‌ی «+» برای تمدید سریع همین واکسن (برای همه‌ی رکوردها)
        {
            auto* btnAdd = new QPushButton;
            btnAdd->setIcon(QIcon(":/icons/plus.svg"));
            btnAdd->setIconSize(QSize(16, 16));
            btnAdd->setFixedSize(28, 28);
            btnAdd->setCursor(Qt::PointingHandCursor);
            btnAdd->setToolTip("ثبت واکسن جدید (تمدید)");
            btnAdd->setStyleSheet(R"(
                QPushButton {
                    background: #E8F5E9; border: none; border-radius: 6px;
                }
                QPushButton:hover { background: #C8E6C9; }
            )");
            connect(btnAdd, &QPushButton::clicked, this,
                    [this, animalId, vaccineTypeId, reminderDays]() {
                        AddVaccineDialog dlg(animalId, this);
                        dlg.prefillFrom(vaccineTypeId, reminderDays);
                        if (dlg.exec() == QDialog::Accepted)
                            reloadPreservingState();
                    });
            auto* w   = new QWidget;
            auto* lay = new QHBoxLayout(w);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setAlignment(Qt::AlignCenter);
            lay->addWidget(btnAdd);
            w->setStyleSheet("background: transparent;");
            tbl->setCellWidget(row, 11, w);
        }

        row++;
    }

    m_offset = offset + fetched;
    bool hasMore = (fetched > kPageSize);

    if (fetched > kPageSize) {
        hasMore = true;
        m_offset = offset + kPageSize;
    } else {
        hasMore = false;
    }

    tbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Manage "Show More" button — added to outerLayout (page level)
    // to prevent table resize when button is added/removed
    auto* outerLayout = qobject_cast<QVBoxLayout*>(this->layout());

    if (hasMore) {
        if (!m_loadMoreBtn) {
            m_loadMoreBtn = new QPushButton(
                QString("نمایش بیشتر  (در حال نمایش %1 ردیف)").arg(m_offset));
            m_loadMoreBtn->setStyleSheet(R"(
                QPushButton {
                    background: #F1F8E9;
                    border: 0.5px solid #C8E6C9;
                    border-radius: 10px;
                    color: #2E7D32;
                    font-size: 12px;
                    font-weight: 500;
                    padding: 10px;
                }
                QPushButton:hover { background: #E8F5E9; }
                QPushButton:pressed { background: #C8E6C9; }
            )");
            if (outerLayout) outerLayout->addWidget(m_loadMoreBtn);
            connect(m_loadMoreBtn, &QPushButton::clicked,
                    this, &RemindersWidget::onLoadMoreClicked);
        } else {
            m_loadMoreBtn->setText(
                QString("نمایش بیشتر  (در حال نمایش %1 ردیف)").arg(m_offset));
        }
    } else {
        // All records loaded — remove button
        if (m_loadMoreBtn) {
            if (outerLayout) outerLayout->removeWidget(m_loadMoreBtn);
            m_loadMoreBtn->deleteLater();
            m_loadMoreBtn = nullptr;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper widgets
// ─────────────────────────────────────────────────────────────────────────────

QWidget* RemindersWidget::makeBadge(const QString& text,
                                    const QString& bg,
                                    const QString& fg)
{
    auto* container = new QWidget;
    container->setStyleSheet("background: transparent;");
    auto* lay = new QHBoxLayout(container);
    lay->setContentsMargins(4, 2, 4, 2);
    lay->setAlignment(Qt::AlignCenter);

    auto* lbl = new QLabel(text);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lbl->setStyleSheet(QString(
                           "background:%1;color:%2;border-radius:4px;"
                           "font-size:11px;font-weight:500;padding:2px 8px;"
                           ).arg(bg, fg));
    lay->addWidget(lbl);
    return container;
}

QWidget* RemindersWidget::makeResponseCombo(int rfId, int currentResponse)
{
    // outer: fills the entire cell
    auto* container = new QWidget;
    container->setStyleSheet("background: transparent;");
    auto* outerLay = new QVBoxLayout(container);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->setSpacing(0);
    outerLay->setAlignment(Qt::AlignVCenter);

    // inner: holds the combo
    auto* inner = new QWidget;
    inner->setStyleSheet("background: transparent;");
    auto* lay = new QHBoxLayout(inner);
    lay->setContentsMargins(4, 0, 4, 0);
    lay->setSpacing(0);
    lay->setAlignment(Qt::AlignCenter);
    outerLay->addWidget(inner, 0, Qt::AlignVCenter);

    auto* combo = new QComboBox;
    combo->setFixedSize(110, 29); // fixed size — key to vertical centering
    combo->addItem("در انتظار");
    combo->addItem("جواب داد");
    combo->addItem("جواب نداد");
    combo->setCurrentIndex(currentResponse);
    combo->setStyleSheet(R"(
        QComboBox {
            background: #F5F5F5;
            border: 0.5px solid #E0E0E0;
            border-radius: 6px;
            padding: 3px 8px;
            font-size: 11px;
            color: #212121;
        }
        QComboBox:hover { border-color: #2E7D32; }
        QComboBox::drop-down { border: none; width: 20px; }
        QComboBox::down-arrow {
            image: url(:/icons/chevron-down.svg);
            width: 7px; height: 7px;
        }
        QComboBox QAbstractItemView {
            background: white;
            border: 0.5px solid #C8E6C9;
            font-size: 11px;
            selection-background-color: #E8F5E9;
            selection-color: #2E7D32;
            outline: none;
        }
        QComboBox QAbstractItemView::item { padding: 6px 10px; min-height: 26px; }
    )");

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, rfId](int idx) {
                onOwnerResponseChanged(rfId, idx);
            });

    lay->addWidget(combo);
    return container;
}

// ─────────────────────────────────────────────────────────────────────────────
// DB update slots
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::onFollowUpChanged(int rfId, bool checked)
{
    QSqlQuery q;
    q.prepare("UPDATE reminder_followups SET "
              "is_followed_up = :val, "
              "followed_up_at = CASE WHEN :val2 = 1 THEN NOW() ELSE NULL END "
              "WHERE id = :id");
    q.bindValue(":val",  checked ? 1 : 0);
    q.bindValue(":val2", checked ? 1 : 0);
    q.bindValue(":id",   rfId);
    q.exec();
    loadStats();
}

void RemindersWidget::onOwnerResponseChanged(int rfId, int responseIndex)
{
    QSqlQuery q;
    if (responseIndex == 0) {
        q.prepare("UPDATE reminder_followups SET owner_responded = NULL WHERE id = :id");
    } else {
        q.prepare("UPDATE reminder_followups SET owner_responded = :val WHERE id = :id");
        q.bindValue(":val", responseIndex == 1 ? 1 : 0);
    }
    q.bindValue(":id", rfId);
    q.exec();
    loadStats();
}

// ─────────────────────────────────────────────────────────────────────────────
// PDF Export — Independent query without LIMIT, all records
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::onExportPdfClicked()
{
    QString path = QFileDialog::getSaveFileName(
        this, "ذخیره PDF", "یادآوری‌ها.pdf", "PDF Files (*.pdf)");
    if (path.isEmpty()) return;

    // Full query — no LIMIT, all records matching current filters
    QString sql =
        "SELECT DISTINCT rf.id AS rf_id, "
        "a.name AS animal_name, a.file_number, a.animal_type_id, "
        "CONCAT(o.first_name,' ',o.last_name) AS owner_name, o.phone, "
        "vt.name AS vaccine_name, v.vaccinated_at, v.next_reminder_at, "
        "rf.is_followed_up, rf.owner_responded "
        "FROM reminder_followups rf "
        "JOIN vaccinations v ON rf.vaccination_id = v.id "
        "JOIN animals a ON v.animal_id = a.id "
        "JOIN owners o ON a.owner_id = o.id "
        "JOIN vaccine_types vt ON v.vaccine_type_id = vt.id "
        + buildWhereClause()
        + "ORDER BY v.next_reminder_at DESC, a.name ASC";

    QSqlQuery q;
    q.prepare(sql);
    bindWhereParams(q);

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا", "خطا در دریافت داده‌ها.");
        return;
    }

    // ── Build filter description for PDF header ───────────────────────────────
    QString filterDesc;
    if (m_subManualMode) {
        int  days     = m_daysSpin ? m_daysSpin->value() : 7;
        bool isFuture = m_dirBtn ? m_dirBtn->property("isFuture").toBool() : true;
        filterDesc = QString("%1 روز %2").arg(days).arg(isFuture ? "آینده" : "گذشته");
    } else {
        QDate from = m_pickerFrom ? m_pickerFrom->date() : QDate::currentDate();
        QDate to   = m_pickerTo   ? m_pickerTo->date()   : QDate::currentDate();
        if (from == to)
            filterDesc = PersianDate::toDisplayShort(from);
        else
            filterDesc = PersianDate::toDisplayShort(from) + " تا " +
                         PersianDate::toDisplayShort(to);
    }

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);

    QPainter painter(&writer);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont titleFont ("Arial", 16, QFont::Bold);
    QFont subFont   ("Arial", 10);
    QFont headerFont("Arial",  9, QFont::Bold);
    QFont cellFont  ("Arial",  8);

    int pageW = writer.width();
    int y     = 80;

    // Title
    painter.setFont(titleFont);
    painter.setPen(QColor("#1B5E20"));
    painter.drawText(QRect(0, y, pageW, 220), Qt::AlignCenter,
                     "لیست یادآوری‌های واکسیناسیون");
    y += 260;

    // Filter description
    painter.setFont(subFont);
    painter.setPen(QColor("#555555"));
    painter.drawText(QRect(0, y, pageW, 160), Qt::AlignCenter,
                     "بازه زمانی: " + filterDesc);
    y += 220;

    // Columns — reversed order (PDF is LTR but we want RTL appearance)
    // App order: #, Case No., Animal, Type, Owner, Number, Vaccine Type,
    // Injection Date, Reminder Due Date, Followed, Owner Response
    // PDF reversed: Owner Response, Followed, Reminder Due Date, ...,
    struct ColDef { QString header; int weight; };
    QList<ColDef> colDefs = {
        {"پاسخ صاحب",        2},
        {"پیگیری شد",        2},
        {"موعد یادآوری",     2},
        {"تاریخ تزریق",      2},
        {"نوع واکسن",        3},
        {"شماره",            2},
        {"صاحب",             3},
        {"نوع",              2},
        {"حیوان",            2},
        {"شماره پرونده",     2},
        {"#",                1}
    };

    int totalWeight = 0;
    for (const auto& c : colDefs) totalWeight += c.weight;
    QVector<int> colWidths, colX;
    int cx = 0;
    for (const auto& c : colDefs) {
        int w = (pageW * c.weight) / totalWeight;
        colWidths.append(w);
        colX.append(cx);
        cx += w;
    }

    int rowH = 200;

    // Draw header row
    auto drawHeader = [&]() {
        painter.setFont(headerFont);
        painter.fillRect(0, y, pageW, rowH, QColor("#E8F5E9"));
        painter.setPen(QColor("#1B5E20"));
        for (int i = 0; i < colDefs.size(); i++)
            painter.drawText(QRect(colX[i], y, colWidths[i], rowH),
                             Qt::AlignCenter, colDefs[i].header);
        y += rowH;
    };

    drawHeader();

    int rowNum = 0;
    while (q.next()) {
        if (y + rowH > writer.height() - 150) {
            writer.newPage();
            y = 80;
            drawHeader();
        }

        // Alternating background
        painter.fillRect(0, y, pageW, rowH,
                         rowNum % 2 == 0 ? QColor("#FAFAFA") : Qt::white);

        auto     ti        = AnimalTypeInfo::get(q.value("animal_type_id").toInt());
        QVariant ownerResp = q.value("owner_responded");
        int      respIdx   = ownerResp.isNull() ? 0 : (ownerResp.toBool() ? 1 : 2);
        QStringList respLabels = {"در انتظار", "جواب داد", "جواب نداد"};
        bool isFollowedUp = q.value("is_followed_up").toBool();

        // Cell data — same reversed order as colDefs
        struct Cell { QString text; QColor color; bool bold; };
        QList<Cell> cells = {
            {respLabels[respIdx],                                        Qt::black,           false},
            {isFollowedUp ? "بله" : "خیر",
             isFollowedUp ? QColor("#2E7D32") : QColor("#C62828"),       true},
            {PersianDate::toDisplayShort(q.value("next_reminder_at").toDate()), Qt::black,   false},
            {PersianDate::toDisplayShort(q.value("vaccinated_at").toDate()),    Qt::black,   false},
            {q.value("vaccine_name").toString(),                         Qt::black,           false},
            {q.value("phone").toString(),                                Qt::black,           false},
            {q.value("owner_name").toString(),                           Qt::black,           false},
            {ti.name,                                                    QColor(ti.badgeFg),  false},
            {q.value("animal_name").toString(),                          Qt::black,           false},
            {q.value("file_number").toString(),                          Qt::black,           false},
            {QString::number(rowNum + 1),                                Qt::black,           false}
        };

        for (int i = 0; i < cells.size(); i++) {
            painter.setFont(cells[i].bold
                                ? QFont("Arial", 8, QFont::Bold)
                                : cellFont);
            painter.setPen(cells[i].color);
            painter.drawText(QRect(colX[i], y, colWidths[i], rowH),
                             Qt::AlignCenter, cells[i].text);
        }

        // Grid line
        painter.setPen(QColor("#E0E0E0"));
        painter.drawLine(0, y + rowH - 1, pageW, y + rowH - 1);

        y += rowH;
        rowNum++;
    }

    painter.end();

    StyledMessageBox::success(this, "موفق",
                              QString("فایل PDF با موفقیت ذخیره شد.\n%1 ردیف چاپ شد.").arg(rowNum));
}