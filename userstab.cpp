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

UsersTab::UsersTab(QWidget* parent)
    : QWidget(parent)
{
    setLayoutDirection(Qt::RightToLeft);

    if (Session::instance().isAdmin()) {
        buildAdminView();
    } else {
        buildTechnicianView();
    }

    applyStyle();
}

// ── Helper: build a section card ─────────────────────────────────────────────

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

static QWidget* makeField(const QString& label, QLineEdit* edit)
{
    auto* row = new QWidget;
    auto* lay = new QVBoxLayout(row);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);
    auto* lbl = new QLabel(label);
    lbl->setObjectName("fieldLabel");
    lay->addWidget(lbl);
    lay->addWidget(edit);
    return row;
}

// ── Admin view ────────────────────────────────────────────────────────────────

void UsersTab::buildAdminView()
{
    auto* rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(24, 24, 24, 24);
    rootLay->setSpacing(20);

    auto* pageTitle = new QLabel("مدیریت کاربران");
    pageTitle->setObjectName("pageTitle");
    rootLay->addWidget(pageTitle);

    // ── Section 1: Change own password ──────────────────────────────────────
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
    ownLay->addWidget(makeField("رمز عبور فعلی *", m_currentPassEdit));
    ownLay->addWidget(makeField("رمز عبور جدید *", m_newPassEdit));
    ownLay->addWidget(makeField("تکرار رمز جدید *", m_confirmPassEdit));

    auto* ownBtnRow = new QHBoxLayout;
    ownBtnRow->addStretch();
    ownBtnRow->addWidget(btnChangeOwn);
    ownLay->addLayout(ownBtnRow);

    rootLay->addWidget(makeCard("تغییر رمز عبور ادمین", ownLay));

    // ── Section 2: Technician account ───────────────────────────────────────
    m_techUsernameLabel = new QLabel;
    m_techUsernameLabel->setObjectName("techUsernameLabel");
    m_techUsernameLabel->setWordWrap(false);
    m_techUsernameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    loadTechnicianInfo();

    m_techNewPassEdit = new QLineEdit;
    m_techNewPassEdit->setEchoMode(QLineEdit::Password);
    m_techNewPassEdit->setPlaceholderText("رمز جدید تکنسین");

    m_techConfirmEdit = new QLineEdit;
    m_techConfirmEdit->setEchoMode(QLineEdit::Password);
    m_techConfirmEdit->setPlaceholderText("تکرار رمز جدید");

    auto* btnResetTech = new QPushButton("تغییر رمز تکنسین");
    btnResetTech->setObjectName("btnPrimary");

    auto* techLay = new QVBoxLayout;
    techLay->setSpacing(10);
    techLay->addWidget(m_techUsernameLabel);
    techLay->addWidget(makeField("رمز عبور جدید *", m_techNewPassEdit));
    techLay->addWidget(makeField("تکرار رمز جدید *", m_techConfirmEdit));

    auto* techBtnRow = new QHBoxLayout;
    techBtnRow->addStretch();
    techBtnRow->addWidget(btnResetTech);
    techLay->addLayout(techBtnRow);

    rootLay->addWidget(makeCard("مدیریت حساب تکنسین", techLay));

    rootLay->addStretch();

    connect(btnChangeOwn, &QPushButton::clicked, this, &UsersTab::onChangeOwnPassword);
    connect(btnResetTech, &QPushButton::clicked, this, &UsersTab::onResetTechPassword);
}

// ── Technician view ───────────────────────────────────────────────────────────

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
    lay->addWidget(makeField("رمز عبور فعلی *", m_currentPassEdit));
    lay->addWidget(makeField("رمز عبور جدید *", m_newPassEdit));
    lay->addWidget(makeField("تکرار رمز جدید *", m_confirmPassEdit));

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(btnChange);
    lay->addLayout(btnRow);

    rootLay->addWidget(makeCard("تغییر رمز عبور", lay));
    rootLay->addStretch();

    connect(btnChange, &QPushButton::clicked, this, &UsersTab::onChangeOwnPassword);
}

// ── Load technician info ──────────────────────────────────────────────────────

