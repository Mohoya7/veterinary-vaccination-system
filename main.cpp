#include "mainwindow.h"
#include <QApplication>
#include "database.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!Database::instance().connect("localhost", "veterinary", "root", "پسورد MySQL ات")) {
        qDebug() << "Failed to connect to database!";
        return -1;
    }

    MainWindow w;
    w.show();

    return a.exec();
}