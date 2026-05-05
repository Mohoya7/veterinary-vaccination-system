#include "dashboardwidget.h"
#include "ui_dashboardwidget.h"
#include <QSqlQuery>
#include <QDate>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DashboardWidget)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);

    this->setStyleSheet(R"(
        QWidget {
            background-color: transparent;
        }
        QLabel#pageTitle {
            color: #757575;
            font-size: 13px;
        }
        QWidget#animalCard, QWidget#ownerCard {
            background-color: white;
            border: 1px solid #C8E6C9;
            border-radius: 10px;
        }
        QWidget#reminderCard {
            background-color: white;
            border: 1px solid #F9A825;
            border-radius: 10px;
        }
        QLabel#animalCardTitle, QLabel#ownerCardTitle, QLabel#reminderCardTitle {
            color: #757575;
            font-size: 12px;
            background: transparent;
        }
        QLabel#animalCountLabel, QLabel#ownerCountLabel {
            color: #2E7D32;
            font-size: 24px;
            font-weight: bold;
            background: transparent;
        }
        QLabel#reminderCountLabel {
            color: #F9A825;
            font-size: 24px;
            font-weight: bold;
            background: transparent;
        }
        QWidget#tableCard {
            background-color: white;
            border: 1px solid #C8E6C9;
            border-radius: 10px;
        }
        QLabel#tableTitle {
            color: #212121;
            font-size: 14px;
            font-weight: bold;
            background: transparent;
        }
        QTableWidget {
            background-color: white;
            border: none;
            gridline-color: #F1F8E9;
            font-size: 12px;
        }
        QTableWidget::item {
            padding: 6px 8px;
            color: #212121;
            border-bottom: 1px solid #F1F8E9;
        }
        QTableWidget::item:selected {
            background-color: #E8F5E9;
            color: #212121;
        }
        QHeaderView::section {
            background-color: white;
            color: #757575;
            font-size: 12px;
            font-weight: normal;
            border: none;
            border-bottom: 1px solid #E8F5E9;
            padding: 6px 8px;
            qproperty-alignment: AlignLeft;
        }
    )");

    // All labels right-aligned (Left in RTL = visual right)
    ui->pageTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->animalCardTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->ownerCardTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->reminderCardTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->animalCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->ownerCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->reminderCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->tableTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    loadData();
}

DashboardWidget::~DashboardWidget()
{
    delete ui;
}

void DashboardWidget::loadData()
{
    loadStats();
    loadTodayReminders();
}

void DashboardWidget::loadStats()
{
    QSqlQuery q;

    q.exec("SELECT COUNT(*) FROM animals WHERE is_deleted = FALSE");
    if (q.next()) {
        ui->animalCountLabel->setText(q.value(0).toString());
        ui->animalCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    q.exec("SELECT COUNT(*) FROM owners WHERE is_deleted = FALSE");
    if (q.next()) {
        ui->ownerCountLabel->setText(q.value(0).toString());
        ui->ownerCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    q.exec(QString(
               "SELECT COUNT(*) FROM reminder_followups rf "
               "JOIN vaccinations v ON rf.vaccination_id = v.id "
               "WHERE v.next_reminder_at = '%1' AND rf.is_resolved = FALSE"
               ).arg(QDate::currentDate().toString("yyyy-MM-dd")));
    if (q.next()) {
        ui->reminderCountLabel->setText(q.value(0).toString());
        ui->reminderCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
}

static QWidget* makeBadgeWidget(const QString& text,
                                const QString& bg,
                                const QString& fg)
{
    QWidget* container = new QWidget;
    QHBoxLayout* lay = new QHBoxLayout(container);
    lay->setContentsMargins(8, 2, 8, 2);
    // Left in RTL = visual right
    lay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel* lbl = new QLabel(text);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lbl->setStyleSheet(QString(
                           "QLabel {"
                           "  background-color: %1;"
                           "  color: %2;"
                           "  border-radius: 4px;"
                           "  font-size: 11px;"
                           "  padding: 2px 8px;"
                           "}"
                           ).arg(bg, fg));

    lay->addWidget(lbl);
    lay->addStretch();
    return container;
}

static QTableWidgetItem* makeItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    // AlignLeft in RTL = visual right
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return item;
}

void DashboardWidget::loadTodayReminders()
{
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    QSqlQuery q;
    q.prepare(
        "SELECT a.name, a.type, "
        "CONCAT(o.first_name, ' ', o.last_name) AS owner_name, "
        "o.phone, vt.name AS vaccine_name, "
        "rf.is_followed_up, rf.owner_responded "
        "FROM reminder_followups rf "
        "JOIN vaccinations v ON rf.vaccination_id = v.id "
        "JOIN animals a ON v.animal_id = a.id "
        "JOIN owners o ON a.owner_id = o.id "
        "JOIN vaccine_types vt ON v.vaccine_type_id = vt.id "
        "WHERE v.next_reminder_at = :today AND rf.is_resolved = FALSE"
        );
    q.bindValue(":today", today);
    q.exec();

    QTableWidget* tbl = ui->remindersTable;
    tbl->setRowCount(0);
    tbl->setColumnCount(6);
    tbl->setLayoutDirection(Qt::RightToLeft);

    QStringList headers = { "حیوان", "نوع", "صاحب", "شماره صاحب", "نوع واکسن", "وضعیت" };
    tbl->setHorizontalHeaderLabels(headers);

    QHeaderView* hv = tbl->horizontalHeader();
    hv->setLayoutDirection(Qt::RightToLeft);
    hv->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hv->setSectionResizeMode(QHeaderView::Stretch);

    tbl->verticalHeader()->setVisible(false);
    tbl->setShowGrid(false);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setFocusPolicy(Qt::NoFocus);

    int row = 0;
    while (q.next()) {
        tbl->insertRow(row);
        tbl->setRowHeight(row, 38);

        tbl->setItem(row, 0, makeItem(q.value("name").toString()));

        bool isDog = (q.value("type").toString() == "dog");
        tbl->setCellWidget(row, 1, makeBadgeWidget(
                                       isDog ? "سگ" : "گربه",
                                       isDog ? "#E8F5E9" : "#FFF3E0",
                                       isDog ? "#2E7D32" : "#E65100"
                                       ));

        tbl->setItem(row, 2, makeItem(q.value("owner_name").toString()));
        tbl->setItem(row, 3, makeItem(q.value("phone").toString()));
        tbl->setItem(row, 4, makeItem(q.value("vaccine_name").toString()));

        QString statusText, statusBg, statusFg;
        if (q.value("owner_responded").toBool()) {
            statusText = "جواب داد";  statusBg = "#E8F5E9"; statusFg = "#2E7D32";
        } else if (q.value("is_followed_up").toBool()) {
            statusText = "پیگیری شد"; statusBg = "#E8F5E9"; statusFg = "#2E7D32";
        } else {
            statusText = "در انتظار"; statusBg = "#FFF9C4"; statusFg = "#F57F17";
        }
        tbl->setCellWidget(row, 5, makeBadgeWidget(statusText, statusBg, statusFg));

        row++;
    }

    // Hide scrollbar
    tbl->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tbl->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Dynamic height based on row count
    int totalHeight = tbl->horizontalHeader()->height();
    for (int i = 0; i < tbl->rowCount(); i++)
        totalHeight += tbl->rowHeight(i);
    tbl->setMinimumHeight(totalHeight);
    tbl->setMaximumHeight(totalHeight);
}