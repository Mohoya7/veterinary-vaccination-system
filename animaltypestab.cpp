#include "animaltypestab.h"
#include "session.h"
#include "styledmessagebox.h"
#include "animaltypeinfo.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QColorDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpressionValidator>

// ─────────────────────────────────────────────────────────────────────────────
// AddAnimalTypeDialog
// ─────────────────────────────────────────────────────────────────────────────

class AddAnimalTypeDialog : public QDialog
{
public:
    explicit AddAnimalTypeDialog(QWidget* parent = nullptr, int typeId = -1)
        : QDialog(parent), m_typeId(typeId)
    {
        setWindowTitle(typeId < 0 ? "افزودن نوع حیوان" : "ویرایش نوع حیوان");
        setLayoutDirection(Qt::RightToLeft);
        setFixedWidth(380);

        auto* vlay = new QVBoxLayout(this);
        vlay->setContentsMargins(0, 0, 0, 0);
        vlay->setSpacing(0);

        // Header
        auto* hdr = new QWidget;
        hdr->setFixedHeight(48);
        hdr->setStyleSheet("QWidget{background:#2E7D32;}");
        auto* hdrLay = new QHBoxLayout(hdr);
        hdrLay->setContentsMargins(16, 0, 16, 0);
        auto* ttl = new QLabel(typeId < 0 ? "افزودن نوع حیوان" : "ویرایش نوع حیوان");
        ttl->setStyleSheet("color:white;font-size:14px;font-weight:500;background:transparent;");
        hdrLay->addWidget(ttl);
        vlay->addWidget(hdr);

        // Body
        auto* body = new QWidget;
        body->setStyleSheet("background:white;");
        auto* bodyLay = new QVBoxLayout(body);
        bodyLay->setContentsMargins(20, 20, 20, 16);
        bodyLay->setSpacing(14);

        QString fieldStyle =
            "QLineEdit{border:1px solid #A5D6A7;border-radius:6px;"
            "padding:7px 10px;font-size:13px;background:#F9FBF9;"
            "color:#212121;min-height:36px;}"
            "QLineEdit:focus{border-color:#2E7D32;background:white;}";

        // Name
        m_nameEdit = new QLineEdit;
        m_nameEdit->setPlaceholderText("مثال: سگ");
        m_nameEdit->setStyleSheet(fieldStyle);
        addField(bodyLay, "نام نوع حیوان *", m_nameEdit);

        // Emoji — single emoji only
        m_emojiEdit = new QLineEdit;
        m_emojiEdit->setPlaceholderText("مثال: 🐕");
        m_emojiEdit->setMaxLength(2);
        m_emojiEdit->setStyleSheet(fieldStyle);
        addField(bodyLay, "ایموجی", m_emojiEdit);

        // Badge color row
        auto* colorLbl = new QLabel("رنگ بج *");
        colorLbl->setStyleSheet("font-size:12px;color:#757575;background:transparent;");
        bodyLay->addWidget(colorLbl);

        auto* colorRow = new QHBoxLayout;
        colorRow->setSpacing(8);

        auto* bgBtn = new QPushButton("رنگ پس‌زمینه");
        auto* fgBtn = new QPushButton("رنگ متن");
        QString colorBtnStyle =
            "QPushButton{border:1px solid #A5D6A7;border-radius:6px;"
            "padding:6px 12px;font-size:12px;background:#F9FBF9;color:#212121;}"
            "QPushButton:hover{background:#E8F5E9;}";
        bgBtn->setStyleSheet(colorBtnStyle);
        fgBtn->setStyleSheet(colorBtnStyle);

        colorRow->addWidget(bgBtn);
        colorRow->addWidget(fgBtn);
        colorRow->addStretch();

        // Badge preview
        m_badgePreview = new QLabel;
        m_badgePreview->setAlignment(Qt::AlignCenter);
        m_badgePreview->setFixedSize(52, 28);
        colorRow->addWidget(m_badgePreview);

        // Emoji preview — separate, bigger
        m_emojiPreview = new QLabel;
        m_emojiPreview->setAlignment(Qt::AlignCenter);
        m_emojiPreview->setFixedSize(36, 36);
        m_emojiPreview->setStyleSheet(
            "font-size:22px;background:#F1F8E9;border-radius:6px;"
            "border:1px solid #C8E6C9;");
        colorRow->addWidget(m_emojiPreview);

        bodyLay->addLayout(colorRow);
        vlay->addWidget(body);

        // Footer
        auto* footer = new QWidget;
        footer->setStyleSheet("background:white;border-top:1px solid #E8F5E9;");
        footer->setFixedHeight(56);
        auto* footerLay = new QHBoxLayout(footer);
        footerLay->setContentsMargins(16, 0, 16, 0);
        footerLay->setSpacing(8);

        auto* btnSave   = new QPushButton("ذخیره");
        auto* btnCancel = new QPushButton("انصراف");
        btnSave->setFixedHeight(36);
        btnCancel->setFixedHeight(36);
        btnSave->setStyleSheet(
            "QPushButton{background:#2E7D32;color:white;border:none;"
            "border-radius:6px;font-size:13px;padding:0 20px;}"
            "QPushButton:hover{background:#1B5E20;}");
        btnCancel->setStyleSheet(
            "QPushButton{background:white;color:#757575;border:1px solid #E0E0E0;"
            "border-radius:6px;font-size:13px;padding:0 20px;}"
            "QPushButton:hover{background:#F5F5F5;}");

        footerLay->addStretch();
        footerLay->addWidget(btnCancel);
        footerLay->addWidget(btnSave);
        vlay->addWidget(footer);

        // Load existing or defaults
        if (typeId >= 0) {
            QSqlQuery q;
            q.prepare("SELECT name, emoji, badge_bg, badge_fg FROM animal_types WHERE id=:id");
            q.bindValue(":id", typeId);
            q.exec();
            if (q.next()) {
                m_nameEdit->setText(q.value("name").toString());
                m_emojiEdit->setText(q.value("emoji").toString());
                m_bgColor = QColor(q.value("badge_bg").toString());
                m_fgColor = QColor(q.value("badge_fg").toString());
            }
        } else {
            m_bgColor = QColor("#E8F5E9");
            m_fgColor = QColor("#1B5E20");
        }
        updatePreview();

        connect(m_emojiEdit, &QLineEdit::textChanged, this,
                [this](const QString&) { updatePreview(); });
        connect(m_nameEdit, &QLineEdit::textChanged, this,
                [this](const QString&) { updatePreview(); });

        connect(bgBtn, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(m_bgColor, this, "رنگ پس‌زمینه بج");
            if (c.isValid()) { m_bgColor = c; updatePreview(); }
        });
        connect(fgBtn, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(m_fgColor, this, "رنگ متن بج");
            if (c.isValid()) { m_fgColor = c; updatePreview(); }
        });
        connect(btnSave,   &QPushButton::clicked, this, &AddAnimalTypeDialog::onSave);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    QString name()    const { return m_nameEdit->text().trimmed(); }
    QString emoji()   const { return m_emojiEdit->text().trimmed(); }
    QString badgeBg() const { return m_bgColor.name(); }
    QString badgeFg() const { return m_fgColor.name(); }

