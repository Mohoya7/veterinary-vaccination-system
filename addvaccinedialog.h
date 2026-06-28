#ifndef ADDVACCINEDIALOG_H
#define ADDVACCINEDIALOG_H

#include <QDialog>
#include <QMap>
#include "persiandatepicker.h"

namespace Ui { class AddVaccineDialog; }

class AddVaccineDialog : public QDialog
{
    Q_OBJECT

public:
    // Add mode
    explicit AddVaccineDialog(int animalId, QWidget *parent = nullptr);
    // Edit mode
    explicit AddVaccineDialog(int animalId, int vaccinationId, QWidget *parent = nullptr);
    ~AddVaccineDialog();

    // برای دکمه‌ی «+» تمدید سریع: در حالت Add، نوع واکسن و تعداد روز یادآوری
    // را از یک رکورد قبلی پیش‌پر می‌کند (قابل تغییر توسط کاربر، فقط پیش‌فرض است)
    void prefillFrom(int vaccineTypeId, int reminderDays);

private slots:
    void onVaccineTypeChanged(int index);
    void onSaveClicked();

private:
    void applyStyle();
    void loadVaccineTypes();
    void loadExistingData(int vaccinationId);

    Ui::AddVaccineDialog *ui;
    PersianDatePicker   *m_datePicker  = nullptr;
    int m_animalId      = -1;
    int m_vaccinationId = -1; // -1 = add mode
    int m_animalTypeId = -1;
    int m_oldVaccineTypeId = -1; // مقدار vaccine_type_id قبل از ویرایش (فقط در edit mode پر می‌شود)
    QMap<int, int> m_typeIdToDefaultDays;
};

#endif