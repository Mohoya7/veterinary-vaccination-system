#ifndef USERSTAB_H
#define USERSTAB_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

// ─────────────────────────────────────────────────────────────────────────────
// UsersTab — User management tab in settings
//
// Admin:
//   - Change own password (current + new + confirm) + eye buttons
//   - Change technician password (admin confirmation + new + confirm) + eye buttons
//
// Technician:
//   - Change own password only (current + new + confirm) + eye buttons
//
// All password fields: max 30 chars, no spaces/tabs, allowed chars regex
// ─────────────────────────────────────────────────────────────────────────────

class UsersTab : public QWidget
{
    Q_OBJECT

public:
    explicit UsersTab(QWidget* parent = nullptr);

private slots:
    void onChangeOwnPassword();
    void onResetTechPassword();

private:
    void buildAdminView();
    void buildTechnicianView();
    void applyStyle();
    void loadTechnicianInfo();

    // Own password fields
    QLineEdit* m_currentPassEdit   = nullptr;
    QLineEdit* m_newPassEdit       = nullptr;
    QLineEdit* m_confirmPassEdit   = nullptr;

    // Admin-only: tech section
    QLineEdit* m_adminConfirmEdit  = nullptr; // admin password confirmation
    QLineEdit* m_techNewPassEdit   = nullptr;
    QLineEdit* m_techConfirmEdit   = nullptr;

    int m_techUserId = -1;
};

#endif // USERSTAB_H