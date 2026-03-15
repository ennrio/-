#include "simulationmanager.h"

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
}

void SimulationManager::stopSimulationView()
{
    m_simulationView->stopSimulation();
}

int SimulationManager::getContActivevehicle()
{
    return 20;
}
