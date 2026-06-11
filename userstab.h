#ifndef USERSTAB_H
#define USERSTAB_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

// ─────────────────────────────────────────────────────────────────────────────
// UsersTab — User management tab in settings
//
// Admin sees:
//   - Change own password (current + new + confirm)
//   - Technician account section: view username, set new password directly
//
// Technician sees:
//   - Change own password only (current + new + confirm)
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

    // Change own password fields
    QLineEdit* m_currentPassEdit = nullptr;
    QLineEdit* m_newPassEdit     = nullptr;
    QLineEdit* m_confirmPassEdit = nullptr;

    // Admin-only: technician section
    QLabel*    m_techUsernameLabel = nullptr;
    QLineEdit* m_techNewPassEdit   = nullptr;
    QLineEdit* m_techConfirmEdit   = nullptr;

    int m_techUserId = -1;
};

#endif // USERSTAB_H