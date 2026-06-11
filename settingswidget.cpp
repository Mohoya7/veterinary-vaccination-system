#include "settingswidget.h"
#include "userstab.h"
#include "animaltypestab.h"
#include "backuptab.h"
#include "abouttab.h"
#include "session.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

SettingsWidget::SettingsWidget(QWidget* parent)
    : QWidget(parent)
{
    setLayoutDirection(Qt::RightToLeft);

    auto* rootLay = new QHBoxLayout(this);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    // ── Left sidebar ────────────────────────────────────────────────────────
    auto* sidebar = new QWidget;
    sidebar->setObjectName("settingsSidebar");
    sidebar->setFixedWidth(200);
    auto* sidebarLay = new QVBoxLayout(sidebar);
    sidebarLay->setContentsMargins(12, 20, 12, 20);
    sidebarLay->setSpacing(4);

    auto* sideTitle = new QLabel("تنظیمات");
    sideTitle->setObjectName("settingsSideTitle");
    sidebarLay->addWidget(sideTitle);

    auto* divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setObjectName("settingsDivider");
    sidebarLay->addWidget(divider);
    sidebarLay->addSpacing(8);

    // Nav buttons
    m_btnUsers = new QPushButton("مدیریت کاربران");
    m_btnUsers->setObjectName("settingsNavBtn");
    m_btnUsers->setCheckable(true);
    m_btnUsers->setChecked(true);

    m_btnAnimalTypes = new QPushButton("انواع حیوانات و واکسن‌ها");
    m_btnAnimalTypes->setObjectName("settingsNavBtn");
    m_btnAnimalTypes->setCheckable(true);

    m_btnBackup = new QPushButton("بکاپ");
    m_btnBackup->setObjectName("settingsNavBtn");
    m_btnBackup->setCheckable(true);

    m_btnAbout = new QPushButton("درباره");
    m_btnAbout->setObjectName("settingsNavBtn");
    m_btnAbout->setCheckable(true);

    sidebarLay->addWidget(m_btnUsers);
    sidebarLay->addWidget(m_btnAnimalTypes);

    // Backup tab — admin only
    if (Session::instance().isAdmin()) {
        sidebarLay->addWidget(m_btnBackup);
    }

    sidebarLay->addWidget(m_btnAbout);
    sidebarLay->addStretch();

    // ── Content stack ────────────────────────────────────────────────────────
    m_stack = new QStackedWidget;

    m_usersTab = new UsersTab(this);
    m_stack->addWidget(m_usersTab);       // index 0

    m_animalTypesTab = new AnimalTypesTab(this);
    m_stack->addWidget(m_animalTypesTab); // index 1

    m_backupTab = new BackupTab(this);
    m_stack->addWidget(m_backupTab);      // index 2

    m_aboutTab = new AboutTab(this);
    m_stack->addWidget(m_aboutTab);       // index 3

    m_stack->setCurrentIndex(0);

    rootLay->addWidget(sidebar);
    rootLay->addWidget(m_stack, 1);

    // ── Connect nav buttons ──────────────────────────────────────────────────
    connect(m_btnUsers,       &QPushButton::clicked, this, [this]() { switchTab(0); });
    connect(m_btnAnimalTypes, &QPushButton::clicked, this, [this]() { switchTab(1); });
    connect(m_btnBackup,      &QPushButton::clicked, this, [this]() { switchTab(2); });
    connect(m_btnAbout,       &QPushButton::clicked, this, [this]() { switchTab(3); });

    applyStyle();
}

void SettingsWidget::switchTab(int index)
{
    m_btnUsers->setChecked(false);
    m_btnAnimalTypes->setChecked(false);
    m_btnBackup->setChecked(false);
    m_btnAbout->setChecked(false);

    switch (index) {
    case 0: m_btnUsers->setChecked(true);       break;
    case 1: m_btnAnimalTypes->setChecked(true); break;
    case 2: m_btnBackup->setChecked(true);      break;
    case 3: m_btnAbout->setChecked(true);       break;
    }

    m_stack->setCurrentIndex(index);
}

void SettingsWidget::applyStyle()
{
    setStyleSheet(R"(
        QWidget#settingsSidebar {
            background: white;
            border-left: 1px solid #E8F5E9;
        }
        QLabel#settingsSideTitle {
            font-size: 15px;
            font-weight: bold;
            color: #2E7D32;
            background: transparent;
            padding: 4px 0px;
        }
        QFrame#settingsDivider {
            color: #E8F5E9;
        }
        QPushButton#settingsNavBtn {
            background: transparent;
            color: #555;
            border: none;
            border-radius: 8px;
            padding: 10px 14px;
            font-size: 13px;
            text-align: right;
        }
        QPushButton#settingsNavBtn:hover {
            background: #F1F8E9;
            color: #2E7D32;
        }
        QPushButton#settingsNavBtn:checked {
            background: #E8F5E9;
            color: #2E7D32;
            font-weight: bold;
        }
        QStackedWidget {
            background: #FAFAFA;
        }
    )");
}