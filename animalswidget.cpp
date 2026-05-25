#include "animalswidget.h"
#include "ui_animalswidget.h"
#include "addanimaldialog.h"
#include "addvaccinedialog.h"
#include "styledmessagebox.h"

#include <QSqlQuery>
#include <QDate>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialog>
#include <QScrollBar>
#include <QPainter>

// ─── Animal item widget ─────────────────────────────────
class AnimalItemWidget : public QWidget {
public:
    AnimalItemWidget(const QString& name, const QString& type,
                     const QString& breed, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(56);
        setLayoutDirection(Qt::RightToLeft);

        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(14, 8, 14, 8);
        lay->setSpacing(10);

        // Avatar with animal type icon
        bool isDog = (type == "dog");
        auto* avatar = new QLabel(isDog ? "🐕" : "🐈");
        avatar->setFixedSize(36, 36);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(QString(
                                  "background:%1;border-radius:18px;font-size:16px;")
                                  .arg(isDog ? "#E8F5E9" : "#FFF3E0"));

        auto* infoW = new QWidget;
        infoW->setStyleSheet("background:transparent;");
        auto* infoL = new QVBoxLayout(infoW);
        infoL->setContentsMargins(0,0,0,0);
        infoL->setSpacing(2);

        auto* nameL = new QLabel(name);
        nameL->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        nameL->setStyleSheet("font-size:13px;font-weight:500;color:#212121;background:transparent;");

        QString sub = isDog ? "سگ" : "گربه";
        if (!breed.isEmpty()) sub += " | " + breed;
        auto* subL = new QLabel(sub);
        subL->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        subL->setStyleSheet("font-size:11px;color:#757575;background:transparent;");

        infoL->addWidget(nameL);
        infoL->addWidget(subL);

        lay->addWidget(infoW);
        lay->addWidget(avatar);
    }
};

// ─── Constructor ─────────────────────────────────────────
AnimalsWidget::AnimalsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AnimalsWidget)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    applyStyle();

    ui->profileWidget->hide();

    connect(ui->searchEdit,      &QLineEdit::textChanged,
            this, &AnimalsWidget::onSearchChanged);
    connect(ui->typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AnimalsWidget::onTypeFilterChanged);
    connect(ui->animalListWidget, &QListWidget::currentItemChanged,
            this, &AnimalsWidget::onAnimalSelected);
    connect(ui->btnAddAnimal,    &QPushButton::clicked, this, &AnimalsWidget::onAddAnimalClicked);
    connect(ui->btnEdit,         &QPushButton::clicked, this, &AnimalsWidget::onEditAnimalClicked);
    connect(ui->btnDelete,       &QPushButton::clicked, this, &AnimalsWidget::onDeleteAnimalClicked);
    connect(ui->btnViewHistory,  &QPushButton::clicked, this, &AnimalsWidget::onViewVaccinationHistoryClicked);

    loadAnimals();
}

AnimalsWidget::~AnimalsWidget() { delete ui; }

void AnimalsWidget::loadData() { loadAnimals(); }

