#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QAction>
#include "database.h"

namespace Ui { class LoginDialog; }

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    QString role()     const { return m_role; }
    int     userId()   const { return m_userId; }
    QString username() const { return m_username; }

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onLoginClicked();
    void onTogglePasswordVisibility();

private:
    Ui::LoginDialog *ui;
    QString m_role;
    int     m_userId   = -1;
    QString m_username;
    QAction *m_togglePasswordAction = nullptr;
};

#endif // LOGINDIALOG_H