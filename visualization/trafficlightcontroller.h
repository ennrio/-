// trafficlightcontroller.h
#pragma once
#include <QObject>
#include <QTimer>
#include "trafficlight.h"
#include "../constants.h"

struct PhaseConfig {
    LightState state;
    int durationMs;  // длительность фазы в мс
};

class TrafficLightController : public QObject {
    Q_OBJECT
public:
    explicit TrafficLightController(TrafficLight* light, QObject* parent = nullptr);

    // Настройка стандартного цикла: красный → зелёный → жёлтый
    void setStandardCycle(int greenMs = 60000, int yellowMs = 5000, int redMs = 25000);

    // Для адаптивного управления: установить фазы динамически
    void setPhases(const QVector<PhaseConfig>& phases);

    // Управление извне (оператор/алгоритм)
    void forceState(LightState state, int durationMs = -1); // -1 = до отмены
    void resumeAutomatic();

    void restartCycle();
    void setMode(LightMode mode);

    LightState currentState() const;

signals:
    void phaseChanged(int lightId, LightState newState);
    void cycleCompleted(int lightId);

private slots:
    void onPhaseTimeout();
    void onBlinkTimeout();

private:
    TrafficLight* m_light;
    QTimer m_phaseTimer;
    QTimer m_blinkTimer;
    QVector<PhaseConfig> m_phases;
    int m_currentPhaseIndex{0};

    bool m_manualOverride{false};
    LightState m_overrideState{LightState::Red};
    LightMode lm;

    void startPhase(int index);
};
