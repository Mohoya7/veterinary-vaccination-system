#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dashboardwidget.h"
#include "ownerswidget.h"
#include "vaccinationswidget.h"
#include "reminderswidget.h"
#include "animalswidget.h"
#include "userstab.h"
#include "animaltypestab.h"
#include "backuptab.h"
#include "abouttab.h"
#include "session.h"

#include <QPainter>
#include <QIcon>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QVBoxLayout>

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

    auto* textured = new TexturedWidget(this);
    textured->setLayout(ui->centralWidget->layout());
    ui->centralWidget->setLayout(nullptr);
    setCentralWidget(textured);

    // ── SVG icon helper ──────────────────────────────────────────────────────
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

    // ── Settings sub-buttons — injected into sidebar after btnSettings ───────
    // Find the navLayout inside sidebarWidget and insert a sub-widget after btnSettings
    m_settingsSubWidget = new QWidget(ui->sidebarWidget);
    m_settingsSubWidget->setObjectName("settingsSubWidget");
    m_settingsSubWidget->setMaximumHeight(0); // collapsed by default
    m_settingsSubWidget->setVisible(true);

    auto* subLay = new QVBoxLayout(m_settingsSubWidget);
    subLay->setContentsMargins(20, 2, 10, 4); // indent to show hierarchy
    subLay->setSpacing(2);

    auto makeSubBtn = [&](const QString& text) -> QPushButton* {
        auto* btn = new QPushButton(text, m_settingsSubWidget);
        btn->setObjectName("settingsSubBtn");
        btn->setCheckable(true);
        btn->setMinimumHeight(36);
        subLay->addWidget(btn);
        return btn;
    };

    m_btnSubAnimalTypes = makeSubBtn("حیوانات و واکسن‌ها");
    m_btnSubUsers       = makeSubBtn("مدیریت کاربران");

    // Backup sub-button — admin only
    if (Session::instance().isAdmin()) {
        m_btnSubBackup = makeSubBtn("بکاپ");
    }

    m_btnSubAbout = makeSubBtn("درباره");

    // Insert sub-widget into the sidebar's navLayout after btnSettings
    // navLayout is inside sidebarWidget — we find it and insert
    auto* navLayout = qobject_cast<QVBoxLayout*>(
        ui->sidebarWidget->findChild<QVBoxLayout*>("navLayout"));
    if (navLayout) {
        // Find btnSettings index and insert sub-widget right after it
        for (int i = 0; i < navLayout->count(); ++i) {
            if (navLayout->itemAt(i)->widget() == ui->btnSettings) {
                navLayout->insertWidget(i + 1, m_settingsSubWidget);
                break;
            }
        }
    }

    // ── Stylesheet ───────────────────────────────────────────────────────────
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
        /* Settings sub-buttons — slightly smaller and indented */
        QPushButton#settingsSubBtn {
            background-color: transparent;
            color: rgba(255,255,255,0.65);
            border: none; border-radius: 6px;
            font-size: 12px; padding: 6px 10px;
            text-align: left;
        }
        QPushButton#settingsSubBtn:hover {
            background-color: rgba(255,255,255,0.08); color: rgba(255,255,255,0.9);
        }
        QPushButton#settingsSubBtn:checked {
            background-color: rgba(255,255,255,0.15);
            color: white; font-weight: bold;
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

    // ── Checkable nav buttons ────────────────────────────────────────────────
    ui->btnDashboard->setCheckable(true);
    ui->btnAnimals->setCheckable(true);
    ui->btnOwners->setCheckable(true);
    ui->btnReminders->setCheckable(true);
    ui->btnVaccinations->setCheckable(true);
    ui->btnSettings->setCheckable(true);
    ui->btnDashboard->setChecked(true);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(ui->btnDashboard,     &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnAnimals,       &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnOwners,        &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnReminders,     &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnVaccinations,  &QPushButton::clicked, this, &MainWindow::onNavButtonClicked);
    connect(ui->btnSettings,      &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(ui->btnLogout,        &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    connect(ui->btnToggleSidebar, &QPushButton::clicked, this, &MainWindow::onToggleSidebar);

    // Settings sub-button connections
    connect(m_btnSubUsers, &QPushButton::clicked, this, [this]() {
        uncheckAllButtons();
        uncheckAllSubButtons();
        ui->btnSettings->setChecked(true);
        m_btnSubUsers->setChecked(true);
        ui->contentStack->setCurrentIndex(kIdxUsers);
    });
    connect(m_btnSubAnimalTypes, &QPushButton::clicked, this, [this]() {
        uncheckAllButtons();
        uncheckAllSubButtons();
        ui->btnSettings->setChecked(true);
        m_btnSubAnimalTypes->setChecked(true);
        ui->contentStack->setCurrentIndex(kIdxAnimalTypes);
    });
    if (m_btnSubBackup) {
        connect(m_btnSubBackup, &QPushButton::clicked, this, [this]() {
            uncheckAllButtons();
            uncheckAllSubButtons();
            ui->btnSettings->setChecked(true);
            m_btnSubBackup->setChecked(true);
            ui->contentStack->setCurrentIndex(kIdxBackup);
        });
    }
    connect(m_btnSubAbout, &QPushButton::clicked, this, [this]() {
        uncheckAllButtons();
        uncheckAllSubButtons();
        ui->btnSettings->setChecked(true);
        m_btnSubAbout->setChecked(true);
        ui->contentStack->setCurrentIndex(kIdxAbout);
    });

    // ── Sidebar animation ────────────────────────────────────────────────────
    m_sidebarAnim = new QPropertyAnimation(ui->sidebarWidget, "maximumWidth", this);
    m_sidebarAnim->setDuration(kAnimDuration);
    m_sidebarAnim->setEasingCurve(QEasingCurve::InOutCubic);

    updateToggleIcon();

    // ── Content stack — clear .ui placeholders and add real widgets ──────────
    while (ui->contentStack->count() > 0)
        ui->contentStack->removeWidget(ui->contentStack->widget(0));

    m_dashboard = new DashboardWidget(this);
    ui->contentStack->addWidget(m_dashboard);         // index 0

    m_animals = new AnimalsWidget(this);
    ui->contentStack->addWidget(m_animals);           // index 1

    m_owners = new OwnersWidget(this);
    ui->contentStack->addWidget(m_owners);            // index 2

    m_reminders = new RemindersWidget(this);
    ui->contentStack->addWidget(m_reminders);         // index 3

    m_vaccinations = new VaccinationsWidget(this);
    ui->contentStack->addWidget(m_vaccinations);      // index 4

    m_usersTab = new UsersTab(this);
    ui->contentStack->addWidget(m_usersTab);          // index 5

    m_animalTypes = new AnimalTypesTab(this);
    ui->contentStack->addWidget(m_animalTypes);       // index 6

    m_backup = new BackupTab(this);
    ui->contentStack->addWidget(m_backup);            // index 7

    m_about = new AboutTab(this);
    ui->contentStack->addWidget(m_about);             // index 8

    ui->contentStack->setCurrentIndex(0);

    // ── Cross-widget navigation ──────────────────────────────────────────────
    connect(m_animals, &AnimalsWidget::navigateToOwner, this, [this](int ownerId) {
        ui->btnOwners->click();
        m_owners->showOwnerById(ownerId);
    });
    connect(m_owners, &OwnersWidget::navigateToAnimal, this, [this](int animalId) {
        ui->btnAnimals->click();
        m_animals->showAnimalById(animalId);
    });

    showMaximized();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ── Settings expand/collapse ──────────────────────────────────────────────────

void MainWindow::onSettingsClicked()
{
    uncheckAllButtons();
    ui->btnSettings->setChecked(true);

    bool expand = !m_settingsExpanded;
    setSettingsExpanded(expand);

    // If collapsing, go to first sub-page by default
    if (expand) {
        uncheckAllSubButtons();
        m_btnSubUsers->setChecked(true);
        ui->contentStack->setCurrentIndex(kIdxUsers);
    }
}

void MainWindow::setSettingsExpanded(bool expanded)
{
    m_settingsExpanded = expanded;

    // Animate sub-widget height
    auto* anim = new QPropertyAnimation(m_settingsSubWidget, "maximumHeight", this);
    anim->setDuration(180);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    anim->setStartValue(m_settingsSubWidget->maximumHeight());

    // Calculate target height based on number of sub-buttons
    int btnCount = m_settingsSubWidget->layout()->count();
    int targetH  = expanded ? (btnCount * 40 + 8) : 0;

    anim->setEndValue(targetH);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// ── uncheck helpers ───────────────────────────────────────────────────────────

void MainWindow::uncheckAllButtons()
{
    ui->btnDashboard->setChecked(false);
    ui->btnAnimals->setChecked(false);
    ui->btnOwners->setChecked(false);
    ui->btnReminders->setChecked(false);
    ui->btnVaccinations->setChecked(false);
    ui->btnSettings->setChecked(false);
}

void MainWindow::uncheckAllSubButtons()
{
    m_btnSubUsers->setChecked(false);
    m_btnSubAnimalTypes->setChecked(false);
    if (m_btnSubBackup) m_btnSubBackup->setChecked(false);
    m_btnSubAbout->setChecked(false);
}

// ── Nav button clicked ────────────────────────────────────────────────────────

void MainWindow::onNavButtonClicked()
{
    uncheckAllButtons();
    uncheckAllSubButtons();

    // Collapse settings sub-menu if navigating away
    if (m_settingsExpanded) {
        setSettingsExpanded(false);
    }

    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) btn->setChecked(true);

    if      (sender() == ui->btnDashboard)    ui->contentStack->setCurrentIndex(kIdxDashboard);
    else if (sender() == ui->btnAnimals)      ui->contentStack->setCurrentIndex(kIdxAnimals);
    else if (sender() == ui->btnOwners)       ui->contentStack->setCurrentIndex(kIdxOwners);
    else if (sender() == ui->btnReminders)    ui->contentStack->setCurrentIndex(kIdxReminders);
    else if (sender() == ui->btnVaccinations) ui->contentStack->setCurrentIndex(kIdxVaccinations);
}

// ── Toggle sidebar ────────────────────────────────────────────────────────────

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

void MainWindow::onLogoutClicked()
{
    close();
}