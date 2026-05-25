#ifndef ANIMALSWIDGET_H
#define ANIMALSWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QPushButton>

namespace Ui { class AnimalsWidget; }

class AnimalsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnimalsWidget(QWidget *parent = nullptr);
    ~AnimalsWidget();

    void loadData();
    void showAnimalById(int animalId);

signals:
    void navigateToOwner(int ownerId);
    void navigateToVaccination(int animalId);

private slots:
    void onSearchChanged(const QString& text);
    void onTypeFilterChanged(int index);
    void onAnimalSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onAddAnimalClicked();
    void onEditAnimalClicked();
    void onDeleteAnimalClicked();
    void onViewVaccinationHistoryClicked();

private:
    void applyStyle();
    void loadAnimals(const QString& search = "", int typeFilter = 0);
    void showAnimalProfile(int animalId);
    void clearProfile();
    void loadVaccinationHistory(int animalId);
    void addInfoRow(QWidget* container, const QString& label, const QString& value);
    void clearInfoRows();

    Ui::AnimalsWidget *ui;
    int m_selectedAnimalId = -1;
    int m_selectedOwnerId  = -1;
    int m_currentOffset = 0;
    int m_pageSize      = 50;
    bool m_hasMore      = false;
    QPushButton* m_loadMoreBtn = nullptr;

    void appendAnimals(const QString& search, int typeFilter, int offset);
};

#endif