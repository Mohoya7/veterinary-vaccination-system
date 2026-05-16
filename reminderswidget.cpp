#include "reminderswidget.h"
#include "ui_reminderswidget.h"
#include "addvaccinedialog.h"

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
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>

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
            this, &RemindersWidget::onFiltersChanged);
    connect(ui->followUpCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RemindersWidget::onFiltersChanged);
    connect(ui->responseCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RemindersWidget::onFiltersChanged);
    connect(ui->overdueDaysSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RemindersWidget::onFiltersChanged);
    connect(ui->btnExportPdf,    &QPushButton::clicked,
            this, &RemindersWidget::onExportPdfClicked);

    connect(ui->btnModeToday, &QPushButton::clicked, this, [this]() {
        ui->btnModeToday->setChecked(true);
        ui->btnModeOverdue->setChecked(false);
        ui->overdueDaysLabel->setVisible(false);
        ui->overdueDaysSpin->setVisible(false);
        onFiltersChanged();
    });

    connect(ui->btnModeOverdue, &QPushButton::clicked, this, [this]() {
        ui->btnModeOverdue->setChecked(true);
        ui->btnModeToday->setChecked(false);
        ui->overdueDaysLabel->setVisible(true);
        ui->overdueDaysSpin->setVisible(true);
        onFiltersChanged();
    });

    loadData();
}

RemindersWidget::~RemindersWidget() { delete ui; }

void RemindersWidget::loadData()
{
    loadStats();
    loadTable();
}

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
        QComboBox::drop-down { border: none; width: 28px; }
        QComboBox::down-arrow {
            image: url(:/icons/chevron-down.svg);
            width: 8px; height: 8px;
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
            font-size: 11px;
            color: #757575;
            background: transparent;
        }
        QLabel#lblTodayCount {
            font-size: 22px;
            font-weight: 500;
            color: #F9A825;
            background: transparent;
        }
        QLabel#lblOverdueCount {
            font-size: 22px;
            font-weight: 500;
            color: #C62828;
            background: transparent;
        }
        QLabel#lblDoneCount {
            font-size: 22px;
            font-weight: 500;
            color: #2E7D32;
            background: transparent;
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
        QCheckBox {
            spacing: 0px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
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

void RemindersWidget::onFiltersChanged() { loadData(); }

void RemindersWidget::loadStats()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    QSqlQuery q;

    // Today count
    q.prepare("SELECT COUNT(DISTINCT v.id) FROM vaccinations v "
              "JOIN reminder_followups rf ON rf.vaccination_id = v.id "
              "WHERE v.next_reminder_at = :today "
              "AND rf.is_resolved = FALSE AND v.is_deleted = FALSE");
    q.bindValue(":today", today);
    q.exec();
    if (q.next()) ui->lblTodayCount->setText(q.value(0).toString());

    // Overdue count
    q.prepare("SELECT COUNT(DISTINCT v.id) FROM vaccinations v "
              "JOIN reminder_followups rf ON rf.vaccination_id = v.id "
              "WHERE v.next_reminder_at < :today "
              "AND rf.is_resolved = FALSE AND v.is_deleted = FALSE");
    q.bindValue(":today", today);
    q.exec();
    if (q.next()) ui->lblOverdueCount->setText(q.value(0).toString());

    // Done today count
    q.prepare("SELECT COUNT(DISTINCT v.id) FROM vaccinations v "
              "JOIN reminder_followups rf ON rf.vaccination_id = v.id "
              "WHERE v.next_reminder_at = :today "
              "AND rf.is_followed_up = TRUE AND v.is_deleted = FALSE");
    q.bindValue(":today", today);
    q.exec();
    if (q.next()) ui->lblDoneCount->setText(q.value(0).toString());
}

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
            background: transparent;
            width: 6px;
            border-radius: 3px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: rgba(100,170,80,180);
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: rgba(46,125,50,220); }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical { background: transparent; }
    )");

    QString today     = QDate::currentDate().toString("yyyy-MM-dd");
    bool    modeToday = ui->btnModeToday->isChecked();
    int     overdueDays = ui->overdueDaysSpin->value();
    QString search    = ui->searchEdit->text().trimmed();
    int     animalTypeIdx = ui->animalTypeCombo->currentIndex();
    int     followUpIdx   = ui->followUpCombo->currentIndex();
    int     responseIdx   = ui->responseCombo->currentIndex();

    QString sql =
        "SELECT DISTINCT rf.id AS rf_id, a.name AS animal_name, a.type AS animal_type, "
        "CONCAT(o.first_name,' ',o.last_name) AS owner_name, o.phone, "
        "vt.name AS vaccine_name, v.next_reminder_at, "
        "rf.is_followed_up, rf.owner_responded "
        "FROM reminder_followups rf "
        "JOIN vaccinations v ON rf.vaccination_id = v.id "
        "JOIN animals a ON v.animal_id = a.id "
        "JOIN owners o ON a.owner_id = o.id "
        "JOIN vaccine_types vt ON v.vaccine_type_id = vt.id "
        "WHERE rf.is_resolved = FALSE AND v.is_deleted = FALSE ";

    if (modeToday)
        sql += "AND v.next_reminder_at = :today ";
    else {
        sql += "AND v.next_reminder_at >= :overdueFrom "
               "AND v.next_reminder_at < :today ";
    }

    if (!search.isEmpty())
        sql += "AND (a.name LIKE :search OR o.phone LIKE :search2 "
               "OR CONCAT(o.first_name,' ',o.last_name) LIKE :search3) ";
    if (animalTypeIdx == 1) sql += "AND a.type = 'dog' ";
    else if (animalTypeIdx == 2) sql += "AND a.type = 'cat' ";
    if (followUpIdx == 1) sql += "AND rf.is_followed_up = FALSE ";
    else if (followUpIdx == 2) sql += "AND rf.is_followed_up = TRUE ";
    if (responseIdx == 1) sql += "AND rf.owner_responded IS NULL ";
    else if (responseIdx == 2) sql += "AND rf.owner_responded = TRUE ";
    else if (responseIdx == 3) sql += "AND rf.owner_responded = FALSE ";

    sql += "ORDER BY v.next_reminder_at ASC";

    QSqlQuery q;
    q.prepare(sql);
    q.bindValue(":today", today);
    if (!modeToday) {
        QString overdueFrom = QDate::currentDate()
        .addDays(-overdueDays)
            .toString("yyyy-MM-dd");
        q.bindValue(":overdueFrom", overdueFrom);
    }
    if (!search.isEmpty()) {
        q.bindValue(":search",  "%" + search + "%");
        q.bindValue(":search2", "%" + search + "%");
        q.bindValue(":search3", "%" + search + "%");
    }

    if (!q.exec()) return;

    int row = 0;
    while (q.next()) {
        int     rfId        = q.value("rf_id").toInt();
        QString animalName  = q.value("animal_name").toString();
        QString animalType  = q.value("animal_type").toString();
        QString ownerName   = q.value("owner_name").toString();
        QString phone       = q.value("phone").toString();
        QString vaccineName = q.value("vaccine_name").toString();
        QDate   reminderDate= q.value("next_reminder_at").toDate();
        bool    isFollowedUp= q.value("is_followed_up").toBool();
        QVariant ownerResp  = q.value("owner_responded");

        tbl->insertRow(row);
        tbl->setRowHeight(row, 46);

        auto makeItem = [](const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            return item;
        };

        bool isDog = (animalType == "dog");

        tbl->setItem(row, 0, makeItem(animalName));
        tbl->setCellWidget(row, 1, makeBadge(
                                       isDog ? "سگ" : "گربه",
                                       isDog ? "#E8F5E9" : "#FFF3E0",
                                       isDog ? "#1B5E20" : "#BF360C"));
        tbl->setItem(row, 2, makeItem(ownerName));
        tbl->setItem(row, 3, makeItem(phone));
        tbl->setItem(row, 4, makeItem(vaccineName));
        tbl->setItem(row, 5, makeItem(reminderDate.toString("yyyy/MM/dd")));

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

    tbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

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
        // Waiting — set to null
        q.prepare("UPDATE reminder_followups SET owner_responded = NULL WHERE id = :id");
    } else {
        q.prepare("UPDATE reminder_followups SET owner_responded = :val WHERE id = :id");
        q.bindValue(":val", responseIndex == 1 ? 1 : 0);
    }
    q.bindValue(":id", rfId);
    q.exec();
    loadStats();
}

