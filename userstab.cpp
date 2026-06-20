#include "userstab.h"
#include "session.h"
#include "database.h"
#include "styledmessagebox.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QSqlQuery>
#include <QSqlError>
#include <QToolButton>
#include <QSvgRenderer>
#include <QPainter>
#include <QRegularExpressionValidator>

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

// ── Helper: password field with eye toggle ────────────────────────────────────
static QWidget* makePasswordWidget(QLineEdit* edit, QWidget* parent)
{
    // Apply constraints: max 30, no space/tab, allowed chars only
    auto* v = new QRegularExpressionValidator(
        QRegularExpression("[a-zA-Z0-9@#$%&*!._\\-+=?/]*"), edit);
    edit->setValidator(v);
    edit->setMaxLength(30);

    auto* eyeBtn = makeEyeBtn(parent);
    QObject::connect(eyeBtn, &QToolButton::toggled, [edit](bool checked) {
        edit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    auto* w   = new QWidget;
    w->setStyleSheet("background:transparent;");
    auto* lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);
    lay->addWidget(edit);
    lay->addWidget(eyeBtn);
    return w;
}

// ── Helper: card ──────────────────────────────────────────────────────────────
static QWidget* makeCard(const QString& title, QLayout* contentLay)
{
    auto* card = new QWidget;
    card->setObjectName("settingsCard");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 16, 20, 20);
    lay->setSpacing(14);

    auto* titleLbl = new QLabel(title);
    titleLbl->setObjectName("cardTitle");
    lay->addWidget(titleLbl);

    auto* divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setObjectName("cardDivider");
    lay->addWidget(divider);

    lay->addLayout(contentLay);
    return card;
}

static QWidget* makeField(const QString& label, QWidget* widget)
{
    auto* row = new QWidget;
    row->setStyleSheet("background:transparent;");
    auto* lay = new QVBoxLayout(row);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);
    auto* lbl = new QLabel(label);
    lbl->setObjectName("fieldLabel");
    lay->addWidget(lbl);
    lay->addWidget(widget);
    return row;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UsersTab::UsersTab(QWidget* parent)
    : QWidget(parent)
{
    setLayoutDirection(Qt::RightToLeft);

    if (Session::instance().isAdmin())
        buildAdminView();
    else
        buildTechnicianView();

    applyStyle();
}

// ─────────────────────────────────────────────────────────────────────────────
// Admin view
// ─────────────────────────────────────────────────────────────────────────────

void UsersTab::buildAdminView()
{
    auto* rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(24, 24, 24, 24);
    rootLay->setSpacing(20);

    auto* pageTitle = new QLabel("مدیریت کاربران");
    pageTitle->setObjectName("pageTitle");
    rootLay->addWidget(pageTitle);

    // ── Section 1: Change admin password ────────────────────────────────────
    m_currentPassEdit = new QLineEdit;
    m_currentPassEdit->setEchoMode(QLineEdit::Password);
    m_currentPassEdit->setPlaceholderText("رمز فعلی");

    m_newPassEdit = new QLineEdit;
    m_newPassEdit->setEchoMode(QLineEdit::Password);
    m_newPassEdit->setPlaceholderText("رمز جدید");

    m_confirmPassEdit = new QLineEdit;
    m_confirmPassEdit->setEchoMode(QLineEdit::Password);
    m_confirmPassEdit->setPlaceholderText("تکرار رمز جدید");

    auto* btnChangeOwn = new QPushButton("تغییر رمز عبور");
    btnChangeOwn->setObjectName("btnPrimary");

    auto* ownLay = new QVBoxLayout;
    ownLay->setSpacing(10);
    ownLay->addWidget(makeField("رمز عبور فعلی *", makePasswordWidget(m_currentPassEdit, this)));
    ownLay->addWidget(makeField("رمز عبور جدید *",  makePasswordWidget(m_newPassEdit,     this)));
    ownLay->addWidget(makeField("تکرار رمز جدید *", makePasswordWidget(m_confirmPassEdit, this)));

    // Right aligned
    auto* ownBtnRow = new QHBoxLayout;
    ownBtnRow->addStretch();
    ownBtnRow->addWidget(btnChangeOwn);
    ownLay->addLayout(ownBtnRow);

    rootLay->addWidget(makeCard("تغییر رمز عبور ادمین", ownLay));

    // ── Section 2: Technician account ───────────────────────────────────────
    m_techNewPassEdit = new QLineEdit;
    m_techNewPassEdit->setEchoMode(QLineEdit::Password);
    m_techNewPassEdit->setPlaceholderText("رمز جدید تکنسین");

    m_techConfirmEdit = new QLineEdit;
    m_techConfirmEdit->setEchoMode(QLineEdit::Password);
    m_techConfirmEdit->setPlaceholderText("تکرار رمز جدید");

    // Admin must confirm with own password to change tech password
    m_adminConfirmEdit = new QLineEdit;
    m_adminConfirmEdit->setEchoMode(QLineEdit::Password);
    m_adminConfirmEdit->setPlaceholderText("رمز عبور ادمین برای تأیید");

    auto* btnResetTech = new QPushButton("تغییر رمز تکنسین");
    btnResetTech->setObjectName("btnPrimary");

    loadTechnicianInfo();

    auto* techLay = new QVBoxLayout;
    techLay->setSpacing(10);
    techLay->addWidget(makeField("تأیید هویت ادمین *",  makePasswordWidget(m_adminConfirmEdit, this)));
    techLay->addWidget(makeField("رمز عبور جدید *",      makePasswordWidget(m_techNewPassEdit,  this)));
    techLay->addWidget(makeField("تکرار رمز جدید *",     makePasswordWidget(m_techConfirmEdit,  this)));

    // Right aligned
    auto* techBtnRow = new QHBoxLayout;
    techBtnRow->addStretch();
    techBtnRow->addWidget(btnResetTech);
    techLay->addLayout(techBtnRow);

    rootLay->addWidget(makeCard("مدیریت حساب تکنسین", techLay));
    rootLay->addStretch();

    connect(btnChangeOwn,  &QPushButton::clicked, this, &UsersTab::onChangeOwnPassword);
    connect(btnResetTech,  &QPushButton::clicked, this, &UsersTab::onResetTechPassword);
}

