#include "logindialog.h"
#include "ui_logindialog.h"
#include "styledmessagebox.h"
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setLayoutDirection(Qt::RightToLeft);
    ui->headerLayout->setAlignment(ui->logoLabel, Qt::AlignHCenter);

    QPixmap pixmap(90, 90);
    pixmap.fill(Qt::transparent);
    QPainter iconPainter(&pixmap);
    iconPainter.setRenderHint(QPainter::Antialiasing);

    iconPainter.setPen(QPen(QColor(255, 255, 255, 60), 2));
    iconPainter.setBrush(QColor(255, 255, 255, 50));
    iconPainter.drawEllipse(2, 2, 86, 86);

    iconPainter.setPen(Qt::NoPen);
    iconPainter.setBrush(QColor(255, 255, 255, 210));
    iconPainter.drawEllipse(18, 52, 54, 34);
    iconPainter.drawEllipse(20, 28, 50, 42);

    QPolygonF earLeft;
    earLeft << QPointF(22, 40) << QPointF(17, 18) << QPointF(35, 32);
    iconPainter.drawPolygon(earLeft);

    QPolygonF earRight;
    earRight << QPointF(68, 40) << QPointF(73, 18) << QPointF(55, 32);
    iconPainter.drawPolygon(earRight);

    iconPainter.setBrush(QColor("#2E7D32"));
    iconPainter.drawEllipse(28, 36, 12, 12);
    iconPainter.drawEllipse(50, 36, 12, 12);

    iconPainter.setBrush(QColor(249, 168, 37));
    iconPainter.drawEllipse(38, 48, 14, 10);

    iconPainter.end();
    ui->logoLabel->setPixmap(pixmap);
    ui->logoLabel->setAlignment(Qt::AlignCenter);
    ui->logoLabel->setFixedSize(90, 90);
    ui->logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(ui->loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);

    this->setStyleSheet(R"(
        QDialog {
            background-color: transparent;
        }
        QWidget#headerWidget {
            background-color: #2E7D32;
            border-top-left-radius: 16px;
            border-top-right-radius: 16px;
        }
        QWidget#formWidget {
            background-color: white;
        }
        QWidget#footerWidget {
            background-color: #F1F8E9;
            border-top: 1px solid #C8E6C9;
            border-bottom-left-radius: 16px;
            border-bottom-right-radius: 16px;
        }
        QLabel#titleLabel {
            color: white;
            font-size: 20px;
            font-weight: bold;
        }
        QLabel#subtitleLabel {
            color: rgba(255,255,255,0.7);
            font-size: 13px;
        }
        QLabel#usernameLabel, QLabel#passwordLabel {
            color: #5F5E5A;
            font-size: 13px;
        }
        QLabel#forgotLabel {
            color: #9E9E9E;
            font-size: 12px;
        }
        QLabel#versionLabel {
            color: #757575;
            font-size: 11px;
        }
        QLineEdit {
            border: 1px solid #A5D6A7;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 14px;
            background-color: #F9FBF9;
            color: #212121;
        }
        QLineEdit:focus {
            border: 1px solid #2E7D32;
            background-color: white;
        }
        QPushButton#loginButton {
            background-color: #2E7D32;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton#loginButton:hover {
            background-color: #388E3C;
        }
        QPushButton#loginButton:pressed {
            background-color: #1B5E20;
        }
    )");
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::onLoginClicked()
{
    QString username = ui->usernameEdit->text().trimmed();
    QString password = ui->passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "خطا", "لطفاً نام کاربری و رمز عبور را وارد کنید.");
        return;
    }

    QString hashedPassword = Database::hashPassword(password);

    QSqlQuery query;
    query.prepare("SELECT role FROM users WHERE username = :username AND password_hash = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", hashedPassword);
    query.exec();

    if (query.next()) {
        m_role = query.value("role").toString();
        accept();
    } else {
        QMessageBox::warning(this, "خطا", "نام کاربری یا رمز عبور اشتباه است.");
        ui->passwordEdit->clear();
        ui->passwordEdit->setFocus();
    }
}

void LoginDialog::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), QColor("#1B5E20"));

    painter.setPen(QPen(QColor(255, 255, 255, 15), 1));
    painter.setBrush(Qt::NoBrush);
    int hexW = 60, hexH = 52;
    for (int row = 0; row * hexH < height() + hexH; row++) {
        for (int col = 0; col * hexW < width() + hexW; col++) {
            int x = col * hexW + (row % 2 == 0 ? 0 : hexW / 2);
            int y = row * hexH;
            QPolygonF hex;
            for (int i = 0; i < 6; i++) {
                double angle = M_PI / 180.0 * (60.0 * i - 30);
                hex << QPointF(x + 26 * cos(angle), y + 26 * sin(angle));
            }
            painter.drawPolygon(hex);
        }
    }

    painter.setPen(QPen(QColor(165, 214, 167, 40), 2));
    painter.setBrush(QColor(165, 214, 167, 15));
    painter.drawEllipse(width() - 180, -100, 280, 280);
    painter.drawEllipse(-100, height() - 180, 280, 280);

    painter.setPen(QPen(QColor(249, 168, 37, 60), 2));
    painter.setBrush(QColor(249, 168, 37, 15));
    painter.drawEllipse(-40, 40, 120, 120);

    painter.setPen(QPen(QColor(249, 168, 37, 40), 1.5));
    painter.setBrush(QColor(249, 168, 37, 12));
    painter.drawEllipse(width() - 80, height() - 80, 140, 140);

    painter.setPen(QPen(QColor(249, 168, 37, 50), 1.5));
    painter.setBrush(QColor(249, 168, 37, 12));
    painter.drawEllipse(width() - 30, height() / 2 - 30, 60, 60);
}