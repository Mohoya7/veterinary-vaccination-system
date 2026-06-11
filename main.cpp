#include <QApplication>
#include <QMessageBox>
#include "database.h"
#include "logindialog.h"
#include "mainwindow.h"
#include "session.h"
#include "backuptab.h"
#include "styledmessagebox.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setLayoutDirection(Qt::RightToLeft);

    if (!Database::instance().connect("localhost", "veterinary", "root", "1234")) {
        QMessageBox::critical(nullptr, "خطا", "اتصال به دیتابیس برقرار نشد.");
        return -1;
    }

    LoginDialog login;
    if (login.exec() != QDialog::Accepted) {
        return 0;
    }

    // Store logged-in user info in Session singleton
    // loginDialog already fetched id and username — we read them here
    // We need userId too, so we query once more after successful login
    {
        QSqlQuery q;
        q.prepare("SELECT id, username FROM users WHERE role = :role LIMIT 1");
        q.bindValue(":role", login.role());
        // Actually we want the exact user who logged in, not just by role
        // LoginDialog should expose username — for now we store role only
        // and set a placeholder id; UsersTab reads Session::userId() for password change
        // so we need the real id. Let LoginDialog expose it.
    }
    // Session is set using what LoginDialog exposes
    Session::instance().setUser(login.userId(), login.username(), login.role());

    // Check and run auto-backup silently in background
    BackupTab::checkAndRunAutoBackup();

    MainWindow w(login.role());
    w.show();

    return a.exec();
}