// ─────────────────────────────────────────────────────────────────────────────
// Technician view
// ─────────────────────────────────────────────────────────────────────────────

void UsersTab::buildTechnicianView()
{
    auto* rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(24, 24, 24, 24);
    rootLay->setSpacing(20);

    auto* pageTitle = new QLabel("تغییر رمز عبور");
    pageTitle->setObjectName("pageTitle");
    rootLay->addWidget(pageTitle);

    m_currentPassEdit = new QLineEdit;
    m_currentPassEdit->setEchoMode(QLineEdit::Password);
    m_currentPassEdit->setPlaceholderText("رمز فعلی");

    m_newPassEdit = new QLineEdit;
    m_newPassEdit->setEchoMode(QLineEdit::Password);
    m_newPassEdit->setPlaceholderText("رمز جدید");

    m_confirmPassEdit = new QLineEdit;
    m_confirmPassEdit->setEchoMode(QLineEdit::Password);
    m_confirmPassEdit->setPlaceholderText("تکرار رمز جدید");

    auto* btnChange = new QPushButton("تغییر رمز عبور");
    btnChange->setObjectName("btnPrimary");

    auto* lay = new QVBoxLayout;
    lay->setSpacing(10);
    lay->addWidget(makeField("رمز عبور فعلی *", makePasswordWidget(m_currentPassEdit, this)));
    lay->addWidget(makeField("رمز عبور جدید *",  makePasswordWidget(m_newPassEdit,     this)));
    lay->addWidget(makeField("تکرار رمز جدید *", makePasswordWidget(m_confirmPassEdit, this)));

    // Right aligned
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(btnChange);
    lay->addLayout(btnRow);

    rootLay->addWidget(makeCard("تغییر رمز عبور", lay));
    rootLay->addStretch();

    connect(btnChange, &QPushButton::clicked, this, &UsersTab::onChangeOwnPassword);
}

// ─────────────────────────────────────────────────────────────────────────────
// Load technician info
// ─────────────────────────────────────────────────────────────────────────────

void UsersTab::loadTechnicianInfo()
{
    QSqlQuery q;
    q.prepare("SELECT id FROM users WHERE role='technician' LIMIT 1");
    q.exec();
    if (q.next())
        m_techUserId = q.value("id").toInt();
}

// ─────────────────────────────────────────────────────────────────────────────
// Change own password (admin or technician)
// ─────────────────────────────────────────────────────────────────────────────

