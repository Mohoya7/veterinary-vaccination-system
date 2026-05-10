#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dashboardwidget.h"
#include "ownerswidget.h"
#include <QPainter>
#include <QIcon>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPropertyAnimation>
#include <QEasingCurve>

class TexturedWidget : public QWidget {
public:
    explicit TexturedWidget(QWidget* parent = nullptr) : QWidget(parent) {}
protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor("#F1F8E9"));

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(139, 195, 74, 35));
        painter.drawEllipse(30, height() - 240, 220, 220);
        painter.setBrush(QColor(249, 168, 37, 25));
        painter.drawEllipse(width() - 260, height() - 200, 200, 200);
        painter.setBrush(QColor(139, 195, 74, 28));
        painter.drawEllipse(width() / 2 - 90, height() - 160, 170, 170);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(100, 180, 80, 55), 1.5));
        painter.drawEllipse(width() - 220, height() / 2 - 20, 200, 200);
        painter.setPen(QPen(QColor(100, 180, 80, 45), 1.5));
        painter.drawEllipse(40, height() / 2 - 100, 180, 180);
        painter.setPen(QPen(QColor(249, 168, 37, 40), 1.5));
        painter.drawEllipse(width() / 2 + 120, height() / 2 - 60, 160, 160);
        painter.setPen(QPen(QColor(100, 180, 80, 35), 1.5));
        painter.drawEllipse(width() / 2 - 280, height() / 2 + 60, 150, 150);
        painter.setPen(QPen(QColor(249, 168, 37, 30), 1.5));
        painter.drawEllipse(width() - 120, height() - 380, 140, 140);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(100, 170, 80, 100));
        const int spacing = 28;
        const int radius  = 1;
        for (int x = 0; x < width(); x += spacing)
            for (int y = 0; y < height(); y += spacing)
                painter.drawEllipse(QPointF(x, y), radius, radius);

        QWidget::paintEvent(event);
    }
};

