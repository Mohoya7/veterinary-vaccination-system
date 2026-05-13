#ifndef VACCINATIONSWIDGET_H
#define VACCINATIONSWIDGET_H

#include <QWidget>
#include <QDate>

namespace Ui { class VaccinationsWidget; }

class VaccinationsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VaccinationsWidget(QWidget *parent = nullptr);
    ~VaccinationsWidget();
    void loadData();

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

    QWidget* makeBadge(const QString& text, const QString& bg, const QString& fg);
    QWidget* makeActionButtons(int vaccinationId);

    // Animal-picker dialog
    int  showAnimalPickerDialog();

    Ui::VaccinationsWidget *ui;
    bool m_rangeMode = true;
};

#endif // VACCINATIONSWIDGET_H