void UsersTab::onChangeOwnPassword()
{
    QString current = m_currentPassEdit->text();
    QString newPass = m_newPassEdit->text();
    QString confirm = m_confirmPassEdit->text();

    if (current.isEmpty() || newPass.isEmpty() || confirm.isEmpty()) {
        StyledMessageBox::warning(this, "خطا", "لطفاً همه فیلدها را پر کنید.");
        return;
    }
    if (newPass != confirm) {
        StyledMessageBox::warning(this, "خطا", "رمز جدید و تکرار آن یکسان نیستند.");
        return;
    }
    if (newPass.length() < 4) {
        StyledMessageBox::warning(this, "خطا", "رمز عبور باید حداقل ۴ کاراکتر باشد.");
        return;
    }

    // Verify current password
    QString currentHash = Database::hashPassword(current);
    QSqlQuery check;
    check.prepare("SELECT id FROM users WHERE id=:id AND password_hash=:hash");
    check.bindValue(":id",   Session::instance().userId());
    check.bindValue(":hash", currentHash);
    check.exec();

    if (!check.next()) {
        StyledMessageBox::warning(this, "خطا", "رمز عبور فعلی اشتباه است.");
        return;
    }

    QString newHash = Database::hashPassword(newPass);
    QSqlQuery q;
    q.prepare("UPDATE users SET password_hash=:hash, updated_at=NOW() WHERE id=:id");
    q.bindValue(":hash", newHash);
    q.bindValue(":id",   Session::instance().userId());

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا",
                                "خطا در به‌روزرسانی رمز:\n" + q.lastError().text());
        return;
    }

    m_currentPassEdit->clear();
    m_newPassEdit->clear();
    m_confirmPassEdit->clear();
    StyledMessageBox::success(this, "موفق", "رمز عبور با موفقیت تغییر کرد.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Reset technician password (admin only — requires admin password confirmation)
// ─────────────────────────────────────────────────────────────────────────────

void UsersTab::onResetTechPassword()
{
    if (m_techUserId < 0) {
        StyledMessageBox::warning(this, "خطا", "حساب تکنسین پیدا نشد.");
        return;
    }

    QString adminPass = m_adminConfirmEdit ? m_adminConfirmEdit->text() : "";
    QString newPass   = m_techNewPassEdit->text();
    QString confirm   = m_techConfirmEdit->text();

    if (adminPass.isEmpty() || newPass.isEmpty() || confirm.isEmpty()) {
        StyledMessageBox::warning(this, "خطا", "لطفاً همه فیلدها را پر کنید.");
        return;
    }

    // Verify admin password
    QString adminHash = Database::hashPassword(adminPass);
    QSqlQuery check;
    check.prepare(
        "SELECT id FROM users WHERE id=:id AND password_hash=:hash AND role='admin'");
    check.bindValue(":id",   Session::instance().userId());
    check.bindValue(":hash", adminHash);
    check.exec();

    if (!check.next()) {
        StyledMessageBox::warning(this, "خطا", "رمز عبور ادمین اشتباه است.");
        return;
    }

    if (newPass != confirm) {
        StyledMessageBox::warning(this, "خطا", "رمز جدید و تکرار آن یکسان نیستند.");
        return;
    }
    if (newPass.length() < 4) {
        StyledMessageBox::warning(this, "خطا", "رمز عبور باید حداقل ۴ کاراکتر باشد.");
        return;
    }

    QString newHash = Database::hashPassword(newPass);
    QSqlQuery q;
    q.prepare("UPDATE users SET password_hash=:hash, updated_at=NOW() WHERE id=:id");
    q.bindValue(":hash", newHash);
    q.bindValue(":id",   m_techUserId);

    if (!q.exec()) {
        StyledMessageBox::error(this, "خطا",
                                "خطا در بازنشانی رمز:\n" + q.lastError().text());
        return;
    }

    if (m_adminConfirmEdit) m_adminConfirmEdit->clear();
    m_techNewPassEdit->clear();
    m_techConfirmEdit->clear();
    StyledMessageBox::success(this, "موفق", "رمز تکنسین با موفقیت تغییر کرد.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Style
// ─────────────────────────────────────────────────────────────────────────────

void UsersTab::applyStyle()
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
        QLineEdit {
            border: 1px solid #A5D6A7; border-radius: 6px;
            padding: 8px 10px; font-size: 13px;
            background: #F9FBF9; color: #212121; min-height: 36px;
        }
        QLineEdit:focus { border-color: #2E7D32; background: white; }
        QPushButton#btnPrimary {
            background: #2E7D32; color: white; border: none;
            border-radius: 6px; padding: 8px 24px; font-size: 13px;
        }
        QPushButton#btnPrimary:hover { background: #1B5E20; }
    )");
}