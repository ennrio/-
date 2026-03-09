// trafficlightcontroller.cpp
#include "trafficlightcontroller.h"
#include <QDebug>

// ============================================================================
// Конструктор / Деструктор
// ============================================================================

TrafficLightController::TrafficLightController(TrafficLight* light, QObject* parent)
    : QObject(parent)
    , m_light(light)
    , m_currentPhaseIndex(0)
    , m_manualOverride(false)
    , m_overrideState(LightState::Red)
{
    if (!m_light) {
        qWarning() << "TrafficLightController created with nullptr light";
        return;
    }

    // Соединяем таймер с обработчиком
    connect(&m_phaseTimer, &QTimer::timeout, this, &TrafficLightController::onPhaseTimeout);

    // Устанавливаем дефолтный цикл при создании
    setStandardCycle();
}

// ============================================================================
// Публичные методы
// ============================================================================

void TrafficLightController::setStandardCycle(int greenMs, int yellowMs, int redMs)
{
    // Стандартная последовательность для автомобильного светофора:
    // Красный → Зелёный → Жёлтый → (повтор)
    // Примечание: в некоторых странах порядок: Зелёный → Жёлтый → Красный

    m_phases.clear();

    // Фаза 1: Красный
    m_phases.append({LightState::Red, redMs});

    // Фаза 2: Зелёный
    m_phases.append({LightState::Green, greenMs});

    // Фаза 3: Жёлтый
    m_phases.append({LightState::Yellow, yellowMs});

    // Сбрасываем индекс и запускаем с начала
    m_currentPhaseIndex = 0;
    m_manualOverride = false;

    // Запускаем первую фазу немедленно
    if (!m_phases.isEmpty()) {
        startPhase(m_currentPhaseIndex);
    }
}

void TrafficLightController::setPhases(const QVector<PhaseConfig>& phases)
{
    if (phases.isEmpty()) {
        qWarning() << "setPhases called with empty phases vector";
        return;
    }

    // Валидация: длительность фазы должна быть > 0 для авто-режима
    for (int i = 0; i < phases.size(); ++i) {
        if (phases[i].durationMs <= 0 && phases[i].state != LightState::Off) {
            qWarning() << "Phase" << i << "has invalid duration:" << phases[i].durationMs;
        }
    }

    m_phases = phases;
    m_currentPhaseIndex = 0;
    m_manualOverride = false;

    if (!m_phases.isEmpty()) {
        startPhase(m_currentPhaseIndex);
    }
}

void TrafficLightController::forceState(LightState state, int durationMs)
{
    if (!m_light) return;

    m_manualOverride = true;
    m_overrideState = state;

    // Устанавливаем состояние напрямую
    m_light->setState(state);
    emit phaseChanged(m_light->id(), state);

    // Если указана длительность — ставим таймер на возврат к авто-режиму
    if (durationMs > 0) {
        m_phaseTimer.start(durationMs);
    } else if (durationMs == 0) {
        //durationMs == 0 означает мгновенный возврат
        m_phaseTimer.stop();
    }
    // durationMs < 0 означает "бессрочно", таймер не запускаем
}

void TrafficLightController::resumeAutomatic()
{
    m_manualOverride = false;

    // Возвращаемся к текущей фазе цикла
    if (!m_phases.isEmpty()) {
        startPhase(m_currentPhaseIndex);
    }
}

LightState TrafficLightController::currentState() const
{
    if (!m_light) return LightState::Off;
    return m_light->state();
}

// ============================================================================
// Приватные методы
// ============================================================================

void TrafficLightController::startPhase(int index)
{
    if (!m_light || index < 0 || index >= m_phases.size()) {
        qWarning() << "startPhase: invalid index" << index << "for phases size" << m_phases.size();
        return;
    }

    const PhaseConfig& phase = m_phases[index];

    // Если в ручном режиме — не меняем состояние автоматически
    if (m_manualOverride) {
        return;
    }

    // Устанавливаем новое состояние светофора
    m_light->setState(phase.state);
    emit phaseChanged(m_light->id(), phase.state);

    // Запускаем таймер для следующей фазы, если длительность > 0
    if (phase.durationMs > 0) {
        m_phaseTimer.start(phase.durationMs);
    } else {
        m_phaseTimer.stop();
    }

// Отладочный вывод
#ifdef QT_DEBUG
    QString stateStr;
    switch (phase.state) {
    case LightState::Red: stateStr = "RED"; break;
    case LightState::Yellow: stateStr = "YELLOW"; break;
    case LightState::Green: stateStr = "GREEN"; break;
    case LightState::Off: stateStr = "OFF"; break;
    }
    // qDebug() << "TrafficLight" << m_light->id()
    //          << "→" << stateStr
    //          << "(" << phase.durationMs << "ms)";
#endif
}

void TrafficLightController::onPhaseTimeout()
{
    // Если ручной режим активен и таймер сработал для временного override — возвращаем авто
    if (m_manualOverride && !m_phases.isEmpty()) {
        // Проверяем, не был ли это временный forceState
        // Если m_overrideState не совпадает с текущей фазой — значит это был override
        if (m_phases[m_currentPhaseIndex].state != m_overrideState) {
            resumeAutomatic();
            return;
        }
    }

    // Переход к следующей фазе цикла
    m_currentPhaseIndex = (m_currentPhaseIndex + 1) % m_phases.size();

    // Если вернулись к началу цикла — сигнализируем
    if (m_currentPhaseIndex == 0) {
        emit cycleCompleted(m_light->id());
    }

    // Запускаем новую фазу
    startPhase(m_currentPhaseIndex);
}
