#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QPainter>
#include <QPropertyAnimation>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class DashboardWidget;
class OwnersWidget;
class VaccinationsWidget;
class RemindersWidget;
class AnimalsWidget;



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

private:
    void uncheckAllButtons();
    void updateToggleIcon();


    Ui::MainWindow     *ui;
    QString             m_role;
    bool                m_sidebarExpanded = true;
    QPropertyAnimation *m_sidebarAnim     = nullptr;
    DashboardWidget    *m_dashboard       = nullptr;
    OwnersWidget       *m_owners          = nullptr;
    RemindersWidget    *m_reminders       = nullptr;
    VaccinationsWidget *m_vaccinations    = nullptr;
    AnimalsWidget *m_animals = nullptr;


    static constexpr int kSidebarExpanded  = 220;
    static constexpr int kSidebarCollapsed = 0;
    static constexpr int kAnimDuration     = 220;
};

#endif