#ifndef ADDVACCINEDIALOG_H
#define ADDVACCINEDIALOG_H

#include <QDialog>
#include <QMap>

namespace Ui { class AddVaccineDialog; }

class AddVaccineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddVaccineDialog(int animalId, QWidget *parent = nullptr);
    ~AddVaccineDialog();

private slots:
    void onVaccineTypeChanged(int index);
    void onSaveClicked();

private:
    void applyStyle();
    void loadVaccineTypes();

    Ui::AddVaccineDialog *ui;
    int m_animalId  = -1;
    QString m_animalType;
    QMap<int, int> m_typeIdToDefaultDays; // vaccineType.id → default_reminder_days
};

#endif