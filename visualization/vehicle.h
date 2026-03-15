#pragma once
#include <QObject>
#include <QPointF>
#include <QList>
#include "trafficlight.h"

/*
┌─────────────────────────────────────────────────────────┐
│                    Vehicle                              │
│  • position, speed, route                               │
│  • update(deltaTime) — основная логика                  │
│  • checkTrafficLightAhead() — проверка светофора        │
│  • applyBraking()/applyAcceleration() — физика          │
└─────────────────────────────────────────────────────────┘
                          │
                          │ запрашивает состояние
                          ▼
┌─────────────────────────────────────────────────────────┐
│              SimulationView                             │
│  • m_trafficLights — модель светофоров                  │
│  • m_controllers — контроллеры                          │
│  • getTrafficLightAtPosition() — поиск по координатам   │
└─────────────────────────────────────────────────────────┘

*/

class Vehicle : public QObject {
    Q_OBJECT
public:
    explicit Vehicle(QPointF startPosition, int id, QObject* parent = nullptr);

    void update(double deltaTime);
    void setRoute(const QList<QPointF>& route);
    void setTrafficLightAwareness(bool enabled);

    bool isActive() const { return m_active; }
    bool isRouteFinished() const { return m_routeFinished; }
    qint64 finishedTimestamp() const { return m_finishedTimestamp; }
    void markRouteFinished();

    QPointF position() const { return m_position; }
    qreal speed() const { return m_speed; }
    int id() const { return m_id; }

    // Для связи с SimulationView
    using TrafficLightChecker = std::function<LightState(const QPointF&, qreal)>;
    void setTrafficLightChecker(TrafficLightChecker checker);

    //управление скоростью
    void setSpeed(qreal maxSpeed);
    void setMaxSpeed(qreal speed) { m_maxSpeed = speed; } // Алиас
    void setAcceleration(qreal acc) { m_acceleration = acc; }
    void setDeceleration(qreal dec) { m_deceleration = dec; }


signals:
    void positionChanged(int id, QPointF newPos);
    void stoppedAtLight(int id, long long lightId);
    void startedFromLight(int id, long long lightId);

private:
    int m_id;
    QPointF m_position;
    qreal m_speed{0.0};
    qreal m_maxSpeed{15.0}; // м/с (~54 км/ч)
    qreal m_acceleration{2.0}; // м/с²
    qreal m_deceleration{4.0}; // м/с² для торможения
    qreal m_metersPerPixel{0.5};
    QList<QPointF> m_route;
    int m_currentRouteIndex{0};

    bool m_trafficLightAware{true};
    long long m_stoppedAtLightId{-1};
    TrafficLightChecker m_trafficLightChecker;

    void moveAlongPath(qreal distance);
    qreal distanceToNextPoint() const;
    bool checkTrafficLightAhead();
    void applyBraking(double deltaTime);
    void applyAcceleration(double deltaTime);

    bool m_active{true};
    bool m_routeFinished{false};
    qint64 m_finishedTimestamp{0};
};
