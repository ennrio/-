#ifndef VEHICLEITEM_H
#define VEHICLEITEM_H

#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QObject>
#include "vehicle.h"

class VehicleItem : public QGraphicsEllipseItem, public QObject
{
public:
    VehicleItem(Vehicle* vehicle, QGraphicsItem* parent = nullptr);

    Vehicle* vehicle() const { return m_vehicle; }

    // QGraphicsItem override
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    Vehicle* m_vehicle;
    QColor m_color;
    QPen m_pen;
    QBrush m_brush;

    void updatePosition();
};

#endif // VEHICLEITEM_H
