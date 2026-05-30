#ifndef ADDVACCINEDIALOG_H
#define ADDVACCINEDIALOG_H

#include <QDialog>
#include <QMap>

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

private slots:
    void onVaccineTypeChanged(int index);
    void onSaveClicked();

private:
    void applyStyle();
    void loadVaccineTypes();
    void loadExistingData(int vaccinationId);

    Ui::AddVaccineDialog *ui;
    int m_animalId      = -1;
    int m_vaccinationId = -1; // -1 = add mode
    int m_animalTypeId = -1;
    QMap<int, int> m_typeIdToDefaultDays;
};

#endif