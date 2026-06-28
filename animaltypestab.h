#ifndef ANIMALTYPESTAB_H
#define ANIMALTYPESTAB_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>

// ─────────────────────────────────────────────────────────────────────────────
// AnimalTypesTab — Manage animal types and their associated vaccine types
//
// Left panel: list of animal types
// Right panel: detail view with edit + associated vaccines
//
// Admin: can add/edit animal types, add/edit vaccines, delete both
// Technician: read-only view
// ─────────────────────────────────────────────────────────────────────────────

class AnimalTypesTab : public QWidget
{
    Q_OBJECT

public:
    explicit AnimalTypesTab(QWidget* parent = nullptr);

    // به‌جای صدا زدن مستقیم loadAnimalTypes(): انتخاب فعلی (نوع حیوان +
    // نوع واکسن) را هم تا حد امکان حفظ می‌کند
    void reloadPreservingState();

private slots:
    void onAnimalTypeSelected(int row);
    void onAddAnimalType();
    void onEditAnimalType();
    void onDeleteAnimalType();
    void onAddVaccineType();
    void onEditVaccineType();
    void onDeleteVaccineType();

private:
    void applyStyle();
    void loadAnimalTypes();
    void loadVaccineTypes(int animalTypeId);
    void clearDetail();

    QListWidget* m_animalTypeList = nullptr;
    QPushButton* m_btnAddType     = nullptr;
    QPushButton* m_btnEditType    = nullptr;
    QPushButton* m_btnDeleteType  = nullptr;

    QListWidget* m_vaccineList      = nullptr;
    QPushButton* m_btnAddVaccine    = nullptr;
    QPushButton* m_btnEditVaccine   = nullptr;
    QPushButton* m_btnDeleteVaccine = nullptr;

    QWidget* m_detailPanel = nullptr;

    int m_selectedAnimalTypeId  = -1;
    int m_selectedVaccineTypeId = -1;
};

#endif // ANIMALTYPESTAB_H