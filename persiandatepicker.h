#ifndef PERSIANDATEPICKER_H
#define PERSIANDATEPICKER_H

#include <QWidget>
#include <QDate>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QEvent>

class PersianDatePicker : public QWidget
{
    Q_OBJECT

public:
    explicit PersianDatePicker(QWidget* parent = nullptr);

    void  setDate(const QDate& gregorianDate);
    QDate date() const;
    void  setReadOnly(bool ro);

signals:
    void dateChanged(const QDate& gregorianDate);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void openDialog();
    void updateField();

    bool isLeapYear(int jy) const;
    int  daysInMonth(int jy, int jm) const;
    QString farsi(int n, int w = 0) const;

    int  m_jy = 1404, m_jm = 1, m_jd = 1;
    bool m_readOnly = false;

    QLineEdit* m_field = nullptr;

    static const QStringList kMonths;
    static const int kMinY = 1400;
    static const int kMaxY = 1420;
};

#endif