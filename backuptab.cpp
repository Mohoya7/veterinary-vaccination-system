#include "backuptab.h"
#include "database.h"
#include "styledmessagebox.h"
#include "persiandate.h"

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

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

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

    // ── Card 1: Settings ─────────────────────────────────────────────────────
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
    pathRowWidget->setLayout(pathRow);
    settingsLay->addWidget(makeLabeledField("مسیر ذخیره بکاپ *", pathRowWidget));

    // Interval
    m_intervalSpin = new QSpinBox;
    m_intervalSpin->setRange(1, 365);
    m_intervalSpin->setValue(1);
    m_intervalSpin->setSuffix(" روز");
    settingsLay->addWidget(makeLabeledField("فاصله بکاپ‌گیری خودکار *", m_intervalSpin));

    // ── Password field with show/hide and change button ───────────────────────
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("رمز عبور فایل بکاپ");

    m_btnShowPassword = new QPushButton("👁");
    m_btnShowPassword->setObjectName("btnEye");
    m_btnShowPassword->setFixedSize(36, 36);
    m_btnShowPassword->setCheckable(true);
    m_btnShowPassword->setToolTip("نمایش/مخفی کردن رمز");

    auto* pwdInputRow = new QHBoxLayout;
    pwdInputRow->setSpacing(6);
    pwdInputRow->addWidget(m_passwordEdit, 1);
    pwdInputRow->addWidget(m_btnShowPassword);
    auto* pwdInputWidget = new QWidget;
    pwdInputWidget->setLayout(pwdInputRow);

    // Status label — shown after password is saved
    m_passwordStatusLbl = new QLabel("رمز تنظیم شده ✓");
    m_passwordStatusLbl->setObjectName("pwdStatusLabel");
    m_passwordStatusLbl->hide();

    // Change password button — disabled until password is set once
    m_btnChangePassword = new QPushButton("تغییر رمز بکاپ");
    m_btnChangePassword->setObjectName("btnSecondary");
    m_btnChangePassword->setEnabled(false);

    auto* pwdFullLay = new QVBoxLayout;
    pwdFullLay->setContentsMargins(0, 0, 0, 0);
    pwdFullLay->setSpacing(6);
    pwdFullLay->addWidget(makeLabeledField("رمز عبور فایل زیپ *", pwdInputWidget));
    pwdFullLay->addWidget(m_passwordStatusLbl);

    auto* pwdBtnRow = new QHBoxLayout;
    pwdBtnRow->addStretch();
    pwdBtnRow->addWidget(m_btnChangePassword);
    pwdFullLay->addLayout(pwdBtnRow);

    auto* pwdFullWidget = new QWidget;
    pwdFullWidget->setLayout(pwdFullLay);
    settingsLay->addWidget(pwdFullWidget);

    // Save button
    m_btnSave = new QPushButton("ذخیره تنظیمات");
    m_btnSave->setObjectName("btnPrimary");
    auto* saveBtnRow = new QHBoxLayout;
    saveBtnRow->addStretch();
    saveBtnRow->addWidget(m_btnSave);
    settingsLay->addLayout(saveBtnRow);

    rootLay->addWidget(makeCard("تنظیمات بکاپ", settingsLay));

    // ── Card 2: Instant backup ───────────────────────────────────────────────
    m_lastBackupLabel = new QLabel("آخرین بکاپ: هنوز بکاپی گرفته نشده");
    m_lastBackupLabel->setObjectName("infoLabel");

    m_btnBackupNow = new QPushButton("بکاپ فوری");
    m_btnBackupNow->setObjectName("btnPrimary");

    auto* nowLay = new QVBoxLayout;
    nowLay->setSpacing(12);
    nowLay->addWidget(m_lastBackupLabel);
    auto* nowBtnRow = new QHBoxLayout;
    nowBtnRow->addStretch();
    nowBtnRow->addWidget(m_btnBackupNow);
    nowLay->addLayout(nowBtnRow);

    rootLay->addWidget(makeCard("بکاپ فوری", nowLay));
    rootLay->addStretch();

    // ── Connections ──────────────────────────────────────────────────────────
    connect(m_btnBrowse,    &QPushButton::clicked, this, &BackupTab::onBrowsePath);
    connect(m_btnSave,      &QPushButton::clicked, this, &BackupTab::onSaveSettings);
    connect(m_btnBackupNow, &QPushButton::clicked, this, &BackupTab::onBackupNow);

    // Show/hide password toggle
    connect(m_btnShowPassword, &QPushButton::toggled, this, [this](bool checked) {
        m_passwordEdit->setEchoMode(
            checked ? QLineEdit::Normal : QLineEdit::Password);
        m_btnShowPassword->setText(checked ? "🙈" : "👁");
    });

    // Change password — ask for old password first, then new password
    connect(m_btnChangePassword, &QPushButton::clicked, this, [this]() {

        // Build inline dialog for old + new password
        QDialog dlg(this);
        dlg.setWindowTitle("تغییر رمز بکاپ");
        dlg.setLayoutDirection(Qt::RightToLeft);
        dlg.setFixedWidth(340);
        dlg.setStyleSheet("QDialog { background: #F1F8E9; }");

        auto* lay = new QVBoxLayout(&dlg);
        lay->setContentsMargins(20, 20, 20, 20);
        lay->setSpacing(12);

        auto* titleLbl = new QLabel("تغییر رمز فایل بکاپ");
        titleLbl->setStyleSheet("font-size:14px;font-weight:bold;color:#212121;");
        lay->addWidget(titleLbl);

        auto makeField = [&](const QString& label) -> QLineEdit* {
            auto* lbl  = new QLabel(label);
            lbl->setStyleSheet("font-size:12px;color:#757575;");
            auto* edit = new QLineEdit;
            edit->setEchoMode(QLineEdit::Password);
            edit->setStyleSheet(
                "border:1px solid #A5D6A7;border-radius:6px;"
                "padding:8px 10px;font-size:13px;min-height:36px;");
            lay->addWidget(lbl);
            lay->addWidget(edit);
            return edit;
        };

        auto* oldEdit     = makeField("رمز فعلی *");
        auto* newEdit     = makeField("رمز جدید *");
        auto* confirmEdit = makeField("تکرار رمز جدید *");

        auto* btnRow = new QHBoxLayout;
        auto* btnCancel = new QPushButton("انصراف");
        auto* btnOk     = new QPushButton("تغییر رمز");
        btnCancel->setStyleSheet(
            "background:white;color:#757575;border:1px solid #E0E0E0;"
            "border-radius:6px;padding:8px 20px;font-size:13px;");
        btnOk->setStyleSheet(
            "background:#2E7D32;color:white;border:none;"
            "border-radius:6px;padding:8px 20px;font-size:13px;");
        btnRow->addStretch();
        btnRow->addWidget(btnCancel);
        btnRow->addWidget(btnOk);
        lay->addLayout(btnRow);

        connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(btnOk, &QPushButton::clicked, &dlg, [&]() {
            // Verify old password against DB
            QSqlQuery q;
            q.exec("SELECT backup_password FROM backup_settings WHERE id = 1");
            if (!q.next() || q.value(0).toString() != oldEdit->text()) {
                StyledMessageBox::warning(&dlg, "خطا", "رمز فعلی اشتباه است.");
                return;
            }
            if (newEdit->text().isEmpty()) {
                StyledMessageBox::warning(&dlg, "خطا", "رمز جدید نمیتواند خالی باشد.");
                return;
            }
            if (newEdit->text() != confirmEdit->text()) {
                StyledMessageBox::warning(&dlg, "خطا", "رمز جدید و تکرار آن یکسان نیستند.");
                return;
            }
            // Save new password
            QSqlQuery upd;
            upd.prepare("UPDATE backup_settings SET backup_password = :pwd WHERE id = 1");
            upd.bindValue(":pwd", newEdit->text());
            if (!upd.exec()) {
                StyledMessageBox::error(&dlg, "خطا", "خطا در ذخیره رمز جدید.");
                return;
            }
            dlg.accept();
        });

        if (dlg.exec() == QDialog::Accepted) {
            StyledMessageBox::success(this, "موفق", "رمز بکاپ با موفقیت تغییر کرد.");
        }
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

    QDateTime last = q.value("last_backup_at").toDateTime();
    if (last.isValid())
        m_lastBackupLabel->setText("آخرین بکاپ: " +
                                   PersianDate::toDisplayShort(last.date()));

    // If password already set — show status label, hide input, enable change button
    QString pwd = q.value("backup_password").toString();
    if (!pwd.isEmpty()) {
        m_passwordIsSet = true;
        m_passwordEdit->setVisible(false);
        m_btnShowPassword->setVisible(false);
        m_passwordStatusLbl->show();
        m_btnChangePassword->setEnabled(true);
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

    // If password not yet set and field is empty
    if (!m_passwordIsSet && m_passwordEdit->text().isEmpty()) {
        StyledMessageBox::warning(this, "خطا", "لطفاً رمز عبور فایل زیپ را وارد کنید.");
        return;
    }

    QSqlQuery q;

    if (!m_passwordIsSet) {
        // Saving new password
        q.prepare("UPDATE backup_settings SET "
                  "backup_path = :path, interval_days = :days, "
                  "is_auto_enabled = 1, backup_password = :pwd, "
                  "updated_at = NOW() WHERE id = 1");
        q.bindValue(":pwd", m_passwordEdit->text());
    } else {
        // Password already set — don't overwrite it
        q.prepare("UPDATE backup_settings SET "
                  "backup_path = :path, interval_days = :days, "
                  "is_auto_enabled = 1, updated_at = NOW() WHERE id = 1");
    }

    q.bindValue(":path", path);
    q.bindValue(":days", m_intervalSpin->value());

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا", "خطا در ذخیره تنظیمات:\n" + q.lastError().text());
        return;
    }

    // After saving new password — switch to status mode
    if (!m_passwordIsSet) {
        m_passwordIsSet = true;
        m_passwordEdit->clear();
        m_passwordEdit->setVisible(false);
        m_btnShowPassword->setVisible(false);
        m_passwordStatusLbl->show();
        m_btnChangePassword->setEnabled(true);
    }

    StyledMessageBox::success(this, "موفق", "تنظیمات بکاپ با موفقیت ذخیره شد.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Immediate backup
// ─────────────────────────────────────────────────────────────────────────────

void BackupTab::onBackupNow()
{
    // Read password from DB
    QSqlQuery q;
    q.exec("SELECT backup_password FROM backup_settings WHERE id = 1");
    if (!q.next() || q.value(0).toString().isEmpty()) {
        StyledMessageBox::warning(this, "خطا",
                                  "لطفاً ابتدا رمز عبور بکاپ را در تنظیمات وارد کنید.");
        return;
    }
    QString password = q.value(0).toString();

    QString folder = QFileDialog::getExistingDirectory(
        this, "انتخاب پوشه برای ذخیره بکاپ", m_pathEdit->text());
    if (folder.isEmpty()) return;

    m_btnBackupNow->setEnabled(false);
    m_btnBackupNow->setText("در حال بکاپ‌گیری...");

    QString error;
    if (!runBackup(folder, password, error)) {
        StyledMessageBox::error(this, "خطا", "بکاپ ناموفق بود:\n" + error);
        m_btnBackupNow->setEnabled(true);
        m_btnBackupNow->setText("بکاپ فوری");
        return;
    }

    QSqlQuery upd;
    upd.prepare("UPDATE backup_settings SET last_backup_at = NOW() WHERE id = 1");
    upd.exec();

    m_lastBackupLabel->setText("آخرین بکاپ: " +
                               PersianDate::toDisplayShort(QDate::currentDate()));

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
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString sqlFile   = folder + "/backup_" + timestamp + ".sql";
    QString zipFile   = folder + "/backup_" + timestamp + ".zip";

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
    for (const QString& p : searchPaths) {
        if (QFileInfo::exists(p)) { mysqldump = p; break; }
    }

    QStringList dumpArgs = {
        QString("--host=%1").arg(host),
        QString("--port=%1").arg(port),
        QString("--user=%1").arg(user),
        QString("--password=%1").arg(pass),
        "--single-transaction",
        "--routines",
        "--triggers",
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

    // ── Step 2: 7za zip with password ───────────────────────────────────────
    QString sevenZip = "7z";
    QStringList zipPaths = {
        QApplication::applicationDirPath() + "/7za.exe",
        "C:/Program Files/7-Zip/7z.exe",
        "C:/Program Files (x86)/7-Zip/7z.exe"
    };
    for (const QString& p : zipPaths) {
        if (QFileInfo::exists(p)) { sevenZip = p; break; }
    }

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

    bool     autoEnabled = q.value("is_auto_enabled").toBool();
    QString  path        = q.value("backup_path").toString();
    QString  password    = q.value("backup_password").toString();
    int      interval    = q.value("interval_days").toInt();
    QDateTime lastBackup = q.value("last_backup_at").toDateTime();

    if (!autoEnabled || path.isEmpty() || password.isEmpty()) return;
    if (!QDir(path).exists()) return;

    bool due = !lastBackup.isValid() ||
               lastBackup.daysTo(QDateTime::currentDateTime()) >= interval;
    if (!due) return;

    QString error;
    if (runBackup(path, password, error)) {
        QSqlQuery upd;
        upd.prepare("UPDATE backup_settings SET last_backup_at = NOW() WHERE id = 1");
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
        QLabel#pwdStatusLabel {
            font-size: 13px; color: #2E7D32; background: #E8F5E9;
            border-radius: 6px; padding: 8px 12px; border: 1px solid #A5D6A7;
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
        QPushButton#btnEye {
            background: white; border: 1px solid #A5D6A7;
            border-radius: 6px; font-size: 16px;
        }
        QPushButton#btnEye:hover { background: #F1F8E9; }
    )");
}