// ─── applyStyle ──────────────────────────────────────────
void AnimalsWidget::applyStyle()
{
    setStyleSheet(R"(
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
            font-size: 15px; font-weight: 500;
            color: #212121; background: transparent;
        }
        QLineEdit#searchEdit {
            background: #F5F5F5;
            border: 0.5px solid #E0E0E0;
            border-radius: 6px;
            padding: 6px 10px;
            font-size: 12px; color: #212121;
        }
        QLineEdit#searchEdit:focus { border-color: #2E7D32; background: white; }

        QComboBox#typeFilterCombo {
            background: #F5F5F5;
            border: 0.5px solid #E0E0E0;
            border-radius: 6px;
            padding: 5px 10px;
            font-size: 12px; color: #212121;
            min-height: 30px;
        }
        QComboBox#typeFilterCombo QAbstractItemView {
            background: white;
            border: 0.5px solid #C8E6C9;
            selection-background-color: #E8F5E9;
            font-size: 12px;
        }

        QListWidget#animalListWidget {
            background: white; border: none; outline: none;
        }
        QListWidget#animalListWidget::item {
            border-bottom: 0.5px solid #F5F5F5; padding: 0px;
        }
        QListWidget#animalListWidget::item:selected {
            background: #E8F5E9; border-right: 3px solid #2E7D32;
        }

        QPushButton#btnAddAnimal {
            background: #2E7D32; color: white; border: none;
            border-radius: 6px; font-size: 12px;
        }
        QPushButton#btnAddAnimal:hover { background: #1B5E20; }

        QWidget#profileHeaderWidget { background: #2E7D32; }
        QLabel#animalNameLabel {
            color: white; font-size: 15px; font-weight: 500; background: transparent;
        }
        QLabel#animalSubLabel {
            color: rgba(255,255,255,0.7); font-size: 12px; background: transparent;
        }

        QPushButton#btnViewHistory {
            background: rgba(255,255,255,0.15);
            color: white;
            border: 0.5px solid rgba(255,255,255,0.3);
            border-radius: 6px; padding: 4px 12px; font-size: 12px;
        }
        QPushButton#btnViewHistory:hover { background: rgba(255,255,255,0.25); }

        QPushButton#btnEdit {
            background: rgba(255,255,255,0.12);
            color: white;
            border: 0.5px solid rgba(255,255,255,0.3);
            border-radius: 6px; padding: 4px 12px; font-size: 12px;
        }
        QPushButton#btnEdit:hover { background: rgba(255,255,255,0.2); }

        QPushButton#btnDelete {
            background: rgba(255,100,100,0.15);
            color: #FFCDD2;
            border: 0.5px solid rgba(255,150,150,0.4);
            border-radius: 6px; padding: 4px 12px; font-size: 12px;
        }
        QPushButton#btnDelete:hover { background: rgba(255,100,100,0.25); }

        QScrollArea { background: transparent; border: none; }
        QWidget#profileScrollContent { background: transparent; }

        QWidget#infoCard, QWidget#ownerCard {
            background: white;
            border: 0.5px solid #E0E0E0;
            border-radius: 10px;
        }
        QLabel#infoCardTitle, QLabel#ownerCardTitle {
            font-size: 12px; font-weight: 500;
            color: #757575; background: transparent;
        }

        QPushButton#btnGoToOwner {
            background: #F1F8E9;
            border: 0.5px solid #C8E6C9;
            border-radius: 8px;
            font-size: 13px; font-weight: 500;
            color: #2E7D32;
            text-align: right;
            padding: 0 12px;
        }
        QPushButton#btnGoToOwner:hover { background: #E8F5E9; border-color: #2E7D32; }

        QLabel#placeholderLabel { color: #BDBDBD; font-size: 14px; }
    )");

    // Scrollbar style for scroll area
    ui->profileScrollArea->verticalScrollBar()->setStyleSheet(R"(
        QScrollBar:vertical {
            background: #F5F5F5; width: 6px;
            border-radius: 3px; margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #A5D6A7; border-radius: 3px; min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: #2E7D32; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
    )");
}

// ─── Load animals list ───────────────────────────────────
void AnimalsWidget::loadAnimals(const QString& search, int typeFilter)
{
    m_currentOffset = 0;
    ui->animalListWidget->clear();

    // حذف دکمه قدیمی از layout
    if (m_loadMoreBtn) {
        auto* lay = qobject_cast<QVBoxLayout*>(ui->listPanel->layout());
        if (lay) lay->removeWidget(m_loadMoreBtn);
        m_loadMoreBtn->deleteLater();
        m_loadMoreBtn = nullptr;
    }

    appendAnimals(search, typeFilter, 0);
}

