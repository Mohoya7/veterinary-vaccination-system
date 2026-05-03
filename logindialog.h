#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "database.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    QString role() const { return m_role; }

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onLoginClicked();

private:
    Ui::LoginDialog *ui;
    QString m_role;
};

#endif // LOGINDIALOG_H