void RemindersWidget::onExportPdfClicked()
{
    QString path = QFileDialog::getSaveFileName(
        this, "ذخیره PDF", "یادآوری‌ها.pdf", "PDF Files (*.pdf)");
    if (path.isEmpty()) return;

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

    // Title
    painter.setFont(titleFont);
    painter.drawText(QRect(0, y, pageW, 200), Qt::AlignCenter, "لیست یادآوری‌های واکسن");
    y += 260;

    // Date
    painter.setFont(cellFont);
    painter.drawText(QRect(0, y, pageW, 150), Qt::AlignCenter,
                     "تاریخ: " + QDate::currentDate().toString("yyyy/MM/dd"));
    y += 220;

    // Table header
    QStringList cols = {"حیوان", "صاحب", "شماره", "نوع واکسن", "موعد یادآوری", "وضعیت"};
    int colW = pageW / cols.size();
    int rowH = 180;

    painter.setFont(headerFont);
    painter.fillRect(0, y, pageW, rowH, QColor("#E8F5E9"));
    for (int i = 0; i < cols.size(); i++) {
        painter.drawText(QRect(i * colW, y, colW, rowH),
                         Qt::AlignCenter, cols[i]);
    }
    y += rowH;

    // Table rows from current table widget
    painter.setFont(cellFont);
    auto* tbl = ui->remindersTable;
    for (int r = 0; r < tbl->rowCount(); r++) {
        if (y + rowH > writer.height() - 200) {
            writer.newPage();
            y = 100;
        }
        if (r % 2 == 0)
            painter.fillRect(0, y, pageW, rowH, QColor("#FAFAFA"));

        QStringList rowData;
        for (int c : {0, 2, 3, 4, 5}) {
            auto* item = tbl->item(r, c);
            rowData << (item ? item->text() : "");
        }
        // Follow-up status
        auto* cbWidget = tbl->cellWidget(r, 6);
        bool followed = false;
        if (cbWidget) {
            auto* cb = cbWidget->findChild<QCheckBox*>();
            if (cb) followed = cb->isChecked();
        }
        rowData << (followed ? "پیگیری شد" : "در انتظار");

        for (int i = 0; i < rowData.size(); i++) {
            painter.drawText(QRect(i * colW, y, colW, rowH),
                             Qt::AlignCenter, rowData[i]);
        }
        y += rowH;
    }

    painter.end();
    QMessageBox::information(this, "موفق", "فایل PDF با موفقیت ذخیره شد.");
}