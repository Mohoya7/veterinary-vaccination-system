#ifndef ADDANIMALDIALOG_H
#define ADDANIMALDIALOG_H

#include <QDialog>

namespace Ui { class AddAnimalDialog; }

class AddAnimalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddAnimalDialog(int ownerId, const QString& ownerPhone, QWidget *parent = nullptr);
    ~AddAnimalDialog();

    int savedAnimalId() const { return m_savedAnimalId; }

private slots:
    void onSaveClicked();

private:
    void applyStyle();

    Ui::AddAnimalDialog *ui;
    int m_ownerId      = -1;
    int m_savedAnimalId = -1;
};

#endif