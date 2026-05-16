#ifndef ANIMALSWIDGET_H
#define ANIMALSWIDGET_H

#include <QDialog>

namespace Ui {
class Animalswidget;
}

class Animalswidget : public QDialog
{
    Q_OBJECT

public:
    explicit Animalswidget(QWidget *parent = nullptr);
    ~Animalswidget();

private:
    Ui::Animalswidget *ui;
};

#endif // ANIMALSWIDGET_H
