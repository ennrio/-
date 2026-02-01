#ifndef VEHICLE_H
#define VEHICLE_H

#include <QObject>
#include <QPointF>
#include <QList>

class Vehicle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPointF position READ position NOTIFY positionChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(double heading READ heading WRITE setHeading NOTIFY headingChanged)

    int m_id;
    QPointF m_position;
    double m_speed;      // м/с
    double m_heading;    // радианы
    QList<QPointF> m_routePoints;  // последовательность точек для движения
    int m_currentRouteIndex;
    bool m_isMoving;

public:
    explicit Vehicle(int id = 0, QObject *parent = nullptr);
    explicit Vehicle(const QPointF& startPosition, int id = 0, QObject *parent = nullptr);

    // Геттеры
    QPointF position() const { return m_position; }
    double speed() const { return m_speed; }
    double heading() const { return m_heading; }
    int id() const { return m_id; }
    bool isMoving() const { return m_isMoving; }

    // Сеттеры
    void setPosition(const QPointF& position);
    void setSpeed(double speed);
    void setHeading(double heading);
    void setId(int id) { m_id = id; }

    // Маршрутизация
    void setRoute(const QList<QPointF>& route);
    void clearRoute() { m_routePoints.clear(); m_currentRouteIndex = 0; m_isMoving = false; }
    const QList<QPointF>& route() const { return m_routePoints; }

    // Основной метод обновления
    void update(double dt);  // dt в секундах — интегрирование Эйлера

    // Вспомогательные методы
    void moveTo(const QPointF& target);
    void stop() { m_speed = 0.0; m_isMoving = false; }
    void start() { if (!m_routePoints.isEmpty()) m_isMoving = true; }

signals:
    void positionChanged();
    void speedChanged();
    void headingChanged();
    void routeCompleted();
    void routeChanged();
};

#endif // VEHICLE_H