void UsersTab::loadTechnicianInfo()
{
    QSqlQuery q;
    q.prepare("SELECT id, username FROM users WHERE role = 'technician' LIMIT 1");
    q.exec();
    if (q.next()) {
        m_techUserId = q.value("id").toInt();
        if (m_techUsernameLabel)
            m_techUsernameLabel->setText("نام کاربری: " + q.value("username").toString());
    }
}

// ── Change own password ───────────────────────────────────────────────────────

void UsersTab::onChangeOwnPassword()
{
    QString current = m_currentPassEdit->text();
    QString newPass = m_newPassEdit->text();
    QString confirm = m_confirmPassEdit->text();

    if (current.isEmpty() || newPass.isEmpty() || confirm.isEmpty()) {
        StyledMessageBox::warning(this, "Error", "Please fill in all fields.");
        return;
    }
    if (newPass != confirm) {
        StyledMessageBox::warning(this, "Error", "New passwords do not match.");
        return;
    }
    if (newPass.length() < 4) {
        StyledMessageBox::warning(this, "Error", "Password must be at least 4 characters.");
        return;
    }

    // Verify current password
    QString currentHash = Database::hashPassword(current);
    QSqlQuery check;
    check.prepare("SELECT id FROM users WHERE id = :id AND password_hash = :hash");
    check.bindValue(":id",   Session::instance().userId());
    check.bindValue(":hash", currentHash);
    check.exec();

    if (!check.next()) {
        StyledMessageBox::warning(this, "Error", "Current password is incorrect.");
        return;
    }

    // Update password
    QString newHash = Database::hashPassword(newPass);
    QSqlQuery q;
    q.prepare("UPDATE users SET password_hash = :hash, updated_at = NOW() WHERE id = :id");
    q.bindValue(":hash", newHash);
    q.bindValue(":id",   Session::instance().userId());

    if (!q.exec()) {
        StyledMessageBox::error(this, "Error", "Failed to update password:\n" + q.lastError().text());
        return;
    }

    m_currentPassEdit->clear();
    m_newPassEdit->clear();
    m_confirmPassEdit->clear();
    StyledMessageBox::success(this, "Success", "Password changed successfully.");
}

// ── Reset technician password (admin only) ────────────────────────────────────

void UsersTab::onResetTechPassword()
{
    if (m_techUserId < 0) {
        StyledMessageBox::warning(this, "Error", "No technician account found.");
        return;
    }

    QString newPass = m_techNewPassEdit->text();
    QString confirm = m_techConfirmEdit->text();

    if (newPass.isEmpty() || confirm.isEmpty()) {
        StyledMessageBox::warning(this, "Error", "Please fill in all fields.");
        return;
    }
    if (newPass != confirm) {
        StyledMessageBox::warning(this, "Error", "Passwords do not match.");
        return;
    }
    if (newPass.length() < 4) {
        StyledMessageBox::warning(this, "Error", "Password must be at least 4 characters.");
        return;
    }

    QString newHash = Database::hashPassword(newPass);
    QSqlQuery q;
    q.prepare("UPDATE users SET password_hash = :hash, updated_at = NOW() WHERE id = :id");
    q.bindValue(":hash", newHash);
    q.bindValue(":id",   m_techUserId);

    if (!q.exec()) {
        StyledMessageBox::error(this, "Error", "Failed to reset password:\n" + q.lastError().text());
        return;
    }

    m_techNewPassEdit->clear();
    m_techConfirmEdit->clear();
    StyledMessageBox::success(this, "Success", "Technician password reset successfully.");
}

// ── Style ─────────────────────────────────────────────────────────────────────

void UsersTab::applyStyle()
{
    setStyleSheet(R"(
        QLabel#pageTitle {
            font-size: 18px; font-weight: bold; color: #212121;
            background: transparent;
        }
        QWidget#settingsCard {
            background: white;
            border: 1px solid #E8F5E9;
            border-radius: 10px;
        }
        QLabel#cardTitle {
            font-size: 14px; font-weight: bold; color: #2E7D32;
            background: transparent;
        }
        QFrame#cardDivider { color: #E8F5E9; }
        QLabel#fieldLabel {
            font-size: 12px; color: #757575; background: transparent;
        }
        QLabel#techUsernameLabel {
            font-size: 13px; color: #212121; background: #F1F8E9;
            border-radius: 6px; padding: 8px 12px;
            border: 1px solid #C8E6C9;
            min-height: 20px;
            max-height: 40px;
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