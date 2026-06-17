#include "styledmessagebox.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPaintEvent>

StyledMessageBox::StyledMessageBox(Type type, const QString& title,
                                   const QString& message, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setLayoutDirection(Qt::RightToLeft);
    setFixedWidth(400);

    // Theme per type
    struct Theme { QString accent, iconBg, iconFg, iconText, btnHover; };
    Theme t;
    switch (type) {
    case Question:
        t = {"#2E7D32", "#E8F5E9", "#2E7D32", "؟", "#1B5E20"}; break;
    case Warning:
        t = {"#F57F17", "#FFF8E1", "#F57F17", "!", "#E65100"}; break;
    case Error:
        t = {"#C62828", "#FFEBEE", "#C62828", "✕", "#B71C1C"}; break;
    case Success:
    default:
        t = {"#2E7D32", "#E8F5E9", "#2E7D32", "✓", "#1B5E20"}; break;
    }

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Card — no shadow, no glass border
    auto* card = new QWidget;
    card->setObjectName("smb_card");
    card->setStyleSheet(R"(
        QWidget#smb_card {
            background: white;
            border-radius: 12px;
        }
    )");

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // Top accent bar
    auto* bar = new QWidget;
    bar->setFixedHeight(4);
    bar->setStyleSheet(QString(
                           "background:%1;"
                           "border-top-left-radius:12px;"
                           "border-top-right-radius:12px;"
                           ).arg(t.accent));
    cardLayout->addWidget(bar);

    // Body
    auto* body = new QWidget;
    body->setStyleSheet("background:transparent;");
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(24, 20, 24, 20);
    bodyLay->setSpacing(16);

    // Icon + Title row — icon on the left (visual right in RTL), title next to it
    auto* topRow = new QHBoxLayout;
    topRow->setSpacing(12);
    topRow->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* iconW = new QLabel(t.iconText);
    iconW->setFixedSize(40, 40);
    iconW->setAlignment(Qt::AlignCenter);
    iconW->setStyleSheet(QString(R"(
        background: %1;
        color: %2;
        border-radius: 20px;
        font-size: 17px;
        font-weight: bold;
    )").arg(t.iconBg, t.iconFg));

    auto* titleL = new QLabel(title);
    titleL->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleL->setStyleSheet(
        "font-size:16px;font-weight:600;color:#1A1A1A;background:transparent;");

    // RTL: icon appears on the right visually, title to its left
    topRow->addWidget(titleL, 1);
    topRow->addWidget(iconW);

    auto* msgL = new QLabel(message);
    msgL->setWordWrap(true);
    msgL->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    msgL->setStyleSheet(
        "font-size:13px;color:#555;line-height:1.6;background:transparent;");

    // Divider
    auto* divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color:#F0F0F0;");

    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    btnRow->setAlignment(Qt::AlignLeft);

    if (type == Question) {
        auto* btnYes = new QPushButton("بله");
        btnYes->setFixedHeight(38);
        btnYes->setMinimumWidth(88);
        btnYes->setCursor(Qt::PointingHandCursor);
        btnYes->setStyleSheet(QString(R"(
            QPushButton {
                background: %1;
                color: white;
                border: none;
                border-radius: 8px;
                font-size: 13px;
                font-weight: 500;
                padding: 0 20px;
            }
            QPushButton:hover { background: %2; }
            QPushButton:pressed { background: %2; }
        )").arg(t.accent, t.btnHover));
        connect(btnYes, &QPushButton::clicked, this, [this]() {
            m_accepted = true;
            accept();
        });

        auto* btnNo = new QPushButton("خیر");
        btnNo->setFixedHeight(38);
        btnNo->setMinimumWidth(88);
        btnNo->setCursor(Qt::PointingHandCursor);
        btnNo->setStyleSheet(R"(
            QPushButton {
                background: white;
                color: #555;
                border: 1px solid #E0E0E0;
                border-radius: 8px;
                font-size: 13px;
                font-weight: 500;
                padding: 0 20px;
            }
            QPushButton:hover { background: #F5F5F5; border-color: #BDBDBD; }
            QPushButton:pressed { background: #EEEEEE; }
        )");
        connect(btnNo, &QPushButton::clicked, this, &QDialog::reject);

        btnRow->addWidget(btnYes);
        btnRow->addWidget(btnNo);
        btnRow->addStretch();

    } else {
        auto* btnOk = new QPushButton("باشه");
        btnOk->setFixedHeight(38);
        btnOk->setMinimumWidth(100);
        btnOk->setCursor(Qt::PointingHandCursor);
        btnOk->setStyleSheet(QString(R"(
            QPushButton {
                background: %1;
                color: white;
                border: none;
                border-radius: 8px;
                font-size: 13px;
                font-weight: 500;
                padding: 0 24px;
            }
            QPushButton:hover { background: %2; }
            QPushButton:pressed { background: %2; }
        )").arg(t.accent, t.btnHover));
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);

        btnRow->addWidget(btnOk);
        btnRow->addStretch();
    }

    bodyLay->addLayout(topRow);
    bodyLay->addWidget(msgL);
    bodyLay->addWidget(divider);
    bodyLay->addLayout(btnRow);

    cardLayout->addWidget(body);
    root->addWidget(card);

    adjustSize();
}

void StyledMessageBox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
}

bool StyledMessageBox::question(QWidget* parent, const QString& title, const QString& message)
{
    StyledMessageBox dlg(Question, title, message, parent);
    dlg.exec();
    return dlg.m_accepted;
}

void StyledMessageBox::warning(QWidget* parent, const QString& title, const QString& message)
{
    StyledMessageBox dlg(Warning, title, message, parent);
    dlg.exec();
}

void StyledMessageBox::success(QWidget* parent, const QString& title, const QString& message)
{
    StyledMessageBox dlg(Success, title, message, parent);
    dlg.exec();
}

void StyledMessageBox::error(QWidget* parent, const QString& title, const QString& message)
{
    StyledMessageBox dlg(Error, title, message, parent);
    dlg.exec();
}