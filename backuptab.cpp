#include "backuptab.h"
#include "database.h"
#include "styledmessagebox.h"
#include "persiandate.h"
#include "session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDialog>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QApplication>
#include <QToolButton>
#include <QSvgRenderer>
#include <QRegularExpressionValidator>
#include <QPainter>

// ── Helper: eye toggle button ─────────────────────────────────────────────────
static QToolButton* makeEyeBtn(QWidget* parent)
{
    auto* btn = new QToolButton(parent);
    btn->setFixedSize(36, 36);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setCheckable(true);
    btn->setChecked(false);
    btn->setStyleSheet(R"(
        QToolButton { background: transparent; border: none; padding: 0px; }
        QToolButton:hover { background: transparent; }
    )");

    auto setIcon = [btn](bool visible) {
        QString path = visible ? ":/icons/eye.svg" : ":/icons/eye-off.svg";
        QSvgRenderer renderer(path);
        QPixmap px(18, 18);
        px.fill(Qt::transparent);
        QPainter p(&px);
        renderer.render(&p);
        p.end();
        QPixmap tinted(px.size());
        tinted.fill(Qt::transparent);
        QPainter pt(&tinted);
        pt.drawPixmap(0, 0, px);
        pt.setCompositionMode(QPainter::CompositionMode_SourceIn);
        pt.fillRect(tinted.rect(), QColor("#9E9E9E"));
        pt.end();
        btn->setIcon(QIcon(tinted));
        btn->setIconSize(QSize(18, 18));
    };
    setIcon(false);

    QObject::connect(btn, &QToolButton::toggled, [btn, setIcon](bool checked) {
        setIcon(checked);
    });

    return btn;
}

// ── Helper: password field with eye button ────────────────────────────────────
static QWidget* makePasswordRow(QLineEdit* edit, QToolButton* eyeBtn)
{
    auto* w   = new QWidget;
    auto* lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);
    lay->addWidget(edit);
    lay->addWidget(eyeBtn);

    QObject::connect(eyeBtn, &QToolButton::toggled, [edit](bool checked) {
        edit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    return w;
}

// ── Helper: apply password constraints ───────────────────────────────────────
static void applyPasswordConstraints(QLineEdit* edit)
{
    auto* v = new QRegularExpressionValidator(
        QRegularExpression("[a-zA-Z0-9@#$%&*!._\\-+=?/]*"), edit);
    edit->setValidator(v);
    edit->setMaxLength(30);
}

// ── Helper: card ──────────────────────────────────────────────────────────────
static QWidget* makeCard(const QString& title, QLayout* content)
{
    auto* card = new QWidget;
    card->setObjectName("settingsCard");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 16, 20, 20);
    lay->setSpacing(14);

    auto* titleLbl = new QLabel(title);
    titleLbl->setObjectName("cardTitle");
    lay->addWidget(titleLbl);

    auto* div = new QFrame;
    div->setFrameShape(QFrame::HLine);
    div->setObjectName("cardDivider");
    lay->addWidget(div);

    lay->addLayout(content);
    return card;
}

static QWidget* makeLabeledField(const QString& label, QWidget* field)
{
    auto* w   = new QWidget;
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);
    auto* lbl = new QLabel(label);
    lbl->setObjectName("fieldLabel");
    lay->addWidget(lbl);
    lay->addWidget(field);
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