private:
    void addField(QVBoxLayout* lay, const QString& label, QWidget* widget) {
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet("font-size:12px;color:#757575;background:transparent;");
        lay->addWidget(lbl);
        lay->addWidget(widget);
    }

    void updatePreview() {
        // Badge preview — shows name text with badge colors
        QString badgeText = m_nameEdit->text().isEmpty() ? "نمونه" : m_nameEdit->text();
        m_badgePreview->setText(badgeText);
        m_badgePreview->setStyleSheet(QString(
                                          "background:%1;color:%2;border-radius:4px;"
                                          "font-size:11px;font-weight:500;padding:2px 6px;")
                                          .arg(m_bgColor.name(), m_fgColor.name()));

        // Emoji preview — shows emoji separately
        QString emojiText = m_emojiEdit->text().trimmed();
        if (emojiText.isEmpty())
            m_emojiPreview->setText("—");
        else
            m_emojiPreview->setText(emojiText);
    }

    void onSave() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            StyledMessageBox::warning(this, "خطا", "لطفاً نام نوع حیوان را وارد کنید.");
            return;
        }
        accept();
    }

    int        m_typeId       = -1;
    QLineEdit* m_nameEdit     = nullptr;
    QLineEdit* m_emojiEdit    = nullptr;
    QLabel*    m_badgePreview = nullptr;
    QLabel*    m_emojiPreview = nullptr;
    QColor     m_bgColor;
    QColor     m_fgColor;
};

