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
#include "persiandatepicker.h"

class QSqlQuery;

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
    void onLoadMoreClicked();
    void onExportPdfClicked();
    void onFollowUpChanged(int rfId, bool checked);
    void onOwnerResponseChanged(int rfId, int responseIndex);

private:
    void applyStyle();
    void loadStats();
    void loadTable();
    void appendRows(int offset);
    void loadVaccineTypeCombo();
    void buildFilterDateWidget();

    QString buildWhereClause() const;
    void    bindWhereParams(QSqlQuery& q) const;

    QWidget* makeBadge(const QString& text, const QString& bg, const QString& fg);
    QWidget* makeResponseCombo(int rfId, int currentResponse);

    Ui::RemindersWidget *ui;

    // ── فیلتر تاریخ ──────────────────────────────────────────────────────────
    QWidget*           m_filterDateWidget  = nullptr;
    // کامبو انتخاب حالت (بر اساس روز / بازه تاریخ)
    QComboBox*         m_subModeCombo      = nullptr;
    // حالت دستی
    QSpinBox*          m_daysSpin          = nullptr;
    QPushButton*       m_dirBtn            = nullptr;  // ← toggle آینده/گذشته
    // حالت بازه
    PersianDatePicker* m_pickerFrom        = nullptr;
    PersianDatePicker* m_pickerTo          = nullptr;

    bool m_subManualMode = true; // true=بر اساس روز، false=بازه تاریخ

    // pagination
    int  m_offset = 0;
    static constexpr int kPageSize = 100;
    QPushButton* m_loadMoreBtn = nullptr;
};

#endif