#ifndef TRAFFICLIGHT_H
#define TRAFFICLIGHT_H

// ┌─────────────────────────────────┐
// │   TrafficLightManager (единый)  │
// │  • Координация перекрёстков     │
// │  • "Зелёная волна", приоритеты  │
// │  • Адаптивная логика (SCOOT)    │
// └────────┬────────────────────────┘
//          │ управляет
//          ▼
// ┌─────────────────────────────────┐
// │  IntersectionController         │
// │  • Группа светофоров одного     │
// │    перекрёстка                  │
// │  • Синхронизация фаз            │
// └────────┬────────────────────────┘
//          │ содержит
//          ▼
// ┌─────────────────────────────────┐
// │  TrafficLightController         │
// │  • Состояние одного светофора   │
// │  • Тайминги фаз, переходы       │
// │  • Сигналы: stateChanged()      │
// └────────┬────────────────────────┘
//          │ уведомляет
//          ▼
// ┌─────────────────────────────────┐
// │  TrafficLight (модель) + View   │
// │  • Данные: id, position, state  │
// │  • QGraphicsItem для отрисовки  │
// └─────────────────────────────────┐


#pragma once
#include <QObject>
#include <QPointF>

enum class LightState { Red, Yellow, Green, Off };

class TrafficLight : public QObject {
    Q_OBJECT
public:
    explicit TrafficLight(long long id,
                          QPointF pos,
                          const QString& direction,
                          bool isPedestrian = false,
                          QObject* parent = nullptr);

    long long id() const { return m_id; }
    QPointF position() const { return m_position; }
    QString direction() const { return m_direction; }
    bool isPedestrian() const { return m_isPedestrian; }
    LightState state() const { return m_currentState; }

    // Вызывается контроллером
    void setState(LightState newState);

signals:
    void stateChanged(int id, LightState newState);

private:
    int m_id;
    QPointF m_position;
    QString m_direction;
    LightState m_currentState{LightState::Red};
    bool m_isPedestrian;
};

#endif // TRAFFICLIGHT_H