// ─────────────────────────────────────────────────────────────────────────────
// AddVaccineTypeDialog
// ─────────────────────────────────────────────────────────────────────────────

class AddVaccineTypeDialog : public QDialog
{
public:
    explicit AddVaccineTypeDialog(QWidget* parent = nullptr, int vaccineId = -1)
        : QDialog(parent), m_vaccineId(vaccineId)
    {
        setWindowTitle(vaccineId < 0 ? "افزودن نوع واکسن" : "ویرایش نوع واکسن");
        setLayoutDirection(Qt::RightToLeft);
        setFixedWidth(340);

        auto* vlay = new QVBoxLayout(this);
        vlay->setContentsMargins(0, 0, 0, 0);
        vlay->setSpacing(0);

        // Header
        auto* hdr = new QWidget;
        hdr->setFixedHeight(48);
        hdr->setStyleSheet("QWidget{background:#2E7D32;}");
        auto* hdrLay = new QHBoxLayout(hdr);
        hdrLay->setContentsMargins(16, 0, 16, 0);
        auto* ttl = new QLabel(vaccineId < 0 ? "افزودن نوع واکسن" : "ویرایش نوع واکسن");
        ttl->setStyleSheet("color:white;font-size:14px;font-weight:500;background:transparent;");
        hdrLay->addWidget(ttl);
        vlay->addWidget(hdr);

        // Body
        auto* body = new QWidget;
        body->setStyleSheet("background:white;");
        auto* bodyLay = new QVBoxLayout(body);
        bodyLay->setContentsMargins(20, 20, 20, 16);
        bodyLay->setSpacing(14);

        QString fieldStyle =
            "QLineEdit,QSpinBox{border:1px solid #A5D6A7;border-radius:6px;"
            "padding:7px 10px;font-size:13px;background:#F9FBF9;"
            "color:#212121;min-height:36px;}"
            "QLineEdit:focus,QSpinBox:focus{border-color:#2E7D32;background:white;}"
            "QSpinBox::up-button,QSpinBox::down-button{border:none;width:20px;background:transparent;}";

        m_nameEdit = new QLineEdit;
        m_nameEdit->setPlaceholderText("مثال: واکسن هاری");
        m_nameEdit->setStyleSheet(fieldStyle);

        // Reminder days — numbers only, label shows (بر حسب روز)
        m_reminderSpin = new QSpinBox;
        m_reminderSpin->setRange(1, 3650);
        m_reminderSpin->setValue(365);
        // No suffix — just plain number, label explains the unit
        m_reminderSpin->setStyleSheet(fieldStyle);

        auto addLblField = [&](const QString& lbl, QWidget* w) {
            auto* l = new QLabel(lbl);
            l->setStyleSheet("font-size:12px;color:#757575;background:transparent;");
            bodyLay->addWidget(l);
            bodyLay->addWidget(w);
        };

        addLblField("نام واکسن *", m_nameEdit);
        addLblField("دوره یادآوری پیش‌فرض * (بر حسب روز)", m_reminderSpin);

        vlay->addWidget(body);

        // Footer
        auto* footer = new QWidget;
        footer->setStyleSheet("background:white;border-top:1px solid #E8F5E9;");
        footer->setFixedHeight(56);
        auto* footerLay = new QHBoxLayout(footer);
        footerLay->setContentsMargins(16, 0, 16, 0);
        footerLay->setSpacing(8);

        auto* btnSave   = new QPushButton("ذخیره");
        auto* btnCancel = new QPushButton("انصراف");
        btnSave->setFixedHeight(36);
        btnCancel->setFixedHeight(36);
        btnSave->setStyleSheet(
            "QPushButton{background:#2E7D32;color:white;border:none;"
            "border-radius:6px;font-size:13px;padding:0 20px;}"
            "QPushButton:hover{background:#1B5E20;}");
        btnCancel->setStyleSheet(
            "QPushButton{background:white;color:#757575;border:1px solid #E0E0E0;"
            "border-radius:6px;font-size:13px;padding:0 20px;}"
            "QPushButton:hover{background:#F5F5F5;}");

        footerLay->addStretch();
        footerLay->addWidget(btnCancel);
        footerLay->addWidget(btnSave);
        vlay->addWidget(footer);

        if (vaccineId >= 0) {
            QSqlQuery q;
            q.prepare("SELECT name, default_reminder_days FROM vaccine_types WHERE id=:id");
            q.bindValue(":id", vaccineId);
            q.exec();
            if (q.next()) {
                m_nameEdit->setText(q.value("name").toString());
                m_reminderSpin->setValue(q.value("default_reminder_days").toInt());
            }
        }

        connect(btnSave, &QPushButton::clicked, this, [this]() {
            if (m_nameEdit->text().trimmed().isEmpty()) {
                StyledMessageBox::warning(this, "خطا", "لطفاً نام واکسن را وارد کنید.");
                return;
            }
            accept();
        });
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    QString name()         const { return m_nameEdit->text().trimmed(); }
    int     reminderDays() const { return m_reminderSpin->value(); }

private:
    int        m_vaccineId    = -1;
    QLineEdit* m_nameEdit     = nullptr;
    QSpinBox*  m_reminderSpin = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// AnimalTypesTab
// ─────────────────────────────────────────────────────────────────────────────

AnimalTypesTab::AnimalTypesTab(QWidget* parent)
    : QWidget(parent)
{
    setLayoutDirection(Qt::RightToLeft);

    auto* rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(24, 24, 24, 24);
    rootLay->setSpacing(16);

    auto* pageTitle = new QLabel("انواع حیوانات و واکسن‌ها");
    pageTitle->setObjectName("pageTitle");
    rootLay->addWidget(pageTitle);

    auto* splitLay = new QHBoxLayout;
    splitLay->setSpacing(16);

    // ── Left: Animal types ────────────────────────────────────────────────────
    auto* leftCard = new QWidget;
    leftCard->setObjectName("settingsCard");
    leftCard->setMinimumWidth(200);
    leftCard->setMaximumWidth(240);
    auto* leftLay = new QVBoxLayout(leftCard);
    leftLay->setContentsMargins(12, 12, 12, 12);
    leftLay->setSpacing(8);

    auto* leftTitle = new QLabel("انواع حیوانات");
    leftTitle->setObjectName("cardTitle");
    leftLay->addWidget(leftTitle);

    auto* divL = new QFrame;
    divL->setFrameShape(QFrame::HLine);
    divL->setObjectName("cardDivider");
    leftLay->addWidget(divL);

    m_animalTypeList = new QListWidget;
    m_animalTypeList->setObjectName("typeList");
    leftLay->addWidget(m_animalTypeList, 1);

    if (Session::instance().isAdmin()) {
        auto* btnRow = new QHBoxLayout;
        m_btnAddType    = new QPushButton("+ افزودن");
        m_btnEditType   = new QPushButton("ویرایش");
        m_btnDeleteType = new QPushButton("حذف");
        m_btnAddType->setObjectName("btnSmallPrimary");
        m_btnEditType->setObjectName("btnSmallSecondary");
        m_btnDeleteType->setObjectName("btnSmallDanger");
        m_btnEditType->setEnabled(false);
        m_btnDeleteType->setEnabled(false);
        btnRow->addWidget(m_btnAddType);
        btnRow->addWidget(m_btnEditType);
        btnRow->addWidget(m_btnDeleteType);
        leftLay->addLayout(btnRow);
    }

    splitLay->addWidget(leftCard);

    // ── Right: Vaccine types ──────────────────────────────────────────────────
    m_detailPanel = new QWidget;
    m_detailPanel->setObjectName("settingsCard");
    auto* rightLay = new QVBoxLayout(m_detailPanel);
    rightLay->setContentsMargins(16, 12, 16, 12);
    rightLay->setSpacing(8);

    auto* rightTitle = new QLabel("واکسن‌های مرتبط");
    rightTitle->setObjectName("cardTitle");
    rightLay->addWidget(rightTitle);

    auto* divR = new QFrame;
    divR->setFrameShape(QFrame::HLine);
    divR->setObjectName("cardDivider");
    rightLay->addWidget(divR);

    auto* selectHint = new QLabel("یک نوع حیوان را از لیست انتخاب کنید");
    selectHint->setObjectName("hintLabel");
    selectHint->setAlignment(Qt::AlignCenter);
    rightLay->addWidget(selectHint, 1, Qt::AlignCenter);

    m_vaccineList = new QListWidget;
    m_vaccineList->setObjectName("typeList");
    m_vaccineList->hide();
    rightLay->addWidget(m_vaccineList, 1);

    if (Session::instance().isAdmin()) {
        auto* vaccBtnRow = new QHBoxLayout;
        m_btnAddVaccine    = new QPushButton("+ افزودن واکسن");
        m_btnEditVaccine   = new QPushButton("ویرایش");
        m_btnDeleteVaccine = new QPushButton("حذف");
        m_btnAddVaccine->setObjectName("btnSmallPrimary");
        m_btnEditVaccine->setObjectName("btnSmallSecondary");
        m_btnDeleteVaccine->setObjectName("btnSmallDanger");
        m_btnAddVaccine->setEnabled(false);
        m_btnEditVaccine->setEnabled(false);
        m_btnDeleteVaccine->setEnabled(false);
        vaccBtnRow->addWidget(m_btnAddVaccine);
        vaccBtnRow->addWidget(m_btnEditVaccine);
        vaccBtnRow->addWidget(m_btnDeleteVaccine);
        rightLay->addLayout(vaccBtnRow);
    }

    splitLay->addWidget(m_detailPanel, 1);
    rootLay->addLayout(splitLay, 1);

    connect(m_animalTypeList, &QListWidget::currentRowChanged,
            this, &AnimalTypesTab::onAnimalTypeSelected);

    if (Session::instance().isAdmin()) {
        connect(m_btnAddType,    &QPushButton::clicked, this, &AnimalTypesTab::onAddAnimalType);
        connect(m_btnEditType,   &QPushButton::clicked, this, &AnimalTypesTab::onEditAnimalType);
        connect(m_btnDeleteType, &QPushButton::clicked, this, &AnimalTypesTab::onDeleteAnimalType);
        connect(m_btnAddVaccine,    &QPushButton::clicked, this, &AnimalTypesTab::onAddVaccineType);
        connect(m_btnEditVaccine,   &QPushButton::clicked, this, &AnimalTypesTab::onEditVaccineType);
        connect(m_btnDeleteVaccine, &QPushButton::clicked, this, &AnimalTypesTab::onDeleteVaccineType);

        connect(m_vaccineList, &QListWidget::currentRowChanged, this, [this](int row) {
            bool valid = (row >= 0);
            m_btnEditVaccine->setEnabled(valid);
            m_btnDeleteVaccine->setEnabled(valid);
            m_selectedVaccineTypeId = valid
                                          ? m_vaccineList->currentItem()->data(Qt::UserRole).toInt()
                                          : -1;
        });
    }

    loadAnimalTypes();
    applyStyle();
}

void AnimalTypesTab::loadAnimalTypes()
{
    m_animalTypeList->clear();
    QSqlQuery q;
    q.exec("SELECT id, name, emoji FROM animal_types ORDER BY id");
    while (q.next()) {
        auto* item = new QListWidgetItem(
            q.value("emoji").toString() + "  " + q.value("name").toString());
        item->setData(Qt::UserRole, q.value("id").toInt());
        m_animalTypeList->addItem(item);
    }
}

void AnimalTypesTab::loadVaccineTypes(int animalTypeId)
{
    m_vaccineList->clear();
    QSqlQuery q;
    q.prepare(
        "SELECT vt.id, vt.name, vt.default_reminder_days "
        "FROM vaccine_types vt "
        "JOIN vaccine_type_animals vta ON vta.vaccine_type_id = vt.id "
        "WHERE vta.animal_type_id = :atid ORDER BY vt.name");
    q.bindValue(":atid", animalTypeId);
    q.exec();
    while (q.next()) {
        QString txt = q.value("name").toString() +
                      "  —  " + QString::number(q.value("default_reminder_days").toInt()) + " روز";
        auto* item = new QListWidgetItem(txt);
        item->setData(Qt::UserRole, q.value("id").toInt());
        m_vaccineList->addItem(item);
    }
}

void AnimalTypesTab::onAnimalTypeSelected(int row)
{
    if (row < 0) { clearDetail(); return; }

    m_selectedAnimalTypeId  = m_animalTypeList->item(row)->data(Qt::UserRole).toInt();
    m_selectedVaccineTypeId = -1;

    if (m_vaccineList) {
        m_vaccineList->show();
        auto* hint = m_detailPanel->findChild<QLabel*>("hintLabel");
        if (hint) hint->hide();
    }

    loadVaccineTypes(m_selectedAnimalTypeId);

    if (Session::instance().isAdmin()) {
        m_btnEditType->setEnabled(true);
        m_btnDeleteType->setEnabled(true);
        m_btnAddVaccine->setEnabled(true);
        m_btnEditVaccine->setEnabled(false);
        m_btnDeleteVaccine->setEnabled(false);
    }
}

void AnimalTypesTab::clearDetail()
{
    m_selectedAnimalTypeId  = -1;
    m_selectedVaccineTypeId = -1;
    if (m_vaccineList) m_vaccineList->hide();
    if (Session::instance().isAdmin()) {
        m_btnEditType->setEnabled(false);
        m_btnDeleteType->setEnabled(false);
        m_btnAddVaccine->setEnabled(false);
        m_btnEditVaccine->setEnabled(false);
        m_btnDeleteVaccine->setEnabled(false);
    }
}

void AnimalTypesTab::onAddAnimalType()
{
    AddAnimalTypeDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    QSqlQuery q;
    q.prepare("INSERT INTO animal_types (name, emoji, badge_bg, badge_fg) "
              "VALUES (:name, :emoji, :bg, :fg)");
    q.bindValue(":name",  dlg.name());
    q.bindValue(":emoji", dlg.emoji().isEmpty() ? "🐾" : dlg.emoji());
    q.bindValue(":bg",    dlg.badgeBg());
    q.bindValue(":fg",    dlg.badgeFg());

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا",
                                "خطا در افزودن نوع حیوان:\n" + q.lastError().text());
        return;
    }

