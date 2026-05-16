#ifndef REMINDERSWIDGET_H
#define REMINDERSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QDate>

namespace Ui { class RemindersWidget; }

class RemindersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RemindersWidget(QWidget *parent = nullptr);
    ~RemindersWidget();
    void loadData();

private slots:
    void onFiltersChanged();
    void onExportPdfClicked();
    void onFollowUpChanged(int rfId, bool checked);
    void onOwnerResponseChanged(int rfId, int responseIndex);

private:
    void applyStyle();
    void loadStats();
    void loadTable();
    QWidget* makeBadge(const QString& text, const QString& bg, const QString& fg);
    QWidget* makeResponseCombo(int rfId, int currentResponse);

    Ui::RemindersWidget *ui;
};

#endif