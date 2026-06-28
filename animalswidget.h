#ifndef ANIMALSWIDGET_H
#define ANIMALSWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QTimer>

namespace Ui { class AnimalsWidget; }

class AnimalsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnimalsWidget(QWidget *parent = nullptr);
    ~AnimalsWidget();

    void loadData();
    void showAnimalById(int animalId);

    // به‌جای loadData(): فیلتر/سرچ/offset/پروفایل انتخاب‌شده‌ی فعلی را
    // حفظ می‌کند و فقط دیتای تازه را دوباره می‌خواند. توسط MainWindow
    // وقتی صفحه «کثیف» تشخیص داده شود صدا زده می‌شود.
    void reloadPreservingState();

signals:
    void navigateToOwner(int ownerId);

private slots:
    void onTypeFilterChanged(int index);
    void onAnimalSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onAddAnimalClicked();
    void onEditAnimalClicked();
    void onDeleteAnimalClicked();
    void onViewVaccinationHistoryClicked();

private:
    void applyStyle();
    // typeFilter اکنون animal_type_id واقعی است؛ -1 یعنی «همه‌ی انواع»
    void loadAnimals(const QString& search = "", int typeFilter = -1);
    void loadTypeFilterCombo();   // پر کردن typeFilterCombo از جدول animal_types
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
    QTimer* m_searchDebounce   = nullptr; // debounce فیلد سرچ لیست (300ms)

    void appendAnimals(const QString& search, int typeFilter, int offset);
};

#endif