    AnimalTypeInfo::clearCache();
    loadAnimalTypes();
    StyledMessageBox::success(this, "موفق", "نوع حیوان با موفقیت اضافه شد.");
}

void AnimalTypesTab::onEditAnimalType()
{
    if (m_selectedAnimalTypeId < 0) return;

    AddAnimalTypeDialog dlg(this, m_selectedAnimalTypeId);
    if (dlg.exec() != QDialog::Accepted) return;

    QSqlQuery q;
    q.prepare("UPDATE animal_types SET name=:name, emoji=:emoji, "
              "badge_bg=:bg, badge_fg=:fg WHERE id=:id");
    q.bindValue(":name",  dlg.name());
    q.bindValue(":emoji", dlg.emoji().isEmpty() ? "🐾" : dlg.emoji());
    q.bindValue(":bg",    dlg.badgeBg());
    q.bindValue(":fg",    dlg.badgeFg());
    q.bindValue(":id",    m_selectedAnimalTypeId);

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا",
                                "خطا در ویرایش نوع حیوان:\n" + q.lastError().text());
        return;
    }

    AnimalTypeInfo::clearCache();
    loadAnimalTypes();
    StyledMessageBox::success(this, "موفق", "نوع حیوان با موفقیت ویرایش شد.");
}

