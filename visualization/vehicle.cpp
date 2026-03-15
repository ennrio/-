#include "vehicle.h"
#include <QLineF>
#include <QDebug>
#include <QDateTime>
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

void Vehicle::markRouteFinished()
{
    m_routeFinished = true;
    m_active = false;
    m_finishedTimestamp = QDateTime::currentMSecsSinceEpoch();
    m_speed = 0.0;  // Останавливаем
    qDebug() << "Vehicle" << m_id << "marked as finished at"
             << QDateTime::fromMSecsSinceEpoch(m_finishedTimestamp).toString("hh:mm:ss");
}

void Vehicle::setNightMode(bool enabled)
{
    if(enabled){
        lm = nightMode;
    }
}

bool Vehicle::isNightMode() const
{
    return lm == LightMode::nightMode;
}

void Vehicle::setTrafficLightChecker(TrafficLightChecker checker)
{
    m_trafficLightChecker = checker;
}

void Vehicle::update(double deltaTime)
{
    // === БАЗОВЫЕ ПРОВЕРКИ ===
    if (m_route.isEmpty() || m_currentRouteIndex >= m_route.size()) {
        markRouteFinished();
        return;
    }

    // Получаем состояние светофора впереди
    LightState currentLightState = LightState::Green;
    bool isLightAhead = false;

    if (m_trafficLightAware && m_trafficLightChecker) {
        currentLightState = m_trafficLightChecker(m_position, 15.0);
        isLightAhead = (currentLightState != LightState::Off);
    }

    // ==========================================
    // РЕШЕНИЕ: ТОРМОЗИТЬ ИЛИ ЕХАТЬ?
    // ==========================================

    bool mustStop = false;

    if (isNightMode()) {
        // Стоим ТОЛЬКО если горит КРАСНЫЙ (на Невском проспекте).
        // На мигающий ЖЕЛТЫЙ не реагируем как на запрет - просто едем.
        if (currentLightState == LightState::Red) {
            mustStop = true;
        } else {
            mustStop = false; // Желтый или Зеленый - едем свободно
        }
    }
    else {
        // Стоим на Красный и Желтый
        if (currentLightState == LightState::Red || currentLightState == LightState::Yellow) {
            mustStop = true;
        } else {
            mustStop = false;
        }
    }


    if (mustStop) {
        if (m_stoppedAtLightId < 0) {
            m_stoppedAtLightId = -2; // Флаг "остановлен светофором"
        }

        applyBraking(deltaTime);
        if (m_speed < 0.1) {
            m_speed = 0;
        }
        emit positionChanged(m_id, m_position);
        return; // Выход, так как мы стоим
    }

    if (m_stoppedAtLightId < 0) {
        // Мы не остановлены, просто продолжаем движение
    } else {
        bool canGo = false;
        if (isNightMode()) {
            if (currentLightState == LightState::Yellow || currentLightState == LightState::Green) {
                canGo = true;
            }
        } else {
            if (currentLightState == LightState::Green) {
                canGo = true;
            }
        }

        if (canGo) {
            m_stoppedAtLightId = -1; // Сброс флага остановки
            // qDebug() << "Vehicle" << m_id << "started from light";
        } else {
            // Все еще ждем разрешающего сигнала
            applyBraking(deltaTime);
            if (m_speed < 0.1) m_speed = 0;
            emit positionChanged(m_id, m_position);
            return;
        }
    }

    // 3. Движение и ускорение (теперь всегда полное, без замедления)
    if (m_stoppedAtLightId < 0) {
        if (m_speed < m_maxSpeed) {
            applyAcceleration(deltaTime);
        }
    }

    // 4. Перемещение
    qreal pixelsPerSecond = m_speed / m_metersPerPixel;
    qreal distancePixels = pixelsPerSecond * deltaTime;
    moveAlongPath(distancePixels);

    // 5. Проверка достижения точки
    if (distanceToNextPoint() < 1.0) {
        m_currentRouteIndex++;
        if (m_currentRouteIndex >= m_route.size()) {
            markRouteFinished();
        }
    }

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
