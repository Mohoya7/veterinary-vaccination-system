#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QPainter>
#include <QPropertyAnimation>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class DashboardWidget;
class OwnersWidget;
class VaccinationsWidget;
class RemindersWidget;
class AnimalsWidget;
class UsersTab;
class AnimalTypesTab;
class BackupTab;
class AboutTab;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& role, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNavButtonClicked();
    void onLogoutClicked();
    void onToggleSidebar();
    void onSettingsClicked();

private:
    void uncheckAllButtons();
    void uncheckAllSubButtons();
    void updateToggleIcon();
    void setSettingsExpanded(bool expanded);

    Ui::MainWindow     *ui;
    QString             m_role;
    bool                m_sidebarExpanded  = true;
    bool                m_settingsExpanded = false;
    QPropertyAnimation *m_sidebarAnim      = nullptr;

    // Settings sub-buttons in sidebar
    QWidget*     m_settingsSubWidget  = nullptr;
    QPushButton* m_btnSubUsers        = nullptr;
    QPushButton* m_btnSubAnimalTypes  = nullptr;
    QPushButton* m_btnSubBackup       = nullptr;
    QPushButton* m_btnSubAbout        = nullptr;

    // Content pages
    DashboardWidget    *m_dashboard    = nullptr;
    AnimalsWidget      *m_animals      = nullptr;
    OwnersWidget       *m_owners       = nullptr;
    RemindersWidget    *m_reminders    = nullptr;
    VaccinationsWidget *m_vaccinations = nullptr;
    UsersTab           *m_usersTab     = nullptr;
    AnimalTypesTab     *m_animalTypes  = nullptr;
    BackupTab          *m_backup       = nullptr;
    AboutTab           *m_about        = nullptr;

    // contentStack indices
    static constexpr int kIdxDashboard   = 0;
    static constexpr int kIdxAnimals     = 1;
    static constexpr int kIdxOwners      = 2;
    static constexpr int kIdxReminders   = 3;
    static constexpr int kIdxVaccinations= 4;
    static constexpr int kIdxUsers       = 5;
    static constexpr int kIdxAnimalTypes = 6;
    static constexpr int kIdxBackup      = 7;
    static constexpr int kIdxAbout       = 8;

    static constexpr int kSidebarExpanded  = 220;
    static constexpr int kSidebarCollapsed = 0;
    static constexpr int kAnimDuration     = 220;
};

#endif