void AnimalTypesTab::onDeleteAnimalType()
{
    if (m_selectedAnimalTypeId < 0) return;

    QSqlQuery count;
    count.prepare(
        "SELECT COUNT(*) as animal_count, "
        "(SELECT COUNT(*) FROM vaccinations v JOIN animals a ON v.animal_id=a.id "
        " WHERE a.animal_type_id=:id) as vacc_count "
        "FROM animals WHERE animal_type_id=:id2");
    count.bindValue(":id",  m_selectedAnimalTypeId);
    count.bindValue(":id2", m_selectedAnimalTypeId);
    count.exec(); count.next();

    int animalCount = count.value("animal_count").toInt();
    int vaccCount   = count.value("vacc_count").toInt();

    QString msg = QString(
                      "با حذف این نوع حیوان:\n\n"
                      "• %1 حیوان حذف می‌شود\n"
                      "• %2 رکورد واکسیناسیون حذف می‌شود\n\n"
                      "این عملیات غیرقابل بازگشت است. ادامه می‌دهید؟"
                      ).arg(animalCount).arg(vaccCount);

    if (!StyledMessageBox::question(this, "هشدار حذف", msg)) return;

    QSqlQuery q;
    q.prepare("DELETE FROM animal_types WHERE id=:id");
    q.bindValue(":id", m_selectedAnimalTypeId);

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا", "خطا در حذف:\n" + q.lastError().text());
        return;
    }

    AnimalTypeInfo::clearCache();
    clearDetail();
    loadAnimalTypes();
    StyledMessageBox::success(this, "موفق", "نوع حیوان با موفقیت حذف شد.");
}

