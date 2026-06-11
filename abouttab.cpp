#include "abouttab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

AboutTab::AboutTab(QWidget* parent)
    : QWidget(parent)
{
    setLayoutDirection(Qt::RightToLeft);

    auto* rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(24, 24, 24, 24);
    rootLay->setSpacing(20);

    auto* pageTitle = new QLabel("درباره");
    pageTitle->setObjectName("pageTitle");
    rootLay->addWidget(pageTitle);

    // ── Card ─────────────────────────────────────────────────────────────────
    auto* card = new QWidget;
    card->setObjectName("aboutCard");
    auto* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(24, 24, 24, 28);
    cardLay->setSpacing(0);

    // App icon placeholder
    auto* iconLbl = new QLabel("🐾");
    iconLbl->setAlignment(Qt::AlignCenter);
    iconLbl->setObjectName("appIcon");
    cardLay->addWidget(iconLbl);
    cardLay->addSpacing(12);

    auto* appName = new QLabel("نرم‌افزار مدیریت واکسیناسیون دامپزشکی");
    appName->setAlignment(Qt::AlignCenter);
    appName->setObjectName("appName");
    cardLay->addWidget(appName);
    cardLay->addSpacing(6);

    auto* version = new QLabel("نسخه ۱.۰.۰");
    version->setAlignment(Qt::AlignCenter);
    version->setObjectName("versionLabel");
    cardLay->addWidget(version);
    cardLay->addSpacing(24);

    auto* div = new QFrame;
    div->setFrameShape(QFrame::HLine);
    div->setObjectName("aboutDivider");
    cardLay->addWidget(div);
    cardLay->addSpacing(20);

    // Info rows
    auto addInfoRow = [&](const QString& label, const QString& value) {
        auto* row = new QHBoxLayout;
        row->setSpacing(12);

        auto* lbl = new QLabel(label);
        lbl->setObjectName("infoLabel");
        lbl->setFixedWidth(130);

        auto* val = new QLabel(value);
        val->setObjectName("infoValue");
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);

        row->addWidget(lbl);
        row->addWidget(val, 1);
        cardLay->addLayout(row);
        cardLay->addSpacing(10);
    };

    addInfoRow("توسعه‌دهنده:", "محسن مشرفی");
    addInfoRow("تاریخ انتشار:", "بهمن ۱۴۰۴");
    addInfoRow("تکنولوژی:", "Qt 6 + MySQL 8");
    addInfoRow("پشتیبانی:", "برای پشتیبانی با توسعه‌دهنده تماس بگیرید");

    rootLay->addWidget(card);
    rootLay->addStretch();

    applyStyle();
}

void AboutTab::applyStyle()
{
    setStyleSheet(R"(
        QLabel#pageTitle {
            font-size: 18px; font-weight: bold; color: #212121; background: transparent;
        }
        QWidget#aboutCard {
            background: white; border: 1px solid #E8F5E9; border-radius: 10px;
        }
        QLabel#appIcon {
            font-size: 48px; background: transparent;
        }
        QLabel#appName {
            font-size: 16px; font-weight: bold; color: #212121; background: transparent;
        }
        QLabel#versionLabel {
            font-size: 13px; color: #757575; background: transparent;
        }
        QFrame#aboutDivider { color: #E8F5E9; }
        QLabel#infoLabel {
            font-size: 13px; color: #757575; background: transparent; font-weight: 500;
        }
        QLabel#infoValue {
            font-size: 13px; color: #212121; background: transparent;
        }
    )");
}