#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(const QString& role, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_role(role)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}