MainWindow::MainWindow(const QString& role, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_role(role)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    showMaximized();

    auto* textured = new TexturedWidget(this);
    textured->setLayout(ui->centralWidget->layout());
    ui->centralWidget->setLayout(nullptr);
    setCentralWidget(textured);

    auto setIcon = [](QPushButton* btn, const QString& path) {
        QSvgRenderer renderer(path);
        QPixmap pixmap(20, 20);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);

        QPixmap tinted(pixmap.size());
        tinted.fill(Qt::transparent);
        QPainter p(&tinted);
        p.drawPixmap(0, 0, pixmap);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(tinted.rect(), QColor(255, 255, 255, 200));
        p.end();

        btn->setIcon(QIcon(tinted));
        btn->setIconSize(QSize(20, 20));
        btn->setLayoutDirection(Qt::RightToLeft);
    };

    setIcon(ui->btnDashboard,    ":/icons/grid.svg");
    setIcon(ui->btnAnimals,      ":/icons/dog.svg");
    setIcon(ui->btnOwners,       ":/icons/users.svg");
    setIcon(ui->btnReminders,    ":/icons/bell.svg");
    setIcon(ui->btnVaccinations, ":/icons/syringe.svg");
    setIcon(ui->btnSettings,     ":/icons/settings.svg");
    setIcon(ui->btnLogout,       ":/icons/log-out.svg");

    {
        QSvgRenderer renderer(QString(":/icons/dog.svg"));
        QPixmap pixmap(28, 28);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);
        QPixmap tinted(pixmap.size());
        tinted.fill(Qt::transparent);
        QPainter p(&tinted);
        p.drawPixmap(0, 0, pixmap);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(tinted.rect(), QColor(255, 255, 255, 220));
        p.end();
        ui->sidebarLogoLabel->setPixmap(tinted);
    }

    this->setStyleSheet(R"(
        QWidget { background-color: transparent; }
        QWidget#toggleStrip { background-color: #1B5E20; }
        QPushButton#btnToggleSidebar {
            background-color: rgba(255,255,255,0.08);
            border: none; border-radius: 4px; color: white;
        }
        QPushButton#btnToggleSidebar:hover { background-color: rgba(255,255,255,0.18); }
        QPushButton#btnToggleSidebar:pressed { background-color: rgba(255,255,255,0.05); }
        QFrame#sidebarDivider, QFrame#logoutDivider { color: rgba(255,255,255,0.12); }
        QLabel#sidebarTitleLabel {
            color: white; font-size: 14px; font-weight: bold; background: transparent;
        }
        QLabel#sidebarSubtitleLabel {
            color: rgba(255,255,255,0.55); font-size: 11px; background: transparent;
        }
        QWidget#sidebarWidget QPushButton {
            background-color: transparent;
            color: rgba(255,255,255,0.8);
            border: none; border-radius: 8px;
            font-size: 13px; padding: 8px 12px;
            icon-size: 20px; text-align: left;
        }
        QWidget#sidebarWidget QPushButton:hover {
            background-color: rgba(255,255,255,0.1); color: white;
        }
        QWidget#sidebarWidget QPushButton:checked {
            background-color: rgba(255,255,255,0.18); color: white; font-weight: bold;
        }
        QWidget#sidebarWidget #btnLogout { color: rgba(255,100,100,0.85); }
        QWidget#sidebarWidget #btnLogout:hover {
            background-color: rgba(255,100,100,0.12); color: rgba(255,120,120,1);
        }
        QStackedWidget { background-color: transparent; }
        QWidget#animalCard, QWidget#ownerCard, QWidget#tableCard {
            background-color: white; border: 1px solid #C8E6C9; border-radius: 10px;
        }
        QWidget#reminderCard {
            background-color: white; border: 1px solid #F9A825; border-radius: 10px;
        }
        QScrollArea QScrollBar:vertical {
            background: transparent; width: 6px; margin: 0px; border-radius: 3px;
        }
        QScrollArea QScrollBar::handle:vertical {
            background: rgba(100,170,80,150); border-radius: 3px; min-height: 30px;
        }
        QScrollArea QScrollBar::handle:vertical:hover { background: rgba(100,170,80,220); }
        QScrollArea QScrollBar::add-line:vertical,
        QScrollArea QScrollBar::sub-line:vertical { height: 0px; }
        QScrollArea QScrollBar::add-page:vertical,
        QScrollArea QScrollBar::sub-page:vertical { background: transparent; }
    )");

    ui->btnDashboard->setCheckable(true);
    ui->btnAnimals->setCheckable(true);
    ui->btnOwners->setCheckable(true);
    ui->btnReminders->setCheckable(true);
    ui->btnVaccinations->setCheckable(true);
    ui->btnSettings->setCheckable(true);
    ui->btnDashboard->setChecked(true);

    connect(ui->btnDashboard,     &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnAnimals,       &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnOwners,        &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnReminders,     &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnVaccinations,  &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnSettings,      &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnLogout,        &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    connect(ui->btnToggleSidebar, &QPushButton::clicked, this, &MainWindow::onToggleSidebar);

    m_sidebarAnim = new QPropertyAnimation(ui->sidebarWidget, "maximumWidth", this);
    m_sidebarAnim->setDuration(kAnimDuration);
    m_sidebarAnim->setEasingCurve(QEasingCurve::InOutCubic);

    updateToggleIcon();

    // ── Setup content stack pages ──────────────────────────────────────────
    // Clear any pages already added via .ui file
    while (ui->contentStack->count() > 0)
        ui->contentStack->removeWidget(ui->contentStack->widget(0));

    m_dashboard = new DashboardWidget(this);
    ui->contentStack->addWidget(m_dashboard);        // index 0 - Dashboard

    ui->contentStack->addWidget(new QWidget(this));  // index 1 - Animals placeholder

    m_owners = new OwnersWidget(this);
    ui->contentStack->addWidget(m_owners);            // index 2 - Owners

    ui->contentStack->addWidget(new QWidget(this));  // index 3 - Reminders placeholder
    ui->contentStack->addWidget(new QWidget(this));  // index 4 - Vaccinations placeholder
    ui->contentStack->addWidget(new QWidget(this));  // index 5 - Settings placeholder

    ui->contentStack->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onToggleSidebar()
{
    if (m_sidebarAnim->state() == QAbstractAnimation::Running)
        m_sidebarAnim->stop();

    int startVal = ui->sidebarWidget->width();
    int endVal;

    if (m_sidebarExpanded) {
        endVal = kSidebarCollapsed;
        m_sidebarExpanded = false;
    } else {
        ui->sidebarWidget->setMaximumWidth(kSidebarExpanded);
        startVal = ui->sidebarWidget->width();
        endVal = kSidebarExpanded;
        m_sidebarExpanded = true;
    }

    m_sidebarAnim->setStartValue(startVal);
    m_sidebarAnim->setEndValue(endVal);
    m_sidebarAnim->start();

    updateToggleIcon();
}

void MainWindow::updateToggleIcon()
{
    // Draw hamburger icon (3 lines)
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(255, 255, 255, 200), 1.5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(2, 4,  14, 4);
    p.drawLine(2, 8,  14, 8);
    p.drawLine(2, 12, 14, 12);
    p.end();

    ui->btnToggleSidebar->setIcon(QIcon(pixmap));
    ui->btnToggleSidebar->setIconSize(QSize(16, 16));
}

void MainWindow::uncheckAllButtons()
{
    ui->btnDashboard->setChecked(false);
    ui->btnAnimals->setChecked(false);
    ui->btnOwners->setChecked(false);
    ui->btnReminders->setChecked(false);
    ui->btnVaccinations->setChecked(false);
    ui->btnSettings->setChecked(false);
}

void MainWindow::onNavButtonClicked()
{
    uncheckAllButtons();
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) btn->setChecked(true);

    if      (sender() == ui->btnDashboard)    ui->contentStack->setCurrentIndex(0);
    else if (sender() == ui->btnAnimals)      ui->contentStack->setCurrentIndex(1);
    else if (sender() == ui->btnOwners)       ui->contentStack->setCurrentIndex(2);
    else if (sender() == ui->btnReminders)    ui->contentStack->setCurrentIndex(3);
    else if (sender() == ui->btnVaccinations) ui->contentStack->setCurrentIndex(4);
    else if (sender() == ui->btnSettings)     ui->contentStack->setCurrentIndex(5);
}

void MainWindow::onLogoutClicked()
{
    close();
}