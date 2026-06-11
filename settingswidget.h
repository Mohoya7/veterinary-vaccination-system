#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>

// ─────────────────────────────────────────────────────────────────────────────
// SettingsWidget — Main settings page with tab navigation
// Tabs: Users | Animal Types & Vaccines | Backup | About
// Admin-only tabs are hidden for technician role
// ─────────────────────────────────────────────────────────────────────────────

class UsersTab;
class AnimalTypesTab;
class BackupTab;
class AboutTab;

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget* parent = nullptr);
    ~SettingsWidget() = default;

private:
    void applyStyle();
    void buildSidebar();
    void switchTab(int index);

    // Sidebar nav buttons
    QPushButton* m_btnUsers       = nullptr;
    QPushButton* m_btnAnimalTypes = nullptr;
    QPushButton* m_btnBackup      = nullptr;
    QPushButton* m_btnAbout       = nullptr;

    QStackedWidget* m_stack = nullptr;

    UsersTab*       m_usersTab       = nullptr;
    AnimalTypesTab* m_animalTypesTab = nullptr;
    BackupTab*      m_backupTab      = nullptr;
    AboutTab*       m_aboutTab       = nullptr;
};

#endif // SETTINGSWIDGET_H