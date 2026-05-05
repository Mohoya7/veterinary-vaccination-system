#include "sidebarwidget.h"
#include <QPaintEvent>
#include <cmath>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{}

void SidebarWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Solid dark green background
    painter.fillRect(rect(), QColor("#1B5E20"));

    // Hex pattern — same as login dialog
    painter.setPen(QPen(QColor(255, 255, 255, 18), 1));
    painter.setBrush(Qt::NoBrush);

    int hexW = 36, hexH = 32;
    for (int row = -1; row * hexH < height() + hexH; row++) {
        for (int col = -1; col * hexW < width() + hexW; col++) {
            int x = col * hexW + (row % 2 == 0 ? 0 : hexW / 2);
            int y = row * hexH;
            QPolygonF hex;
            for (int i = 0; i < 6; i++) {
                double angle = M_PI / 180.0 * (60.0 * i - 30);
                hex << QPointF(x + 16 * cos(angle), y + 16 * sin(angle));
            }
            painter.drawPolygon(hex);
        }
    }

    // Decorative circles
    painter.setPen(Qt::NoPen);

    painter.setBrush(QColor(165, 214, 167, 25));
    painter.drawEllipse(-40, -40, 150, 150);

    painter.setBrush(QColor(249, 168, 37, 20));
    painter.drawEllipse(width() - 90, height() - 90, 120, 120);

    QWidget::paintEvent(event);
}