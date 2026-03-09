#include "trafficlightitem.h"
#include <QCursor>

TrafficLightItem::TrafficLightItem(long long id,
                                   ClickCallback onClick,
                                   QGraphicsItem* parent)
    : QGraphicsEllipseItem(-6, -6, 12, 12, parent)
    , m_id(id)
    , m_onClick(onClick)
{
    setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::PointingHandCursor));
    setAcceptedMouseButtons(Qt::LeftButton);
}

void TrafficLightItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{

    if (event->button() == Qt::LeftButton) {
        if (m_onClick) {
            m_onClick(m_id);
        }
        event->accept();
        return;
    }
    QGraphicsEllipseItem::mousePressEvent(event);
}
