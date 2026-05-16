#include "animalswidget.h"
#include "ui_animalswidget.h"

Animalswidget::Animalswidget(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Animalswidget)
{
    ui->setupUi(this);
}

Animalswidget::~Animalswidget()
{
    delete ui;
}
