#include "trafficlight.h"


TrafficLight::TrafficLight(long long id, QPointF pos, const QString& direction,
                           bool isPedestrian, QObject* parent)
    : QObject(parent)
    , m_id(id)
    , m_position(pos)
    , m_direction(direction)
    , m_isPedestrian(isPedestrian)
{
}

void TrafficLight::setState(LightState newState)
{
    if (m_currentState != newState) {
        m_currentState = newState;
        emit stateChanged(m_id, newState); // ← ключевой сигнал для View
    }
}
