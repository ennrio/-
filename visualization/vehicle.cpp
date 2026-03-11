#include "vehicle.h"
#include <QLineF>
#include <QDebug>
#include <cmath>

Vehicle::Vehicle(QPointF startPosition, int id, QObject* parent)
    : QObject(parent)
    , m_id(id)
    , m_position(startPosition)
{
}

void Vehicle::setRoute(const QList<QPointF>& route)
{
    m_route = route;
    m_currentRouteIndex = 0;
}

void Vehicle::setSpeed(qreal maxSpeed)
{
    m_maxSpeed = maxSpeed;
}

void Vehicle::setTrafficLightAwareness(bool enabled)
{
    m_trafficLightAware = enabled;
}

void Vehicle::setTrafficLightChecker(TrafficLightChecker checker)
{
    m_trafficLightChecker = checker;
}

void Vehicle::update(double deltaTime)
{
    if (m_route.isEmpty() || m_currentRouteIndex >= m_route.size()) {
        return; // Маршрут завершён
    }

    // === ПРОВЕРКА СВЕТОФОРА ===
    if (m_trafficLightAware && m_stoppedAtLightId < 0) {
        if (checkTrafficLightAhead()) {
            // Светофор найден и требует остановки — тормозим
            applyBraking(deltaTime);
            if (m_speed < 0.1) {
                m_speed = 0;
                // emit stoppedAtLight(m_id, m_stoppedAtLightId);
                qDebug() << "Vehicle" << m_id << "stopped at light" << m_stoppedAtLightId;
            }
            emit positionChanged(m_id, m_position);
            return;
        }
    }

    // === ПРОВЕРКА ЗАВЕРШЕНИЯ МАРШРУТА ===
    if (distanceToNextPoint() < 1.0) {
        m_currentRouteIndex++;
        if (m_currentRouteIndex >= m_route.size()) {
            qDebug() << "Vehicle" << m_id << "reached destination";
            emit positionChanged(m_id, m_position);
            return;
        }
    }

    // === ДВИЖЕНИЕ ===
    // Если были остановлены у светофора и он зелёный
    if (m_stoppedAtLightId >= 0 && m_trafficLightChecker) {
        LightState state = m_trafficLightChecker(m_position, 5.0); // радиус 5м
        if (state == LightState::Green) {
            m_stoppedAtLightId = -1;
            // emit startedFromLight(m_id, -1);
            qDebug() << "Vehicle" << m_id << "started from light";
        }
    }

    // Ускорение если нет препятствий
    if (m_stoppedAtLightId < 0 && m_speed < m_maxSpeed) {
        applyAcceleration(deltaTime);
    }

    // Перемещение
    qreal distance = m_speed * deltaTime;
    moveAlongPath(distance);

    emit positionChanged(m_id, m_position);
}

bool Vehicle::checkTrafficLightAhead()
{
    if (!m_trafficLightChecker || m_currentRouteIndex >= m_route.size()) {
        return false;
    }

    QPointF nextPoint = m_route[m_currentRouteIndex];
    qreal distanceToIntersection = QLineF(m_position, nextPoint).length();

    // Проверяем светофор в радиусе 30 метров (≈60 пикселей) перед перекрёстком
    if (distanceToIntersection < 60.0 && distanceToIntersection > 5.0) {
        LightState state = m_trafficLightChecker(nextPoint, 10.0);

        if (state == LightState::Red || state == LightState::Yellow) {
            // Вычисляем тормозной путь
            qreal brakingDistance = (m_speed * m_speed) / (2.0 * m_deceleration);

            // Если не успеваем затормозить — начинаем торможение
            if (distanceToIntersection < brakingDistance + 10.0) {
                m_stoppedAtLightId = -2; // Временный флаг
                return true;
            }
        }
    }

    return false;
}

void Vehicle::applyBraking(double deltaTime)
{
    m_speed = qMax(0.0, m_speed - m_deceleration * deltaTime);
}

void Vehicle::applyAcceleration(double deltaTime)
{
    m_speed = qMin(m_maxSpeed, m_speed + m_acceleration * deltaTime);
}

void Vehicle::moveAlongPath(qreal distance)
{
    if (m_currentRouteIndex >= m_route.size()) return;

    QPointF target = m_route[m_currentRouteIndex];
    QLineF line(m_position, target);
    qreal lineLength = line.length();

    if (distance >= lineLength) {
        // Достигли точки — переходим к следующей
        m_position = target;
        distance -= lineLength;
        m_currentRouteIndex++;
        if (m_currentRouteIndex < m_route.size()) {
            moveAlongPath(distance); // Рекурсивно для плавности
        }
    } else {
        // Двигаемся вдоль текущего сегмента
        qreal ratio = distance / lineLength;
        m_position = QPointF(
            m_position.x() + (target.x() - m_position.x()) * ratio,
            m_position.y() + (target.y() - m_position.y()) * ratio
            );
    }
}

qreal Vehicle::distanceToNextPoint() const
{
    if (m_currentRouteIndex >= m_route.size()) return 0;
    return QLineF(m_position, m_route[m_currentRouteIndex]).length();
}
