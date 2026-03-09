#pragma once
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneMouseEvent>
#include <functional>

class TrafficLightItem : public QGraphicsEllipseItem {
public:
    using ClickCallback = std::function<void(long long)>;

    explicit TrafficLightItem(long long id,
                              ClickCallback onClick,
                              QGraphicsItem* parent = nullptr);

    long long id() const { return m_id; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    long long m_id;
    ClickCallback m_onClick;
};