void AnimalTypesTab::onAddVaccineType()
{
    if (m_selectedAnimalTypeId < 0) return;

    AddVaccineTypeDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    QSqlQuery q;
    q.prepare("INSERT INTO vaccine_types (name, default_reminder_days) VALUES (:name, :days)");
    q.bindValue(":name", dlg.name());
    q.bindValue(":days", dlg.reminderDays());

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا",
                                "خطا در افزودن نوع واکسن:\n" + q.lastError().text());
        return;
    }

    int newVaccineId = q.lastInsertId().toInt();
    QSqlQuery link;
    link.prepare("INSERT INTO vaccine_type_animals (vaccine_type_id, animal_type_id) "
                 "VALUES (:vid, :atid)");
    link.bindValue(":vid",  newVaccineId);
    link.bindValue(":atid", m_selectedAnimalTypeId);
    link.exec();

    loadVaccineTypes(m_selectedAnimalTypeId);
    StyledMessageBox::success(this, "موفق", "نوع واکسن با موفقیت اضافه شد.");
}

void AnimalTypesTab::onEditVaccineType()
{
    if (m_selectedVaccineTypeId < 0) return;

    AddVaccineTypeDialog dlg(this, m_selectedVaccineTypeId);
    if (dlg.exec() != QDialog::Accepted) return;

    QSqlQuery q;
    q.prepare("UPDATE vaccine_types SET name=:name, default_reminder_days=:days WHERE id=:id");
    q.bindValue(":name", dlg.name());
    q.bindValue(":days", dlg.reminderDays());
    q.bindValue(":id",   m_selectedVaccineTypeId);

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا",
                                "خطا در ویرایش نوع واکسن:\n" + q.lastError().text());
        return;
    }

    loadVaccineTypes(m_selectedAnimalTypeId);
    StyledMessageBox::success(this, "موفق", "نوع واکسن با موفقیت ویرایش شد.");
}

