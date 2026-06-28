#ifndef ADDANIMALDIALOG_H
#define ADDANIMALDIALOG_H

#include <QDialog>
#include "persiandatepicker.h"

namespace Ui { class AddAnimalDialog; }

class AddAnimalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddAnimalDialog(int ownerId, const QString& ownerPhone, QWidget *parent = nullptr);
    explicit AddAnimalDialog(int ownerId, const QString& ownerPhone, int animalId, QWidget *parent = nullptr);
    ~AddAnimalDialog();

    int     savedAnimalId()   const { return m_savedAnimalId; }
    QString savedFileNumber() const { return m_savedFileNumber; }

private slots:
    void onSaveClicked();

private:
    void applyStyle();
    void loadAnimalData(int animalId);
    void loadTypeCombo();

    Ui::AddAnimalDialog  *ui;
    PersianDatePicker    *m_birthPicker = nullptr;
    int     m_ownerId        = -1;
    int     m_animalId       = -1;
    int     m_savedAnimalId  = -1;
    QString m_savedFileNumber;
    int     m_originalAnimalTypeId = -1; // برای تشخیص تغییر نوع حیوان در حالت ویرایش
};

#endif