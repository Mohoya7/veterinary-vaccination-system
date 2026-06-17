#ifndef VACCINATIONSWIDGET_H
#define VACCINATIONSWIDGET_H

#include <QWidget>
#include <QDate>
#include <QEvent>
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

signals:
    void navigateToAnimal(int animalId); // emitted when animal name is clicked
    void navigateToOwner(int ownerId);   // emitted when owner name is clicked

private slots:
    void onFiltersChanged();
    void onDateModeChanged();
    void onClearDateClicked();
    void onAddVaccineClicked();

private:
    void applyStyle();
    void loadVaccineTypeCombo();
    void loadStats();
    void loadTable();
    void appendRows(int offset);

    QWidget* makeBadge(const QString& text, const QString& bg, const QString& fg);
    QWidget* makeActionButtons(int vaccinationId);

    int  showAnimalPickerDialog();

    QString buildWhereClause() const;
    void    bindWhereParams(QSqlQuery& q) const;

    void showToast(const QString& message);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

    Ui::VaccinationsWidget *ui;
    bool m_rangeMode = true;

    PersianDatePicker* m_pickerFrom   = nullptr;
    PersianDatePicker* m_pickerTo     = nullptr;
    PersianDatePicker* m_pickerSingle = nullptr;

    int  m_offset = 0;
    static constexpr int kPageSize = 50;
    QPushButton* m_loadMoreBtn = nullptr;
};

#endif // VACCINATIONSWIDGET_H