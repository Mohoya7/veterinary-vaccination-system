#ifndef VACCINATIONSWIDGET_H
#define VACCINATIONSWIDGET_H

#include <QWidget>
#include <QDate>
#include "persiandatepicker.h"

class QSqlQuery;
class QPushButton;

namespace Ui { class VaccinationsWidget; }

class VaccinationsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VaccinationsWidget(QWidget *parent = nullptr);
    ~VaccinationsWidget();
    void loadData();
    void showVaccinationById(int vacId);

private slots:
    void onFiltersChanged();
    void onDateModeChanged();
    void onClearDateClicked();
    void onAddVaccineClicked();

private:
    void applyStyle();
    void loadVaccineTypeCombo();
    void loadStats();
    void loadTable();           // reset + load first page
    void appendRows(int offset); // append next page into existing table

    QWidget* makeBadge(const QString& text, const QString& bg, const QString& fg);
    QWidget* makeActionButtons(int vaccinationId);

    // Animal-picker dialog
    int  showAnimalPickerDialog();

    // helpers for building the shared WHERE clause
    QString buildWhereClause() const;
    void    bindWhereParams(QSqlQuery& q) const;

    Ui::VaccinationsWidget *ui;
    bool m_rangeMode = true;

    // Persian date pickers — جایگزین QDateEdit های UI
    PersianDatePicker* m_pickerFrom   = nullptr;
    PersianDatePicker* m_pickerTo     = nullptr;
    PersianDatePicker* m_pickerSingle = nullptr;

    // pagination
    int  m_offset = 0;
    static constexpr int kPageSize = 50;
    QPushButton* m_loadMoreBtn = nullptr;
};

#endif // VACCINATIONSWIDGET_H