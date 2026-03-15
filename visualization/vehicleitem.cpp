#include "vehicleitem.h"
#include <QDebug>


VehicleItem::VehicleItem(Vehicle* vehicle, QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent)
    , m_vehicle(vehicle)
{
    // цвет машины
    setRect(-5, -5, 10, 10); // Круг радиусом 5
    setBrush(QBrush(QColor(300, 100, 100)));
    m_pen = QPen(Qt::black, 1);
    m_brush = QBrush(m_color);

    // Подключаемся к сигналам Vehicle
    connect(m_vehicle, &Vehicle::positionChanged, this, &VehicleItem::updatePosition);

    // Устанавливаем начальную позицию
    updatePosition();
}

void VehicleItem::setColor(const QColor &color)
{
    m_color = color;
    m_brush = QBrush(color);
    update();
}

void VehicleItem::updatePosition()
{
    QPointF pos = m_vehicle->position();
    setPos(pos);
}

void VehicleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setPen(m_pen);
    painter->setBrush(m_brush);
    painter->drawEllipse(rect());

    // Рисуем направление (стрелку)
    painter->drawLine(0, 0, 7, 0);
}