void AnimalsWidget::appendAnimals(const QString& search, int typeFilter, int offset)
{
    QString sql =
        "SELECT a.id, a.name, a.type, a.breed "
        "FROM animals a "
        "JOIN owners o ON a.owner_id = o.id "
        "WHERE a.is_deleted = FALSE ";

    if (!search.isEmpty())
        sql += "AND (a.name LIKE :s OR o.phone LIKE :s2 "
               "OR CONCAT(o.first_name,' ',o.last_name) LIKE :s3) ";
    if (typeFilter == 1) sql += "AND a.type = 'dog' ";
    else if (typeFilter == 2) sql += "AND a.type = 'cat' ";

    sql += QString("ORDER BY a.name LIMIT %1 OFFSET %2")
               .arg(m_pageSize + 1)
               .arg(offset);

    QSqlQuery q;
    q.prepare(sql);
    if (!search.isEmpty()) {
        QString like = "%" + search + "%";
        q.bindValue(":s",  like);
        q.bindValue(":s2", like);
        q.bindValue(":s3", like);
    }
    q.exec();

    int fetched = 0;
    while (q.next()) {
        fetched++;
        if (fetched > m_pageSize) break; // رکورد اضافه رو نمایش نمی‌دیم

        int     id    = q.value("id").toInt();
        QString name  = q.value("name").toString();
        QString type  = q.value("type").toString();
        QString breed = q.value("breed").toString();

        auto* item = new QListWidgetItem(ui->animalListWidget);
        item->setData(Qt::UserRole, id);
        item->setSizeHint(QSize(0, 56));

        auto* widget = new AnimalItemWidget(name, type, breed, ui->animalListWidget);
        ui->animalListWidget->setItemWidget(item, widget);

        if (id == m_selectedAnimalId)
            ui->animalListWidget->setCurrentItem(item);
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
            // addWidget به انتهای layout اضافه می‌کنه — ولی چون btnAddAnimal
            // داخل یه QVBoxLayout جداست (نه مستقیم widget)، ما باید
            // قبل از آخرین آیتم layout اضافه کنیم
            int insertIdx = panelLayout ? panelLayout->count() - 1 : -1;
            if (panelLayout && insertIdx >= 0)
                panelLayout->insertWidget(insertIdx, m_loadMoreBtn);
            else if (panelLayout)
                panelLayout->addWidget(m_loadMoreBtn);
            connect(m_loadMoreBtn, &QPushButton::clicked, this, [this]() {
                appendAnimals(
                    ui->searchEdit->text().trimmed(),
                    ui->typeFilterCombo->currentIndex(),
                    m_currentOffset);
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

// ─── Show animal profile ─────────────────────────────────
void AnimalsWidget::showAnimalProfile(int animalId)
{
    m_selectedAnimalId = animalId;

    QSqlQuery q;
    q.prepare(
        "SELECT a.name, a.type, a.breed, a.birth_date, a.gender, a.weight, "
        "a.owner_id, CONCAT(o.first_name,' ',o.last_name) AS owner_name, o.phone "
        "FROM animals a JOIN owners o ON a.owner_id = o.id "
        "WHERE a.id = :id");
    q.bindValue(":id", animalId);
    q.exec();
    if (!q.next()) return;

    m_selectedOwnerId = q.value("owner_id").toInt();

    bool    isDog   = (q.value("type").toString() == "dog");
    QString typeStr = isDog ? "سگ" : "گربه";
    QString name    = q.value("name").toString();

    // Avatar
    ui->animalAvatarLabel->setText(isDog ? "🐕" : "🐈");
    ui->animalAvatarLabel->setStyleSheet(QString(
        "background:rgba(255,255,255,0.18);border-radius:25px;font-size:20px;"));

    ui->animalNameLabel->setText(name);

    QString sub = typeStr;
    if (!q.value("breed").toString().isEmpty())
        sub += " | " + q.value("breed").toString();
    ui->animalSubLabel->setText(sub);

    // Info rows
    clearInfoRows();
    addInfoRow(ui->infoRowsContainer, "نوع", typeStr);
    if (!q.value("breed").toString().isEmpty())
        addInfoRow(ui->infoRowsContainer, "نژاد", q.value("breed").toString());
    if (!q.value("birth_date").isNull())
        addInfoRow(ui->infoRowsContainer, "تاریخ تولد",
                   q.value("birth_date").toDate().toString("yyyy/MM/dd"));
    addInfoRow(ui->infoRowsContainer, "جنسیت",
               q.value("gender").toString() == "male" ? "نر" : "ماده");
    if (!q.value("weight").isNull())
        addInfoRow(ui->infoRowsContainer, "وزن",
                   QString::number(q.value("weight").toDouble(), 'f', 1) + " kg");

    // Owner button
    QString ownerText = q.value("owner_name").toString()
                        + "   |   " + q.value("phone").toString();
    ui->btnGoToOwner->setText(ownerText);
    ui->btnGoToOwner->setLayoutDirection(Qt::RightToLeft);

    // Reconnect owner button
    disconnect(ui->btnGoToOwner, nullptr, nullptr, nullptr);
    connect(ui->btnGoToOwner, &QPushButton::clicked, this, [this]() {
        emit navigateToOwner(m_selectedOwnerId);
    });

    ui->placeholderWidget->hide();
    ui->profileWidget->show();
}

void AnimalsWidget::showAnimalById(int animalId)
{
    // Find in list or just show profile directly
    for (int i = 0; i < ui->animalListWidget->count(); i++) {
        if (ui->animalListWidget->item(i)->data(Qt::UserRole).toInt() == animalId) {
            ui->animalListWidget->setCurrentRow(i);
            return;
        }
    }
    // Not in current list — reload without filter then select
    ui->searchEdit->clear();
    ui->typeFilterCombo->setCurrentIndex(0);
    loadAnimals();
    for (int i = 0; i < ui->animalListWidget->count(); i++) {
        if (ui->animalListWidget->item(i)->data(Qt::UserRole).toInt() == animalId) {
            ui->animalListWidget->setCurrentRow(i);
            return;
        }
    }
    // Fallback: just show profile
    showAnimalProfile(animalId);
}

void AnimalsWidget::clearProfile()
{
    m_selectedAnimalId = -1;
    m_selectedOwnerId  = -1;
    ui->profileWidget->hide();
    ui->placeholderWidget->show();
}

// ─── Info rows ───────────────────────────────────────────
void AnimalsWidget::clearInfoRows()
{
    QLayout* lay = ui->infoRowsContainer->layout();
    while (lay->count() > 0) {
        QLayoutItem* item = lay->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}

void AnimalsWidget::addInfoRow(QWidget* container,
                               const QString& label,
                               const QString& value)
{
    auto* row = new QWidget;
    row->setStyleSheet("background: transparent;");
    auto* rowLay = new QHBoxLayout(row);
    rowLay->setContentsMargins(0, 7, 0, 7);
    rowLay->setSpacing(8);

    auto* valueL = new QLabel(value);
    valueL->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    valueL->setStyleSheet("font-size:12px;font-weight:500;color:#212121;background:transparent;");

    auto* labelL = new QLabel(label);
    labelL->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    labelL->setStyleSheet("font-size:12px;color:#757575;background:transparent;");

    rowLay->addWidget(valueL);
    rowLay->addStretch();
    rowLay->addWidget(labelL);

    auto* divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: #F0F0F0;");

    container->layout()->addWidget(row);
    container->layout()->addWidget(divider);
}

// ─── Vaccination history ─────────────────────────────────
void AnimalsWidget::onViewVaccinationHistoryClicked()
{
    if (m_selectedAnimalId < 0) return;

    QDialog histDialog(this);
    histDialog.setWindowTitle("تاریخچه واکسیناسیون");
    histDialog.setLayoutDirection(Qt::RightToLeft);
    histDialog.setMinimumSize(700, 450);
    histDialog.setStyleSheet(R"(
        QDialog { background: #F1F8E9; }
        QWidget#histHeader { background: #2E7D32; }
        QLabel#histTitle {
            color: white; font-size: 14px;
            font-weight: 500; background: transparent;
        }
        QWidget#histBody { background: white; border-radius: 10px; }
        QTableWidget {
            background: white; border: none;
            gridline-color: #F5F5F5; font-size: 12px; outline: none;
        }
        QTableWidget::item {
            padding: 8px 10px; color: #212121;
            border-bottom: 0.5px solid #F5F5F5;
        }
        QTableWidget::item:selected { background: #E8F5E9; color: #212121; }
        QHeaderView::section {
            background: #FAFAFA; color: #757575;
            font-size: 12px; font-weight: 500;
            border: none; border-bottom: 0.5px solid #E0E0E0;
            padding: 8px 10px;
        }
        QScrollBar:vertical {
            background: #F5F5F5; width: 6px;
            border-radius: 3px; margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #A5D6A7; border-radius: 3px; min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: #2E7D32; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");

    auto* mainLay = new QVBoxLayout(&histDialog);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);

    // Header
    auto* header = new QWidget;
    header->setObjectName("histHeader");
    header->setFixedHeight(56);
    auto* headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(20, 0, 20, 0);
    auto* histTitle = new QLabel("تاریخچه واکسیناسیون");
    histTitle->setObjectName("histTitle");
    headerLay->addWidget(histTitle);
    headerLay->addStretch();

    // Body
    auto* body = new QWidget;
    body->setObjectName("histBody");
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(16, 16, 16, 16);

    auto* tbl = new QTableWidget;
    tbl->setColumnCount(5);
    tbl->setLayoutDirection(Qt::RightToLeft);
    QStringList headers = {"نوع واکسن", "تاریخ تزریق", "یادآوری بعدی", "وضعیت", ""};
    tbl->setHorizontalHeaderLabels(headers);

    auto* hv = tbl->horizontalHeader();
    hv->setLayoutDirection(Qt::RightToLeft);
    hv->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hv->setSectionResizeMode(QHeaderView::Stretch);
    hv->setSectionResizeMode(4, QHeaderView::Fixed);
    tbl->setColumnWidth(4, 120);
    tbl->verticalHeader()->setVisible(false);
    tbl->setShowGrid(false);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setFocusPolicy(Qt::NoFocus);

    // Load data
    QSqlQuery q;
    q.prepare(
        "SELECT v.id, vt.name AS vaccine_name, "
        "v.vaccinated_at, v.next_reminder_at "
        "FROM vaccinations v "
        "JOIN vaccine_types vt ON v.vaccine_type_id = vt.id "
        "WHERE v.animal_id = :id AND v.is_deleted = FALSE "
        "ORDER BY v.vaccinated_at DESC");
    q.bindValue(":id", m_selectedAnimalId);
    q.exec();

    int row = 0;
    QDate today = QDate::currentDate();
    while (q.next()) {
        int   vacId    = q.value("id").toInt();
        QDate vacDate  = q.value("vaccinated_at").toDate();
        QDate nextDate = q.value("next_reminder_at").toDate();

        QString statusText, statusBg, statusFg;
        if (nextDate < today)       { statusText="تأخیر دارد"; statusBg="#FFEBEE"; statusFg="#C62828"; }
        else if (nextDate == today) { statusText="موعد رسیده"; statusBg="#FFF9C4"; statusFg="#F57F17"; }
        else                        { statusText="تمدید شده";  statusBg="#E8F5E9"; statusFg="#2E7D32"; }

        tbl->insertRow(row);
        tbl->setRowHeight(row, 42);

        auto makeItem = [](const QString& t) {
            auto* item = new QTableWidgetItem(t);
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            return item;
        };

        tbl->setItem(row, 0, makeItem(q.value("vaccine_name").toString()));
        tbl->setItem(row, 1, makeItem(vacDate.toString("yyyy/MM/dd")));
        tbl->setItem(row, 2, makeItem(nextDate.toString("yyyy/MM/dd")));

        // Status badge
        auto* badge = new QWidget;
        badge->setStyleSheet("background:transparent;");
        auto* badgeLay = new QHBoxLayout(badge);
        badgeLay->setContentsMargins(8, 2, 8, 2);
        badgeLay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        auto* badgeLbl = new QLabel(statusText);
        badgeLbl->setAlignment(Qt::AlignCenter);
        badgeLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        badgeLbl->setStyleSheet(QString(
                                    "background:%1;color:%2;border-radius:4px;"
                                    "font-size:11px;font-weight:500;padding:2px 8px;").arg(statusBg, statusFg));
        badgeLay->addWidget(badgeLbl);
        badgeLay->addStretch();
        tbl->setCellWidget(row, 3, badge);

        // Navigate button
        auto* navBtn = new QPushButton("مشاهده در واکسن‌ها");
        navBtn->setStyleSheet(R"(
            QPushButton {
                background: white; border: 0.5px solid #C8E6C9;
                border-radius: 5px; font-size: 11px;
                color: #2E7D32; padding: 2px 8px; min-height: 26px;
            }
            QPushButton:hover { background: #E8F5E9; }
        )");
        connect(navBtn, &QPushButton::clicked, this, [this, vacId, &histDialog]() {
            histDialog.accept();
            emit navigateToVaccination(vacId);
        });
        tbl->setCellWidget(row, 4, navBtn);

        row++;
    }

    // Hint if empty
    if (row == 0) {
        auto* emptyL = new QLabel("واکسنی ثبت نشده است");
        emptyL->setAlignment(Qt::AlignCenter);
        emptyL->setStyleSheet("color:#BDBDBD;font-size:13px;");
        bodyLay->addWidget(emptyL);
    } else {
        bodyLay->addWidget(tbl);
    }

    auto* btnClose = new QPushButton("بستن");
    btnClose->setFixedHeight(36);
    btnClose->setStyleSheet(R"(
        QPushButton {
            background:#2E7D32;color:white;border:none;
            border-radius:6px;font-size:13px;padding:0 24px;
        }
        QPushButton:hover{background:#1B5E20;}
    )");
    connect(btnClose, &QPushButton::clicked, &histDialog, &QDialog::accept);

    auto* footerRow = new QHBoxLayout;
    footerRow->addStretch();
    footerRow->addWidget(btnClose);
    bodyLay->addLayout(footerRow);

    mainLay->addWidget(header);
    mainLay->addWidget(body, 1);

    histDialog.exec();
}

// ─── Slots ───────────────────────────────────────────────
void AnimalsWidget::onSearchChanged(const QString& text)
{
    loadAnimals(text.trimmed(), ui->typeFilterCombo->currentIndex());
}

void AnimalsWidget::onTypeFilterChanged(int index)
{
    loadAnimals(ui->searchEdit->text().trimmed(), index);
}

void AnimalsWidget::onAnimalSelected(QListWidgetItem* current, QListWidgetItem*)
{
    if (!current) { clearProfile(); return; }
    showAnimalProfile(current->data(Qt::UserRole).toInt());
}

void AnimalsWidget::onAddAnimalClicked()
{
    // Pick owner first
    QDialog picker(this);
    picker.setWindowTitle("انتخاب صاحب");
    picker.setLayoutDirection(Qt::RightToLeft);
    picker.setFixedWidth(400);
    picker.setStyleSheet(R"(
        QDialog{background:white;}
        QLabel{font-size:13px;color:#5F5E5A;background:transparent;}
        QLineEdit{border:1px solid #A5D6A7;border-radius:6px;padding:7px 10px;
                  font-size:13px;background:#F9FBF9;min-height:36px;}
        QLineEdit:focus{border-color:#2E7D32;background:white;}
        QListWidget{border:0.5px solid #E0E0E0;border-radius:6px;font-size:13px;outline:none;}
        QListWidget::item{padding:10px 12px;border-bottom:0.5px solid #F5F5F5;}
        QListWidget::item:selected{background:#E8F5E9;color:#212121;}
        QPushButton#btnConfirm{background:#2E7D32;color:white;border:none;
            border-radius:6px;padding:8px 20px;font-size:13px;}
        QPushButton#btnConfirm:hover{background:#1B5E20;}
        QPushButton#btnPickCancel{background:white;color:#757575;
            border:0.5px solid #E0E0E0;border-radius:6px;padding:8px 20px;font-size:13px;}
    )");

    auto* lay = new QVBoxLayout(&picker);
    lay->setContentsMargins(20,20,20,20);
    lay->setSpacing(12);

    auto* lbl = new QLabel("صاحب حیوان را انتخاب کنید:");
    lbl->setAlignment(Qt::AlignLeft);
    auto* searchLine = new QLineEdit;
    searchLine->setPlaceholderText("جستجو...");
    auto* listW = new QListWidget;
    listW->setMinimumHeight(200);

    auto* btnRow = new QWidget;
    btnRow->setStyleSheet("background:transparent;");
    auto* btnLay = new QHBoxLayout(btnRow);
    btnLay->setContentsMargins(0,0,0,0);
    btnLay->setSpacing(8);
    auto* btnCancel  = new QPushButton("انصراف");
    btnCancel->setObjectName("btnPickCancel");
    auto* btnConfirm = new QPushButton("انتخاب");
    btnConfirm->setObjectName("btnConfirm");
    btnConfirm->setEnabled(false);
    btnLay->addStretch();
    btnLay->addWidget(btnCancel);
    btnLay->addWidget(btnConfirm);

    lay->addWidget(lbl);
    lay->addWidget(searchLine);
    lay->addWidget(listW);
    lay->addWidget(btnRow);

    auto loadOwners = [&](const QString& filter) {
        listW->clear();
        QSqlQuery q;
        q.prepare("SELECT id, first_name, last_name, phone FROM owners "
                  "WHERE is_deleted=FALSE AND "
                  "(CONCAT(first_name,' ',last_name) LIKE :f OR phone LIKE :f2) "
                  "ORDER BY first_name LIMIT 50");
        QString f = filter.isEmpty() ? "%" : "%" + filter + "%";
        q.bindValue(":f", f); q.bindValue(":f2", f);
        q.exec();
        while (q.next()) {
            QString display = q.value("first_name").toString() + " "
                              + q.value("last_name").toString()
                              + "   |   " + q.value("phone").toString();
            auto* item = new QListWidgetItem(display);
            item->setData(Qt::UserRole, q.value("id").toInt());
            item->setData(Qt::UserRole + 1, q.value("phone").toString());
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            listW->addItem(item);
        }
    };
    loadOwners("");

    connect(searchLine, &QLineEdit::textChanged, [&](const QString& t){ loadOwners(t); });
    connect(listW, &QListWidget::itemSelectionChanged,
            [&](){ btnConfirm->setEnabled(listW->currentItem() != nullptr); });
    connect(listW, &QListWidget::itemDoubleClicked, &picker, &QDialog::accept);
    connect(btnConfirm, &QPushButton::clicked, &picker, [&](){ if(listW->currentItem()) picker.accept(); });
    connect(btnCancel,  &QPushButton::clicked, &picker, &QDialog::reject);

    if (picker.exec() != QDialog::Accepted || !listW->currentItem()) return;

    int     ownerId   = listW->currentItem()->data(Qt::UserRole).toInt();
    QString ownerPhone= listW->currentItem()->data(Qt::UserRole + 1).toString();

    AddAnimalDialog dlg(ownerId, ownerPhone, this);
    if (dlg.exec() != QDialog::Accepted) return;

    int newAnimalId = dlg.savedAnimalId();
    loadAnimals();
    showAnimalById(newAnimalId);

    // Ask about vaccine
    bool addVaccine = StyledMessageBox::question(
        this, "افزودن واکسن",
        "آیا می‌خواهید برای این حیوان واکسن اضافه کنید؟");
    if (!addVaccine) return;

    AddVaccineDialog vacDlg(newAnimalId, this);
    if (vacDlg.exec() == QDialog::Accepted) {
        StyledMessageBox::success(this, "موفق", "واکسن با موفقیت ثبت شد.");
    }
}

void AnimalsWidget::onEditAnimalClicked()
{
    if (m_selectedAnimalId < 0) return;

    // Get owner info for the dialog
    QSqlQuery q;
    q.prepare("SELECT owner_id FROM animals WHERE id = :id");
    q.bindValue(":id", m_selectedAnimalId);
    q.exec();
    if (!q.next()) return;
    int ownerId = q.value("owner_id").toInt();

    q.prepare("SELECT phone FROM owners WHERE id = :id");
    q.bindValue(":id", ownerId);
    q.exec();
    QString phone = q.next() ? q.value("phone").toString() : "";

    // Reuse AddAnimalDialog — pre-fill data
    // For now open dialog, load data manually
    AddAnimalDialog dlg(ownerId, phone, this);

    // Pre-fill animal data
    QSqlQuery aq;
    aq.prepare("SELECT name, type, breed, birth_date, gender, weight FROM animals WHERE id=:id");
    aq.bindValue(":id", m_selectedAnimalId);
    aq.exec();
    if (aq.next()) {
        // We need edit support in AddAnimalDialog — for now just open as-is
        // TODO: add edit mode to AddAnimalDialog
    }

    if (dlg.exec() != QDialog::Accepted) return;
    loadAnimals();
    showAnimalProfile(m_selectedAnimalId);
}

void AnimalsWidget::onDeleteAnimalClicked()
{
    if (m_selectedAnimalId < 0) return;

    bool ok = StyledMessageBox::question(
        this, "حذف حیوان",
        "آیا مطمئن هستید؟ تمام واکسن‌های ثبت شده برای این حیوان نیز حذف خواهند شد.");
    if (!ok) return;

    QSqlQuery q;
    q.prepare("UPDATE reminder_followups rf "
              "JOIN vaccinations v ON rf.vaccination_id=v.id "
              "SET rf.is_resolved=TRUE WHERE v.animal_id=:id");
    q.bindValue(":id", m_selectedAnimalId); q.exec();

    q.prepare("UPDATE vaccinations SET is_deleted=TRUE, updated_at=NOW() WHERE animal_id=:id");
    q.bindValue(":id", m_selectedAnimalId); q.exec();

    q.prepare("UPDATE animals SET is_deleted=TRUE, updated_at=NOW() WHERE id=:id");
    q.bindValue(":id", m_selectedAnimalId); q.exec();

    clearProfile();
    loadAnimals();
}