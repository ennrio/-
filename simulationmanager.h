#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include "visualization/simulationview.h"
#include <QMap>

class SimulationManager
{
public:
    static SimulationManager& instance();

    // Запрет копирования
    SimulationManager(const SimulationManager&) = delete;
    SimulationManager& operator=(const SimulationManager&) = delete;

    // Основной метод: получение указателя на SimulationView
    SimulationView* simulationView() const;

    // Установка указателя
    void setSimulationView(SimulationView* view);

    // Остановка симуляции
    void stopSimulationView();

    // Получение количества активных авто (для старых вызовов)
    int getContActivevehicle();


    // Получить список светофоров для заполнения UI
    QMap<long long, CrossingInfo> getTrafficLightCrossings() const;

    // Применить настройки цикла к конкретному светофору
    void setTrafficLightCycle(long long id, int greenMs, int yellowMs, int redMs);

    // Сбросить светофор в автоматический режим
    void resetTrafficLightAuto(long long id);

    // Установить лимит машин
    void setMaxVehicleLimit(int limit);

private:
    explicit SimulationManager() = default;
    ~SimulationManager() = default;

    SimulationView* m_simulationView = nullptr;
};

#endif // SIMULATIONMANAGER_H
