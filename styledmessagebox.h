#ifndef STYLEDMESSAGEBOX_H
#define STYLEDMESSAGEBOX_H

#include <QDialog>
#include <QString>

class StyledMessageBox : public QDialog
{
    Q_OBJECT

public:
    enum Type { Success, Question, Warning, Error };

    static bool question(QWidget* parent, const QString& title, const QString& message);
    static void warning(QWidget* parent,  const QString& title, const QString& message);
    static void success(QWidget* parent,  const QString& title, const QString& message);
    static void error(QWidget* parent,    const QString& title, const QString& message);

private:
    explicit StyledMessageBox(Type type, const QString& title,
                              const QString& message, QWidget* parent = nullptr);
    bool m_accepted = false;

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif