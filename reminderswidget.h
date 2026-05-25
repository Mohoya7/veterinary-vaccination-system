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

class QSqlQuery;

namespace Ui { class RemindersWidget; }

class RemindersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RemindersWidget(QWidget *parent = nullptr);
    ~RemindersWidget();
    void loadData();

private slots:
    void onFiltersChanged();
    void onLoadMoreClicked();
    void onExportPdfClicked();
    void onFollowUpChanged(int rfId, bool checked);
    void onOwnerResponseChanged(int rfId, int responseIndex);

private:
    void applyStyle();
    void loadStats();
    void loadTable();           // reset + load first page
    void appendRows(int offset); // append next page into existing table

    // helpers
    QString buildWhereClause() const;
    void    bindWhereParams(QSqlQuery& q) const;

    QWidget* makeBadge(const QString& text, const QString& bg, const QString& fg);
    QWidget* makeResponseCombo(int rfId, int currentResponse);

    Ui::RemindersWidget *ui;

    // pagination
    int  m_offset      = 0;
    static constexpr int kPageSize = 100;

    QPushButton* m_loadMoreBtn = nullptr;
};

#endif