void AnimalTypesTab::onDeleteVaccineType()
{
    if (m_selectedVaccineTypeId < 0) return;

    QSqlQuery count;
    count.prepare("SELECT COUNT(*) FROM vaccinations WHERE vaccine_type_id=:id");
    count.bindValue(":id", m_selectedVaccineTypeId);
    count.exec(); count.next();
    int vaccCount = count.value(0).toInt();

    QString msg = QString(
                      "با حذف این نوع واکسن:\n\n"
                      "• %1 رکورد واکسیناسیون حذف می‌شود\n\n"
                      "این عملیات غیرقابل بازگشت است. ادامه می‌دهید؟"
                      ).arg(vaccCount);

    if (!StyledMessageBox::question(this, "هشدار حذف", msg)) return;

    QSqlQuery q;
    q.prepare("DELETE FROM vaccine_types WHERE id=:id");
    q.bindValue(":id", m_selectedVaccineTypeId);

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا", "خطا در حذف:\n" + q.lastError().text());
        return;
    }

    m_selectedVaccineTypeId = -1;
    loadVaccineTypes(m_selectedAnimalTypeId);
    StyledMessageBox::success(this, "موفق", "نوع واکسن با موفقیت حذف شد.");
}

void AnimalTypesTab::applyStyle()
{
    setStyleSheet(R"(
        QLabel#pageTitle {
            font-size: 18px; font-weight: bold; color: #212121; background: transparent;
        }
        QWidget#settingsCard {
            background: white; border: 1px solid #E8F5E9; border-radius: 10px;
        }
        QLabel#cardTitle {
            font-size: 14px; font-weight: bold; color: #2E7D32; background: transparent;
        }
        QFrame#cardDivider { color: #E8F5E9; }
        QLabel#hintLabel { color: #BDBDBD; font-size: 13px; background: transparent; }
        QListWidget#typeList {
            border: none; background: transparent; font-size: 13px;
        }
        QListWidget#typeList::item {
            padding: 8px 10px; border-radius: 6px; color: #212121;
        }
        QListWidget#typeList::item:selected { background: #E8F5E9; color: #2E7D32; }
        QListWidget#typeList::item:hover    { background: #F1F8E9; }
        QPushButton#btnSmallPrimary {
            background: #2E7D32; color: white; border: none;
            border-radius: 5px; padding: 5px 10px; font-size: 12px;
        }
        QPushButton#btnSmallPrimary:hover    { background: #1B5E20; }
        QPushButton#btnSmallPrimary:disabled { background: #A5D6A7; }
        QPushButton#btnSmallSecondary {
            background: white; color: #2E7D32;
            border: 1px solid #A5D6A7; border-radius: 5px;
            padding: 5px 10px; font-size: 12px;
        }
        QPushButton#btnSmallSecondary:hover    { background: #F1F8E9; }
        QPushButton#btnSmallSecondary:disabled { color: #BDBDBD; border-color: #E0E0E0; }
        QPushButton#btnSmallDanger {
            background: white; color: #C62828;
            border: 1px solid #FFCDD2; border-radius: 5px;
            padding: 5px 10px; font-size: 12px;
        }
        QPushButton#btnSmallDanger:hover    { background: #FFEBEE; }
        QPushButton#btnSmallDanger:disabled { color: #BDBDBD; border-color: #E0E0E0; }
    )");
}