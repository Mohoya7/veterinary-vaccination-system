#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QSqlQuery>
#include <QHBoxLayout>

namespace Ui {
class DashboardWidget;
}

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();

    void loadData();

private:
    void loadStats();
    void loadTodayReminders();
    QWidget* wrapCenter(QWidget* inner);

    Ui::DashboardWidget *ui;
};

#endif // DASHBOARDWIDGET_H
