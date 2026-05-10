#ifndef OWNERSWIDGET_H
#define OWNERSWIDGET_H

#include <QWidget>
#include <QListWidgetItem>

namespace Ui { class OwnersWidget; }

class OwnersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OwnersWidget(QWidget *parent = nullptr);
    ~OwnersWidget();
    void loadData();

signals:
    void navigateToAnimal(int animalId);

private slots:
    void onSearchChanged(const QString& text);
    void onOwnerSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onAddOwnerClicked();
    void onEditOwnerClicked();
    void onDeleteOwnerClicked();

private:
    void loadOwners(const QString& filter = "");
    void showOwnerProfile(int ownerId);
    void clearProfile();
    void addContactRow(const QString& label, const QString& value);
    void loadAnimals(int ownerId);
    void clearContactRows();
    void clearAnimals();

    Ui::OwnersWidget *ui;
    int m_selectedOwnerId = -1;
};

#endif