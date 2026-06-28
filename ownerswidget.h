#ifndef OWNERSWIDGET_H
#define OWNERSWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QTimer>

namespace Ui { class OwnersWidget; }

class OwnersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OwnersWidget(QWidget *parent = nullptr);
    ~OwnersWidget();
    void loadData();
    void showOwnerById(int ownerId);

    // به‌جای loadData(): فیلتر/سرچ/offset/پروفایل فعلی را حفظ می‌کند
    void reloadPreservingState();

signals:
    void navigateToAnimal(int animalId);

private slots:
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
    int m_currentOffset  = 0;
    static constexpr int m_pageSize = 50;
    bool m_hasMore       = false;
    QPushButton* m_loadMoreBtn = nullptr;
    QTimer* m_searchDebounce   = nullptr; // debounce فیلد سرچ لیست (300ms)

    void appendOwners(const QString& filter, int offset);
};

#endif