BackupTab::BackupTab(QWidget* parent)
    : QWidget(parent)
{
    setLayoutDirection(Qt::RightToLeft);

    auto* rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(24, 24, 24, 24);
    rootLay->setSpacing(20);

    auto* pageTitle = new QLabel("بکاپ");
    pageTitle->setObjectName("pageTitle");
    rootLay->addWidget(pageTitle);

    // ── Card 1: Backup settings ───────────────────────────────────────────────
    auto* settingsLay = new QVBoxLayout;
    settingsLay->setSpacing(12);

    // Path row
    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText("مسیر پوشه ذخیره بکاپ...");
    m_pathEdit->setReadOnly(true);

    m_btnBrowse = new QPushButton("انتخاب پوشه");
    m_btnBrowse->setObjectName("btnSecondary");

    auto* pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(m_btnBrowse);
    auto* pathRowWidget = new QWidget;
    pathRowWidget->setStyleSheet("background:transparent;");
    pathRowWidget->setLayout(pathRow);
    settingsLay->addWidget(makeLabeledField("مسیر ذخیره بکاپ *", pathRowWidget));

    // Interval — label shows (بر حسب روز), only numbers accepted
    auto* intervalLbl = new QLabel("فاصله بکاپ‌گیری خودکار * (بر حسب روز)");
    intervalLbl->setObjectName("fieldLabel");
    m_intervalSpin = new QSpinBox;
    m_intervalSpin->setRange(1, 365);
    m_intervalSpin->setValue(1);
    // Numbers only — no suffix text, just plain number
    auto* intervalLay = new QVBoxLayout;
    intervalLay->setContentsMargins(0, 0, 0, 0);
    intervalLay->setSpacing(4);
    intervalLay->addWidget(intervalLbl);
    intervalLay->addWidget(m_intervalSpin);
    auto* intervalWidget = new QWidget;
    intervalWidget->setStyleSheet("background:transparent;");
    intervalWidget->setLayout(intervalLay);
    settingsLay->addWidget(intervalWidget);

    // Last backup info — in settings card
    m_lastBackupLabel = new QLabel("آخرین بکاپ: هنوز بکاپی گرفته نشده");
    m_lastBackupLabel->setObjectName("infoLabel");
    settingsLay->addWidget(m_lastBackupLabel);

    // Password section — change button only, no input/status label here
    m_btnChangePassword = new QPushButton("تغییر رمز بکاپ");
    m_btnChangePassword->setObjectName("btnSecondary");

    // If no password set yet, change button acts as "set password"
    // Will be enabled/disabled based on loadSettings()
    auto* pwdBtnRow = new QHBoxLayout;
    pwdBtnRow->addStretch();
    pwdBtnRow->addWidget(m_btnChangePassword);
    settingsLay->addLayout(pwdBtnRow);

    // Save button — right aligned
    m_btnSave = new QPushButton("ذخیره تنظیمات");
    m_btnSave->setObjectName("btnPrimary");
    auto* saveBtnRow = new QHBoxLayout;
    saveBtnRow->addStretch();
    saveBtnRow->addWidget(m_btnSave);
    settingsLay->addLayout(saveBtnRow);

    rootLay->addWidget(makeCard("تنظیمات بکاپ", settingsLay));

    // ── Card 2: Instant backup ────────────────────────────────────────────────
    auto* nowLay = new QVBoxLayout;
    nowLay->setSpacing(12);

    m_btnBackupNow = new QPushButton("بکاپ فوری");
    m_btnBackupNow->setObjectName("btnPrimary");

    // Right aligned
    auto* nowBtnRow = new QHBoxLayout;
    nowBtnRow->addStretch();
    nowBtnRow->addWidget(m_btnBackupNow);
    nowLay->addLayout(nowBtnRow);

    rootLay->addWidget(makeCard("بکاپ فوری", nowLay));
    rootLay->addStretch();

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_btnBrowse,    &QPushButton::clicked, this, &BackupTab::onBrowsePath);
    connect(m_btnSave,      &QPushButton::clicked, this, &BackupTab::onSaveSettings);
    connect(m_btnBackupNow, &QPushButton::clicked, this, &BackupTab::onBackupNow);

    // Change/set password dialog
    connect(m_btnChangePassword, &QPushButton::clicked, this, [this]() {

        QDialog dlg(this);
        dlg.setWindowTitle("تغییر رمز فایل بکاپ");
        dlg.setLayoutDirection(Qt::RightToLeft);
        dlg.setFixedWidth(360);
        dlg.setStyleSheet("QDialog { background: #F1F8E9; }");

        auto* vlay = new QVBoxLayout(&dlg);
        vlay->setContentsMargins(0, 0, 0, 0);
        vlay->setSpacing(0);

        // Header
        auto* hdr = new QWidget;
        hdr->setFixedHeight(48);
        hdr->setStyleSheet("QWidget{background:#2E7D32;}");
        auto* hdrLay = new QHBoxLayout(hdr);
        hdrLay->setContentsMargins(16, 0, 16, 0);
        auto* ttl = new QLabel(m_passwordIsSet ? "تغییر رمز فایل بکاپ" : "تنظیم رمز فایل بکاپ");
        ttl->setStyleSheet("color:white;font-size:14px;font-weight:500;background:transparent;");
        hdrLay->addWidget(ttl);
        vlay->addWidget(hdr);

        // Body
        auto* body = new QWidget;
        body->setStyleSheet("background:white;");
        auto* bodyLay = new QVBoxLayout(body);
        bodyLay->setContentsMargins(20, 20, 20, 16);
        bodyLay->setSpacing(12);

        QString fieldStyle =
            "QLineEdit{border:1px solid #A5D6A7;border-radius:6px;"
            "padding:7px 10px;font-size:13px;background:#F9FBF9;"
            "color:#212121;min-height:36px;}"
            "QLineEdit:focus{border-color:#2E7D32;background:white;}";

        auto makePassField = [&](const QString& label) -> QLineEdit* {
            auto* lbl = new QLabel(label);
            lbl->setStyleSheet("font-size:12px;color:#757575;background:transparent;");
            auto* edit = new QLineEdit;
            edit->setEchoMode(QLineEdit::Password);
            edit->setStyleSheet(fieldStyle);
            applyPasswordConstraints(edit);
            auto* eyeBtn = makeEyeBtn(body);
            auto* row = makePasswordRow(edit, eyeBtn);
            bodyLay->addWidget(lbl);
            bodyLay->addWidget(row);
            return edit;
        };

        // First field: admin password (for verification), then new + confirm
        auto* adminPassEdit = makePassField("رمز عبور ادمین *");
        auto* newEdit       = makePassField("رمز جدید بکاپ *");
        auto* confirmEdit   = makePassField("تکرار رمز جدید *");

        vlay->addWidget(body);

        // Footer
        auto* footer = new QWidget;
        footer->setStyleSheet("background:white;border-top:1px solid #E8F5E9;");
        footer->setFixedHeight(56);
        auto* footerLay = new QHBoxLayout(footer);
        footerLay->setContentsMargins(16, 0, 16, 0);
        footerLay->setSpacing(8);

        auto* btnCancel = new QPushButton("انصراف");
        auto* btnOk     = new QPushButton("ذخیره");
        btnCancel->setFixedHeight(36);
        btnOk->setFixedHeight(36);
        btnCancel->setStyleSheet(
            "QPushButton{background:white;color:#757575;border:1px solid #E0E0E0;"
            "border-radius:6px;font-size:13px;padding:0 20px;}"
            "QPushButton:hover{background:#F5F5F5;}");
        btnOk->setStyleSheet(
            "QPushButton{background:#2E7D32;color:white;border:none;"
            "border-radius:6px;font-size:13px;padding:0 20px;}"
            "QPushButton:hover{background:#1B5E20;}");

        footerLay->addStretch();
        footerLay->addWidget(btnCancel);
        footerLay->addWidget(btnOk);
        vlay->addWidget(footer);

        connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(btnOk, &QPushButton::clicked, &dlg, [&]() {
            // Step 1: check admin field not empty
            if (adminPassEdit->text().isEmpty()) {
                StyledMessageBox::warning(&dlg, "خطا", "لطفاً رمز عبور ادمین را وارد کنید.");
                return;
            }
            // Step 2: verify admin password against DB
            QString adminHash = Database::hashPassword(adminPassEdit->text());
            QSqlQuery check;
            check.prepare(
                "SELECT id FROM users WHERE role='admin' AND password_hash=:hash");
            check.bindValue(":hash", adminHash);
            check.exec();
            if (!check.next()) {
                StyledMessageBox::warning(&dlg, "خطا", "رمز عبور ادمین اشتباه است.");
                return;
            }
            // Step 3: validate new password fields
            if (newEdit->text().isEmpty() || confirmEdit->text().isEmpty()) {
                StyledMessageBox::warning(&dlg, "خطا", "لطفاً رمز جدید و تکرار آن را وارد کنید.");
                return;
            }
            if (newEdit->text().length() < 4) {
                StyledMessageBox::warning(&dlg, "خطا", "رمز باید حداقل ۴ کاراکتر باشد.");
                return;
            }
            if (newEdit->text() != confirmEdit->text()) {
                StyledMessageBox::warning(&dlg, "خطا", "رمز جدید و تکرار آن یکسان نیستند.");
                return;
            }
            // Hash the backup password before storing
            QString hashedPwd = Database::hashPassword(newEdit->text());
            QSqlQuery upd;
            upd.prepare(
                "UPDATE backup_settings SET backup_password=:pwd, "
                "updated_at=NOW() WHERE id=1");
            upd.bindValue(":pwd", hashedPwd);
            if (!upd.exec()) {
                StyledMessageBox::error(&dlg, "خطا", "خطا در ذخیره رمز.");
                return;
            }
            m_passwordIsSet = true;
            dlg.accept();
        });

        if (dlg.exec() == QDialog::Accepted)
            StyledMessageBox::success(this, "موفق", "رمز بکاپ با موفقیت تنظیم شد.");
    });

    loadSettings();
    applyStyle();
}

