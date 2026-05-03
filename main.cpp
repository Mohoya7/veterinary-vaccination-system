#include <QApplication>
#include <QMessageBox>
#include "database.h"
#include "logindialog.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!Database::instance().connect("localhost", "veterinary", "root", "1234")) {
        QMessageBox::critical(nullptr, "خطا", "اتصال به دیتابیس برقرار نشد.");
        return -1;
    }

    LoginDialog login;
    if (login.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow w(login.role());
    w.show();

    return a.exec();
}