#ifndef ABOUTTAB_H
#define ABOUTTAB_H

#include <QWidget>

// ─────────────────────────────────────────────────────────────────────────────
// AboutTab — Software info, version, and developer contact
// ─────────────────────────────────────────────────────────────────────────────

class AboutTab : public QWidget
{
    Q_OBJECT

public:
    explicit AboutTab(QWidget* parent = nullptr);

private:
    void applyStyle();
};

#endif // ABOUTTAB_H