// ─────────────────────────────────────────────────────────────────────────────
// Load settings from DB
// ─────────────────────────────────────────────────────────────────────────────

void BackupTab::loadSettings()
{
    QSqlQuery q;
    q.exec("SELECT backup_path, interval_days, last_backup_at, backup_password "
           "FROM backup_settings WHERE id = 1");
    if (!q.next()) return;

    QString path = q.value("backup_path").toString();
    if (!path.isEmpty())
        m_pathEdit->setText(path);

    m_intervalSpin->setValue(q.value("interval_days").toInt());

    // Last backup — update label in settings card
    QDateTime last = q.value("last_backup_at").toDateTime();
    if (last.isValid())
        m_lastBackupLabel->setText(
            "آخرین بکاپ: " + PersianDate::toDisplayShort(last.date()));

    // Password already set?
    QString pwd = q.value("backup_password").toString();
    if (!pwd.isEmpty()) {
        m_passwordIsSet = true;
        m_btnChangePassword->setText("تغییر رمز بکاپ");
    } else {
        m_passwordIsSet = false;
        m_btnChangePassword->setText("تنظیم رمز بکاپ");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Browse folder
// ─────────────────────────────────────────────────────────────────────────────

void BackupTab::onBrowsePath()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "انتخاب پوشه بکاپ", m_pathEdit->text());
    if (!dir.isEmpty())
        m_pathEdit->setText(dir);
}

// ─────────────────────────────────────────────────────────────────────────────
// Save settings
// ─────────────────────────────────────────────────────────────────────────────

void BackupTab::onSaveSettings()
{
    QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty()) {
        StyledMessageBox::warning(this, "خطا", "لطفاً پوشه بکاپ را انتخاب کنید.");
        return;
    }
    if (!QDir(path).exists()) {
        StyledMessageBox::warning(this, "خطا", "پوشه انتخاب شده وجود ندارد.");
        return;
    }
    if (!m_passwordIsSet) {
        StyledMessageBox::warning(this, "خطا",
                                  "لطفاً ابتدا رمز بکاپ را از طریق دکمه «تنظیم رمز بکاپ» تنظیم کنید.");
        return;
    }

    QSqlQuery q;
    q.prepare("UPDATE backup_settings SET "
              "backup_path=:path, interval_days=:days, "
              "is_auto_enabled=1, updated_at=NOW() WHERE id=1");
    q.bindValue(":path", path);
    q.bindValue(":days", m_intervalSpin->value());

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا",
                                "خطا در ذخیره تنظیمات:\n" + q.lastError().text());
        return;
    }

    StyledMessageBox::success(this, "موفق", "تنظیمات بکاپ با موفقیت ذخیره شد.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Instant backup — choose folder + folder name
// ─────────────────────────────────────────────────────────────────────────────

void BackupTab::onBackupNow()
{
    // Read hashed password from DB — we pass hash directly to zip
    QSqlQuery q;
    q.exec("SELECT backup_password FROM backup_settings WHERE id = 1");
    if (!q.next() || q.value(0).toString().isEmpty()) {
        StyledMessageBox::warning(this, "خطا",
                                  "لطفاً ابتدا رمز بکاپ را در تنظیمات تنظیم کنید.");
        return;
    }
    QString password = q.value(0).toString();

    // Dialog: choose base folder + folder name
    QDialog pickerDlg(this);
    pickerDlg.setWindowTitle("انتخاب مقصد بکاپ");
    pickerDlg.setLayoutDirection(Qt::RightToLeft);
    pickerDlg.setFixedWidth(400);
    pickerDlg.setStyleSheet("QDialog{background:#F1F8E9;}");

    auto* vlay = new QVBoxLayout(&pickerDlg);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);

    // Header
    auto* hdr = new QWidget;
    hdr->setFixedHeight(48);
    hdr->setStyleSheet("QWidget{background:#2E7D32;}");
    auto* hdrLay = new QHBoxLayout(hdr);
    hdrLay->setContentsMargins(16, 0, 16, 0);
    auto* ttl = new QLabel("انتخاب مقصد بکاپ فوری");
    ttl->setStyleSheet("color:white;font-size:14px;font-weight:500;background:transparent;");
    hdrLay->addWidget(ttl);
    vlay->addWidget(hdr);

    // Body
    auto* body = new QWidget;
    body->setStyleSheet("background:white;");
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(20, 20, 20, 16);
    bodyLay->setSpacing(12);

    QString fieldStyle =
        "QLineEdit{border:1px solid #A5D6A7;border-radius:6px;"
        "padding:7px 10px;font-size:13px;background:#F9FBF9;"
        "color:#212121;min-height:36px;}"
        "QLineEdit:focus{border-color:#2E7D32;background:white;}";

    // Base folder
    auto* folderEdit = new QLineEdit;
    folderEdit->setReadOnly(true);
    folderEdit->setPlaceholderText("مسیر پوشه مقصد...");
    folderEdit->setStyleSheet(fieldStyle);
    if (!m_pathEdit->text().isEmpty())
        folderEdit->setText(m_pathEdit->text());

    auto* btnPickFolder = new QPushButton("انتخاب پوشه");
    btnPickFolder->setStyleSheet(
        "QPushButton{background:white;color:#2E7D32;border:1px solid #A5D6A7;"
        "border-radius:6px;padding:7px 12px;font-size:12px;}"
        "QPushButton:hover{background:#F1F8E9;}");
    auto* folderRow = new QHBoxLayout;
    folderRow->setSpacing(6);
    folderRow->addWidget(folderEdit, 1);
    folderRow->addWidget(btnPickFolder);
    auto* folderRowW = new QWidget; folderRowW->setStyleSheet("background:transparent;");
    folderRowW->setLayout(folderRow);

    auto* folderLbl = new QLabel("پوشه مقصد *");
    folderLbl->setStyleSheet("font-size:12px;color:#757575;background:transparent;");
    bodyLay->addWidget(folderLbl);
    bodyLay->addWidget(folderRowW);

    // Folder name
    QString defaultName = "backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    auto* nameEdit = new QLineEdit;
    nameEdit->setPlaceholderText("نام پوشه بکاپ");
    nameEdit->setText(defaultName);
    nameEdit->setStyleSheet(fieldStyle);

    auto* nameLbl = new QLabel("نام پوشه بکاپ *");
    nameLbl->setStyleSheet("font-size:12px;color:#757575;background:transparent;");
    bodyLay->addWidget(nameLbl);
    bodyLay->addWidget(nameEdit);

    vlay->addWidget(body);

    // Footer
    auto* footer = new QWidget;
    footer->setStyleSheet("background:white;border-top:1px solid #E8F5E9;");
    footer->setFixedHeight(56);
    auto* footerLay = new QHBoxLayout(footer);
    footerLay->setContentsMargins(16, 0, 16, 0);
    footerLay->setSpacing(8);

    auto* btnCancel = new QPushButton("انصراف");
    auto* btnOk     = new QPushButton("شروع بکاپ");
    btnCancel->setFixedHeight(36);
    btnOk->setFixedHeight(36);
    btnCancel->setStyleSheet(
        "QPushButton{background:white;color:#757575;border:1px solid #E0E0E0;"
        "border-radius:6px;font-size:13px;padding:0 20px;}"
        "QPushButton:hover{background:#F5F5F5;}");
    btnOk->setStyleSheet(
        "QPushButton{background:#2E7D32;color:white;border:none;"
        "border-radius:6px;font-size:13px;padding:0 20px;}"
        "QPushButton:hover{background:#1B5E20;}");

    footerLay->addStretch();
    footerLay->addWidget(btnCancel);
    footerLay->addWidget(btnOk);
    vlay->addWidget(footer);

    connect(btnPickFolder, &QPushButton::clicked, &pickerDlg, [&]() {
        QString d = QFileDialog::getExistingDirectory(
            &pickerDlg, "انتخاب پوشه مقصد", folderEdit->text());
        if (!d.isEmpty()) folderEdit->setText(d);
    });
    connect(btnCancel, &QPushButton::clicked, &pickerDlg, &QDialog::reject);
    connect(btnOk,     &QPushButton::clicked, &pickerDlg, [&]() {
        if (folderEdit->text().isEmpty()) {
            StyledMessageBox::warning(&pickerDlg, "خطا", "لطفاً پوشه مقصد را انتخاب کنید.");
            return;
        }
        if (nameEdit->text().trimmed().isEmpty()) {
            StyledMessageBox::warning(&pickerDlg, "خطا", "لطفاً نام پوشه بکاپ را وارد کنید.");
            return;
        }
        pickerDlg.accept();
    });

    if (pickerDlg.exec() != QDialog::Accepted) return;

    // Build final folder path
    QString finalFolder = folderEdit->text() + "/" + nameEdit->text().trimmed();
    QDir().mkpath(finalFolder);

    m_btnBackupNow->setEnabled(false);
    m_btnBackupNow->setText("در حال بکاپ‌گیری...");

    QString error;
    if (!runBackup(finalFolder, password, error)) {
        StyledMessageBox::error(this, "خطا", "بکاپ ناموفق بود:\n" + error);
        m_btnBackupNow->setEnabled(true);
        m_btnBackupNow->setText("بکاپ فوری");
        return;
    }

    // Update last_backup_at
    QSqlQuery upd;
    upd.prepare("UPDATE backup_settings SET last_backup_at=NOW() WHERE id=1");
    upd.exec();

    // Update label in settings card
    m_lastBackupLabel->setText(
        "آخرین بکاپ: " + PersianDate::toDisplayShort(QDate::currentDate()));

    m_btnBackupNow->setEnabled(true);
    m_btnBackupNow->setText("بکاپ فوری");

    StyledMessageBox::success(this, "موفق", "بکاپ با موفقیت گرفته شد.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Core backup: mysqldump + 7za password-protected zip
// ─────────────────────────────────────────────────────────────────────────────

bool BackupTab::runBackup(const QString& folder,
                          const QString& password,
                          QString& errorOut)
{
    QString sqlFile = folder + "/backup.sql";
    QString zipFile = folder + "/backup.zip";

    QSqlDatabase db = QSqlDatabase::database();
    QString host    = db.hostName();
    QString dbName  = db.databaseName();
    QString user    = db.userName();
    QString pass    = db.password();
    int     port    = db.port();

    // ── Step 1: mysqldump ────────────────────────────────────────────────────
    QString mysqldump = "mysqldump";
    QStringList searchPaths = {
        "C:/Program Files/MySQL/MySQL Server 8.0/bin/mysqldump.exe",
        "C:/Program Files/MySQL/MySQL Server 8.4/bin/mysqldump.exe",
        "C:/xampp/mysql/bin/mysqldump.exe"
    };
    for (const QString& p : searchPaths)
        if (QFileInfo::exists(p)) { mysqldump = p; break; }

    QStringList dumpArgs = {
        QString("--host=%1").arg(host),
        QString("--port=%1").arg(port),
        QString("--user=%1").arg(user),
        QString("--password=%1").arg(pass),
        "--single-transaction", "--routines", "--triggers",
        "--set-gtid-purged=OFF",
        "--result-file=" + sqlFile,
        dbName
    };

    QProcess dumpProc;
    dumpProc.start(mysqldump, dumpArgs);
    if (!dumpProc.waitForStarted(5000)) {
        errorOut = "mysqldump پیدا نشد. مطمئن شوید MySQL نصب است.";
        return false;
    }
    if (!dumpProc.waitForFinished(120000)) {
        dumpProc.kill();
        errorOut = "mysqldump timeout شد.";
        return false;
    }
    if (dumpProc.exitCode() != 0) {
        errorOut = "خطای mysqldump: " + QString(dumpProc.readAllStandardError());
        QFile::remove(sqlFile);
        return false;
    }

    // ── Step 2: 7za zip with password ────────────────────────────────────────
    QString sevenZip = "7z";
    QStringList zipPaths = {
        QApplication::applicationDirPath() + "/7za.exe",
        "C:/Program Files/7-Zip/7z.exe",
        "C:/Program Files (x86)/7-Zip/7z.exe"
    };
    for (const QString& p : zipPaths)
        if (QFileInfo::exists(p)) { sevenZip = p; break; }

    QStringList zipArgs = {
        "a", "-tzip",
        QString("-p%1").arg(password),
        "-mem=AES256",
        zipFile, sqlFile
    };

    QProcess zipProc;
    zipProc.start(sevenZip, zipArgs);
    if (!zipProc.waitForStarted(5000)) {
        errorOut = "7-Zip پیدا نشد. فایل SQL بدون رمز ذخیره شد: " + sqlFile;
        return false;
    }
    if (!zipProc.waitForFinished(60000)) {
        zipProc.kill();
        QFile::remove(sqlFile);
        errorOut = "فشرده‌سازی timeout شد.";
        return false;
    }
    if (zipProc.exitCode() != 0) {
        errorOut = "خطای 7-Zip: " + QString(zipProc.readAllStandardError());
        QFile::remove(sqlFile);
        return false;
    }

    QFile::remove(sqlFile);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Auto-backup check on app startup
// ─────────────────────────────────────────────────────────────────────────────

void BackupTab::checkAndRunAutoBackup()
{
    QSqlQuery q;
    q.exec("SELECT backup_path, interval_days, is_auto_enabled, "
           "last_backup_at, backup_password "
           "FROM backup_settings WHERE id = 1");
    if (!q.next()) return;

    bool      autoEnabled = q.value("is_auto_enabled").toBool();
    QString   path        = q.value("backup_path").toString();
    QString   password    = q.value("backup_password").toString();
    int       interval    = q.value("interval_days").toInt();
    QDateTime lastBackup  = q.value("last_backup_at").toDateTime();

    if (!autoEnabled || path.isEmpty() || password.isEmpty()) return;
    if (!QDir(path).exists()) return;

    bool due = !lastBackup.isValid() ||
               lastBackup.daysTo(QDateTime::currentDateTime()) >= interval;
    if (!due) return;

    // Auto-backup folder name includes date
    QString autoFolder = path + "/auto_" +
                         QDate::currentDate().toString("yyyyMMdd");
    QDir().mkpath(autoFolder);

    QString error;
    if (runBackup(autoFolder, password, error)) {
        QSqlQuery upd;
        upd.prepare("UPDATE backup_settings SET last_backup_at=NOW() WHERE id=1");
        upd.exec();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Style
// ─────────────────────────────────────────────────────────────────────────────

void BackupTab::applyStyle()
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
        QLabel#fieldLabel {
            font-size: 12px; color: #757575; background: transparent;
        }
        QLabel#infoLabel {
            font-size: 13px; color: #424242; background: #F1F8E9;
            border-radius: 6px; padding: 8px 12px; border: 1px solid #C8E6C9;
        }
        QLineEdit, QSpinBox {
            border: 1px solid #A5D6A7; border-radius: 6px;
            padding: 8px 10px; font-size: 13px;
            background: #F9FBF9; color: #212121; min-height: 36px;
        }
        QLineEdit:focus, QSpinBox:focus { border-color: #2E7D32; background: white; }
        QLineEdit[readOnly="true"] { background: #F5F5F5; color: #757575; }
        QSpinBox::up-button, QSpinBox::down-button {
            border: none; width: 20px; background: transparent;
        }
        QPushButton#btnPrimary {
            background: #2E7D32; color: white; border: none;
            border-radius: 6px; padding: 8px 24px; font-size: 13px;
        }
        QPushButton#btnPrimary:hover { background: #1B5E20; }
        QPushButton#btnPrimary:disabled { background: #A5D6A7; }
        QPushButton#btnSecondary {
            background: white; color: #2E7D32;
            border: 1px solid #A5D6A7; border-radius: 6px;
            padding: 8px 16px; font-size: 13px;
        }
        QPushButton#btnSecondary:hover { background: #F1F8E9; }
        QPushButton#btnSecondary:disabled { color: #BDBDBD; border-color: #E0E0E0; }
    )");
}