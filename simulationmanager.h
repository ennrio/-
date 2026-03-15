#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include "visualization/simulationview.h"

class SimulationManager
{
public:
    // === Singleton access ===
    static SimulationManager& instance();

    // === Запрет копирования ===
    SimulationManager(const SimulationManager&) = delete;
    SimulationManager& operator=(const SimulationManager&) = delete;

    // === Основной метод: получение указателя на SimulationView ===
    SimulationView* simulationView() const;

    // === Установка указателя (вызывается один раз при инициализации) ===
    void setSimulationView(SimulationView* view);

    void stopSimulationView();

    int getContActivevehicle();


private:
    // === Приватный конструктор ===
    explicit SimulationManager() = default;
    ~SimulationManager() = default;

    // === Данные ===
    SimulationView* m_simulationView;
};

#endif // SIMULATIONMANAGER_H
