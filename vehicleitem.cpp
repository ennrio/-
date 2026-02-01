#include "vehicleitem.h"
#include <QDebug>


VehicleItem::VehicleItem(Vehicle* vehicle, QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent)
    , m_vehicle(vehicle)
{
    // Устанавливаем размер и цвет транспортного средства
    setRect(-5, -5, 10, 10); // Круг радиусом 5
    m_color = QColor(255, 0, 0); // Красный цвет
    m_pen = QPen(Qt::black, 1);
    m_brush = QBrush(m_color);

    // Подключаемся к сигналам Vehicle
    connect(m_vehicle, &Vehicle::positionChanged, this, &VehicleItem::updatePosition);

    // Устанавливаем начальную позицию
    updatePosition();
}

void VehicleItem::updatePosition()
{
    QPointF pos = m_vehicle->position();
    setPos(pos);

    // Поворачиваем транспорт по направлению движения
    double degrees = m_vehicle->heading() * 180.0 / M_PI;
    setRotation(degrees);
}

void VehicleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setPen(m_pen);
    painter->setBrush(m_brush);
    painter->drawEllipse(rect());

    // Рисуем направление (стрелку)
    painter->drawLine(0, 0, 7, 0);
}
