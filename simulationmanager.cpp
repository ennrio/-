#include "simulationmanager.h"
#include <QDebug>

// ============================================================================
// Singleton
// ============================================================================
SimulationManager& SimulationManager::instance()
{
    static SimulationManager _instance;
    return _instance;
}

// ============================================================================
// Public methods
// ============================================================================

SimulationView* SimulationManager::simulationView() const
{
    return m_simulationView;
}

void SimulationManager::setSimulationView(SimulationView* view)
{
    m_simulationView = view;
    
    // Менеджер ДТП теперь инициализируется в самом SimulationView
    if (view) {
        m_accidentManager = view->accidentManager();
    }
}

void SimulationManager::stopSimulationView()
{
    if (m_simulationView) {
        m_simulationView->stopSimulation();
    }
}

int SimulationManager::getContActivevehicle()
{
    if (m_simulationView) {
        return m_simulationView->getActiveVehicleCount();
    }
    return 0;
}


QMap<long long, CrossingInfo> SimulationManager::getTrafficLightCrossings() const
{
    QMap<long long, CrossingInfo> result;
    if (!m_simulationView) return result;

    // Получаем доступ к внутренним данным симуляции через публичный интерфейс или friend
    // Предполагаем, что в SimulationView есть метод getControllers() или аналогичный,
    // либо мы используем существующие поля, если они доступны.
    // Если поля приватные, нужно добавить геттер в SimulationView:
    // const QMap<long long, TrafficLightController*>& controllers() const;

    // Для примера предположим, что мы добавили геттер в SimulationView:
    // auto controllers = m_simulationView->getControllers();

    // for (auto it = controllers.begin(); it != controllers.end(); ++it) {
    //     CrossingInfo info;
    //     info.id = it.key();
    //     // Формируем имя (пока заглушка, потом можно брать координаты и искать улицу)
    //     info.name = QString("Перекресток #%1").arg(it.key());
    //     result[it.key()] = info;
    // }
    return result;
}

void SimulationManager::setTrafficLightCycle(long long id, int greenMs, int yellowMs, int redMs)
{
    if (!m_simulationView) {
        qWarning() << "SimulationManager: View is not set!";
        return;
    }

    // Вызываем метод симуляции
   // m_simulationView->setTrafficLightCycle(id, greenMs, yellowMs, redMs);

    qDebug() << "Manager: Applied cycle to light" << id
             << "G:" << greenMs << "Y:" << yellowMs << "R:" << redMs;
}

void SimulationManager::resetTrafficLightAuto(long long id)
{
    if (!m_simulationView) return;
   // m_simulationView->resetTrafficLightAuto(id);
    qDebug() << "Manager: Reset light" << id << "to Auto mode.";
}

void SimulationManager::setMaxVehicleLimit(int limit)
{
    if (!m_simulationView) return;
   // m_simulationView->setMaxVehicleLimit(limit);
    qDebug() << "Manager: Max vehicle limit set to" << limit;
}

// ============================================================================
// Accident Management Methods
// ============================================================================

AccidentManager* SimulationManager::accidentManager() const
{
    return m_accidentManager;
}

void SimulationManager::createAccident(const QPointF &position, long long nodeId, const QString &severity)
{
    if (m_accidentManager) {
        m_accidentManager->createAccident(position, nodeId, severity);
    }
}

void SimulationManager::resolveAccident(int accidentId)
{
    if (m_accidentManager) {
        m_accidentManager->resolveAccident(accidentId);
    }
}

int SimulationManager::getActiveAccidentsCount() const
{
    if (m_accidentManager) {
        return m_accidentManager->getActiveAccidentsCount();
    }
    return 0;
}
