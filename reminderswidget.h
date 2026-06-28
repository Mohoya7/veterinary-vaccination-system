#ifndef REMINDERSWIDGET_H
#define REMINDERSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QDate>
#include <QEvent>
#include <QTimer>
#include "persiandatepicker.h"

class QSqlQuery;

namespace Ui { class RemindersWidget; }

class RemindersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RemindersWidget(QWidget *parent = nullptr);
    ~RemindersWidget();
    void loadData();

    // به‌جای loadData(): فیلتر/سرچ/offset فعلی را حفظ می‌کند + کامبوهای
    // نوع حیوان/نوع واکسن را هم از DB دوباره می‌خواند (با حفظ انتخاب فعلی)
    void reloadPreservingState();

signals:
    void navigateToAnimal(int animalId); // emitted when animal name is clicked
    void navigateToOwner(int ownerId);   // emitted when owner name is clicked

private slots:
    void onFiltersChanged();
    void onLoadMoreClicked();
    void onExportPdfClicked();
    void onFollowUpChanged(int rfId, bool checked);
    void onOwnerResponseChanged(int rfId, int responseIndex);

private:
    void applyStyle();
    void loadStats();
    void loadTable();
    void appendRows(int offset);
    void loadAnimalTypeCombo();   // پر کردن animalTypeCombo از جدول animal_types
    void loadVaccineTypeCombo();
    void buildFilterDateWidget();

    QString buildWhereClause() const;
    void    bindWhereParams(QSqlQuery& q) const;

    QWidget* makeBadge(const QString& text, const QString& bg, const QString& fg);
    void showToast(const QString& message);
    QWidget* makeResponseCombo(int rfId, int currentResponse);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

    Ui::RemindersWidget *ui;

    QWidget*           m_filterDateWidget  = nullptr;
    QComboBox*         m_subModeCombo      = nullptr;
    QSpinBox*          m_daysSpin          = nullptr;
    QPushButton*       m_dirBtn            = nullptr;
    PersianDatePicker* m_pickerFrom        = nullptr;
    PersianDatePicker* m_pickerTo          = nullptr;

    bool m_subManualMode = true;

    int  m_offset = 0;
    static constexpr int kPageSize = 100;
    QPushButton* m_loadMoreBtn = nullptr;
    QTimer*      m_searchDebounce = nullptr; // debounce فیلد سرچ بالای صفحه (300ms)
};

#endif