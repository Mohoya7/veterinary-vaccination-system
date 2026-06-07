#include "reminderswidget.h"
#include "animaltypeinfo.h"
#include <QRadioButton>
#include <QButtonGroup>
#include "ui_reminderswidget.h"
#include "addvaccinedialog.h"
#include "persiandate.h"

#include <QSqlQuery>
#include <QDate>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QScrollBar>
#include <QPainter>
#include <QPdfWriter>
#include <QFileDialog>
#include <QMessageBox>

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

    connect(ui->searchEdit,      &QLineEdit::textChanged,
            this, &RemindersWidget::onFiltersChanged);
    connect(ui->animalTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                loadVaccineTypeCombo();
                onFiltersChanged();
            });
    connect(ui->vaccineTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RemindersWidget::onFiltersChanged);

    // ── ساخت widget فیلتر تاریخ ──────────────────────────────────────────────
    buildFilterDateWidget();

    // لود اولیه واکسن‌ها
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

// ── loadVaccineTypeCombo ─────────────────────────────────────────────────────
void RemindersWidget::loadVaccineTypeCombo()
{
    int animalTypeIdx = ui->animalTypeCombo->currentIndex();

    ui->vaccineTypeCombo->blockSignals(true);
    ui->vaccineTypeCombo->clear();
    ui->vaccineTypeCombo->addItem("همه واکسن‌ها", -1);

    QSqlQuery q;
    if (animalTypeIdx == 0) {
        q.prepare("SELECT id, name FROM vaccine_types ORDER BY name");
    } else {
        q.prepare(
            "SELECT DISTINCT vt.id, vt.name "
            "FROM vaccine_types vt "
            "JOIN vaccine_type_animals vta ON vta.vaccine_type_id = vt.id "
            "WHERE vta.animal_type_id = :atid "
            "ORDER BY vt.name");
        q.bindValue(":atid", animalTypeIdx);
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

    // ── کامبو انتخاب حالت (بر اساس روز / بازه تاریخ) ────────────────────
    m_subModeCombo = new QComboBox;
    m_subModeCombo->addItem("بر اساس روز", 0);   // index 0 = حالت دستی
    m_subModeCombo->addItem("بازه تاریخ",  1);   // index 1 = بازه
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

    // ── ستون دستی (بر اساس روز) ──────────────────────────────────────────
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

    // ── toggle button آینده/گذشته ─────────────────────────────────────────
    m_dirBtn = new QPushButton("روز آینده");
    m_dirBtn->setProperty("isFuture", true);   // دیفالت: روز آینده
    m_dirBtn->setFixedHeight(32);
    m_dirBtn->setFixedWidth(88);               // عرض کمتر
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
    // کلیک → تغییر حالت
    connect(m_dirBtn, &QPushButton::clicked, this, [this]() {
        bool wasFuture = m_dirBtn->property("isFuture").toBool();
        m_dirBtn->setProperty("isFuture", !wasFuture);
        m_dirBtn->setText(!wasFuture ? "روز آینده" : "روز گذشته");
        onFiltersChanged();
    });

    manualLay->addWidget(m_daysSpin);
    manualLay->addWidget(m_dirBtn);
    mainLay->addWidget(manualWidget);

    // ── ستون بازه تاریخ ──────────────────────────────────────────────────
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

    // ── جایگزاری در container ─────────────────────────────────────────────
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
    bool    modeToday     = ui->btnModeToday->isChecked();
    QString search        = ui->searchEdit->text().trimmed();
    int     animalTypeIdx = ui->animalTypeCombo->currentIndex();
    int     followUpIdx   = ui->followUpCombo->currentIndex();
    int     responseIdx   = ui->responseCombo->currentIndex();

    QString w = "WHERE rf.is_resolved = FALSE ";

    if (modeToday)
        w += "AND v.next_reminder_at = :today ";
    else
        w += "AND v.next_reminder_at >= :overdueFrom "
             "AND v.next_reminder_at <= :overdueTo ";

    if (!search.isEmpty())
        w += "AND (a.name LIKE :search OR o.phone LIKE :search2 "
             "OR CONCAT(o.first_name,' ',o.last_name) LIKE :search3) ";

    if (animalTypeIdx == 1) w += "AND a.animal_type_id = 1 ";
    else if (animalTypeIdx == 2) w += "AND a.animal_type_id = 2 ";

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

    if (!modeToday) {
        if (m_subManualMode) {
            int days = m_daysSpin ? m_daysSpin->value() : 7;
            // ✅ از property به جای currentIndex
            bool isFuture = m_dirBtn ? m_dirBtn->property("isFuture").toBool() : true;
            if (isFuture) {
                // از امروز تا n روز بعد
                q.bindValue(":overdueFrom", today);
                q.bindValue(":overdueTo",
                            QDate::currentDate().addDays(days).toString("yyyy-MM-dd"));
            } else {
                // از n روز قبل تا امروز
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
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Table — initial load (reset)
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::loadTable()
{
    auto* tbl = ui->remindersTable;
    tbl->setRowCount(0);
    tbl->setColumnCount(8);
    tbl->setLayoutDirection(Qt::RightToLeft);

    QStringList headers = {
        "حیوان", "نوع", "صاحب", "شماره",
        "نوع واکسن", "موعد یادآوری", "پیگیری شد", "پاسخ صاحب"
    };
    tbl->setHorizontalHeaderLabels(headers);

    auto* hv = tbl->horizontalHeader();
    hv->setLayoutDirection(Qt::RightToLeft);
    hv->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hv->setSectionResizeMode(QHeaderView::Stretch);
    hv->setSectionResizeMode(6, QHeaderView::Fixed);
    hv->setSectionResizeMode(7, QHeaderView::Fixed);
    tbl->setColumnWidth(6, 110);
    tbl->setColumnWidth(7, 140);

    tbl->verticalHeader()->setVisible(false);
    tbl->setShowGrid(false);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setFocusPolicy(Qt::NoFocus);
    tbl->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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
        auto* cardLayout = qobject_cast<QVBoxLayout*>(ui->tableCard->layout());
        if (cardLayout) cardLayout->removeWidget(m_loadMoreBtn);
        m_loadMoreBtn->deleteLater();
        m_loadMoreBtn = nullptr;
    }

    m_offset = 0;
    appendRows(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Table — append next page
// ─────────────────────────────────────────────────────────────────────────────

void RemindersWidget::appendRows(int offset)
{
    QString sql =
        "SELECT DISTINCT rf.id AS rf_id, a.name AS animal_name, a.animal_type_id, "
        "CONCAT(o.first_name,' ',o.last_name) AS owner_name, o.phone, "
        "vt.name AS vaccine_name, v.next_reminder_at, "
        "rf.is_followed_up, rf.owner_responded "
        "FROM reminder_followups rf "
        "JOIN vaccinations v ON rf.vaccination_id = v.id "
        "JOIN animals a ON v.animal_id = a.id "
        "JOIN owners o ON a.owner_id = o.id "
        "JOIN vaccine_types vt ON v.vaccine_type_id = vt.id "
        + buildWhereClause()
        + "ORDER BY v.next_reminder_at ASC "
          "LIMIT :limit OFFSET :offset";

    QSqlQuery q;
    q.prepare(sql);
    bindWhereParams(q);
    q.bindValue(":limit",  kPageSize + 1); // Fetch one extra to determine if there is a next page
    q.bindValue(":offset", offset);

    if (!q.exec()) return;

    auto* tbl = ui->remindersTable;
    int row = tbl->rowCount();
    int fetched = 0;

    auto makeItem = [](const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        return item;
    };

    while (q.next()) {
        fetched++;
        if (fetched > kPageSize) break; // Do not display the extra record

        int     rfId         = q.value("rf_id").toInt();
        QString animalName   = q.value("animal_name").toString();
        auto    ti           = AnimalTypeInfo::get(q.value("animal_type_id").toInt());
        QString ownerName    = q.value("owner_name").toString();
        QString phone        = q.value("phone").toString();
        QString vaccineName  = q.value("vaccine_name").toString();
        QDate   reminderDate = q.value("next_reminder_at").toDate();
        bool    isFollowedUp = q.value("is_followed_up").toBool();
        QVariant ownerResp   = q.value("owner_responded");

        tbl->insertRow(row);
        tbl->setRowHeight(row, 46);

        tbl->setItem(row, 0, makeItem(animalName));
        tbl->setCellWidget(row, 1, makeBadge(
                                       ti.name,
                                       ti.badgeBg,
                                       ti.badgeFg));
        tbl->setItem(row, 2, makeItem(ownerName));
        tbl->setItem(row, 3, makeItem(phone));
        tbl->setItem(row, 4, makeItem(vaccineName));
        tbl->setItem(row, 5, makeItem(PersianDate::toDisplayShort(reminderDate)));

        // Follow-up checkbox
        auto* cbContainer = new QWidget;
        cbContainer->setStyleSheet("background: transparent;");
        auto* cbLay = new QHBoxLayout(cbContainer);
        cbLay->setContentsMargins(0, 0, 0, 0);
        cbLay->setAlignment(Qt::AlignLeft);
        auto* cb = new QCheckBox;
        cb->setChecked(isFollowedUp);
        cbLay->addWidget(cb);
        connect(cb, &QCheckBox::toggled, this, [this, rfId](bool checked) {
            onFollowUpChanged(rfId, checked);
        });
        tbl->setCellWidget(row, 6, cbContainer);

        // Owner response combo
        int currentResp = ownerResp.isNull() ? 0 : (ownerResp.toBool() ? 1 : 2);
        tbl->setCellWidget(row, 7, makeResponseCombo(rfId, currentResp));

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

    // Manage "Show More" button
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
            connect(m_loadMoreBtn, &QPushButton::clicked,
                    this, &RemindersWidget::onLoadMoreClicked);
        } else {
            m_loadMoreBtn->setText(
                QString("نمایش بیشتر  (در حال نمایش %1 ردیف)").arg(m_offset));
        }
    } else {
        // All records loaded — remove button
        if (m_loadMoreBtn) {
            if (cardLayout) cardLayout->removeWidget(m_loadMoreBtn);
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
    lay->setContentsMargins(8, 2, 8, 2);
    lay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* lbl = new QLabel(text);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lbl->setStyleSheet(QString(
                           "background:%1;color:%2;border-radius:4px;"
                           "font-size:11px;font-weight:500;padding:2px 8px;"
                           ).arg(bg, fg));
    lay->addWidget(lbl);
    lay->addStretch();
    return container;
}

QWidget* RemindersWidget::makeResponseCombo(int rfId, int currentResponse)
{
    auto* container = new QWidget;
    container->setStyleSheet("background: transparent;");
    auto* lay = new QHBoxLayout(container);
    lay->setContentsMargins(4, 4, 4, 4);

    auto* combo = new QComboBox;
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
            min-height: 26px;
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
        this, "Save PDF", "Reminders.pdf", "PDF Files (*.pdf)");
    if (path.isEmpty()) return;

    QString sql =
        "SELECT DISTINCT a.name AS animal_name, a.animal_type_id, "
        "CONCAT(o.first_name,' ',o.last_name) AS owner_name, o.phone, "
        "vt.name AS vaccine_name, v.next_reminder_at, "
        "rf.is_followed_up, rf.owner_responded "
        "FROM reminder_followups rf "
        "JOIN vaccinations v ON rf.vaccination_id = v.id "
        "JOIN animals a ON v.animal_id = a.id "
        "JOIN owners o ON a.owner_id = o.id "
        "JOIN vaccine_types vt ON v.vaccine_type_id = vt.id "
        + buildWhereClause()
        + "ORDER BY v.next_reminder_at ASC";
    // ← Without LIMIT — all records

    QSqlQuery q;
    q.prepare(sql);
    bindWhereParams(q);

    if (!q.exec()) {
        QMessageBox::critical(this, "Error", "Error retrieving data.");
        return;
    }

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    QPainter painter(&writer);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont titleFont("Arial", 14, QFont::Bold);
    QFont headerFont("Arial", 9, QFont::Bold);
    QFont cellFont("Arial", 8);

    int pageW = writer.width();
    int y = 100;

    painter.setFont(titleFont);
    painter.drawText(QRect(0, y, pageW, 200), Qt::AlignCenter, "Vaccination Reminder List");
    y += 260;

    painter.setFont(cellFont);
    painter.drawText(QRect(0, y, pageW, 150), Qt::AlignCenter,
                     "Date: " + PersianDate::todayDisplay());
    y += 220;

    QStringList cols = {"Animal", "Owner", "Phone", "Vaccine", "Reminder Date", "Follow-up"};
    int colW = pageW / cols.size();
    int rowH = 180;

    painter.setFont(headerFont);
    painter.fillRect(0, y, pageW, rowH, QColor("#E8F5E9"));
    painter.setPen(QColor("#1B5E20"));
    for (int i = 0; i < cols.size(); i++)
        painter.drawText(QRect(i * colW, y, colW, rowH), Qt::AlignCenter, cols[i]);
    y += rowH;

    painter.setFont(cellFont);
    painter.setPen(Qt::black);
    int rowNum = 0;
    while (q.next()) {
        if (y + rowH > writer.height() - 200) {
            writer.newPage();
            y = 100;
        }
        if (rowNum % 2 == 0)
            painter.fillRect(0, y, pageW, rowH, QColor("#FAFAFA"));

        QStringList rowData = {
            q.value("animal_name").toString(),
            q.value("owner_name").toString(),
            q.value("phone").toString(),
            q.value("vaccine_name").toString(),
            PersianDate::toDisplayShort(q.value("next_reminder_at").toDate()),
            q.value("is_followed_up").toBool() ? "Followed up" : "Pending"
        };
        for (int i = 0; i < rowData.size(); i++)
            painter.drawText(QRect(i * colW, y, colW, rowH), Qt::AlignCenter, rowData[i]);

        y += rowH;
        rowNum++;
    }
    painter.end();

    QMessageBox::information(this, "Success",
                             QString("PDF file saved successfully.\n%1 rows printed.").arg(rowNum));
}