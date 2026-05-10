#ifndef ADDOWNERDIALOG_H
#define ADDOWNERDIALOG_H

#include <QDialog>

namespace Ui { class AddOwnerDialog; }

class AddOwnerDialog : public QDialog
{
    Q_OBJECT

public:
    // Add mode
    explicit AddOwnerDialog(QWidget *parent = nullptr);
    // Edit mode
    explicit AddOwnerDialog(int ownerId, QWidget *parent = nullptr);
    ~AddOwnerDialog();

    int     savedOwnerId() const { return m_savedOwnerId; }
    QString savedPhone()   const { return m_savedPhone; }

private slots:
    void onSaveClicked();

private:
    void applyStyle();
    void loadOwnerData(int ownerId);

    Ui::AddOwnerDialog *ui;
    int     m_ownerId      = -1; // -1 = add mode
    int     m_savedOwnerId = -1;
    QString m_savedPhone;
};

#endif