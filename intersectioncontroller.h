// intersectioncontroller.h
#pragma once
#include <QObject>
#include <QMap>
#include "trafficlightcontroller.h"

class IntersectionController : public QObject {
    Q_OBJECT
public:
    explicit IntersectionController(int intersectionId, QObject* parent = nullptr);

    void addLight(TrafficLightController* controller, LightState group);

    // Формирование "зелёной волны" или приоритета
    void setCoordinatedMode(bool enabled, int offsetMs = 0);
    void setPriorityForDirection(LightState dir, int priorityDurationMs);

private:
    int m_intersectionId;
    QMap<LightState, QList<TrafficLightController*>> m_lightGroups;
    bool m_coordinatedMode{false};
    int m_coordinationOffset{0};
};
