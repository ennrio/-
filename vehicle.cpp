#include "vehicle.h"
#include <QDebug>
#include <cmath>

Vehicle::Vehicle(int id, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_position(0, 0)
    , m_speed(0.0)
    , m_heading(0.0)
    , m_currentRouteIndex(0)
    , m_isMoving(false)
{
}

Vehicle::Vehicle(const QPointF& startPosition, int id, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_position(startPosition)
    , m_speed(0.0)
    , m_heading(0.0)
    , m_currentRouteIndex(0)
    , m_isMoving(false)
{
}

void Vehicle::setPosition(const QPointF& position)
{
    if (m_position != position) {
        m_position = position;
        emit positionChanged();
    }
}

void Vehicle::setSpeed(double speed)
{
    if (qFuzzyCompare(m_speed, speed))
        return;

    m_speed = speed;
    emit speedChanged();
}

void Vehicle::setHeading(double heading)
{
    // Нормализуем угол к диапазону [0, 2π)
    while (heading < 0) heading += 2 * M_PI;
    while (heading >= 2 * M_PI) heading -= 2 * M_PI;

    if (!qFuzzyCompare(m_heading, heading)) {
        m_heading = heading;
        emit headingChanged();
    }
}

void Vehicle::setRoute(const QList<QPointF>& route)
{
    m_routePoints = route;
    m_currentRouteIndex = 0;
    m_isMoving = !route.isEmpty();
    emit routeChanged();

    if (!route.isEmpty()) {
        // Направляем транспорт к первой точке маршрута
        QPointF direction = route.first() - m_position;
        if (!direction.isNull()) {
            m_heading = std::atan2(direction.y(), direction.x());
            emit headingChanged();
        }
    }
}

void Vehicle::moveTo(const QPointF& target)
{
    QList<QPointF> route;
    route.append(target);
    setRoute(route);
}

void Vehicle::update(double dt)
{
    if (!m_isMoving || m_speed <= 0.0 || m_routePoints.isEmpty())
        return;

    // Получаем текущую целевую точку
    QPointF target = m_routePoints[m_currentRouteIndex];
    QPointF direction = target - m_position;
    double distanceToTarget = std::sqrt(direction.x() * direction.x() +
                                        direction.y() * direction.y());

    // Если мы уже у цели, переходим к следующей точке
    if (distanceToTarget < 0.1) {  // Порог достижения точки
        m_currentRouteIndex++;

        // Проверяем, завершен ли маршрут
        if (m_currentRouteIndex >= m_routePoints.size()) {
            m_isMoving = false;
            emit routeCompleted();
            return;
        }

        // Обновляем цель
        target = m_routePoints[m_currentRouteIndex];
        direction = target - m_position;
        distanceToTarget = std::sqrt(direction.x() * direction.x() +
                                     direction.y() * direction.y());
    }

    // Вычисляем шаг движения
    double step = m_speed * dt;

    // Если до цели ближе, чем шаг, то просто достигаем цели
    if (step >= distanceToTarget) {
        m_position = target;
    } else {
        // Нормализуем направление и двигаемся
        QPointF normalizedDir = direction / distanceToTarget;
        m_position += normalizedDir * step;

        // Обновляем направление
        double newHeading = std::atan2(normalizedDir.y(), normalizedDir.x());
        setHeading(newHeading);
    }

    emit positionChanged();
}
