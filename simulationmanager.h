#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include "visualization/simulationview.h"
#include <QMap>
#include "accidentmanager.h"

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
    
    // === Методы управления ДТП ===
    AccidentManager* accidentManager() const;
    void createAccident(const QPointF &position, long long nodeId, const QString &severity = "Среднее");
    void resolveAccident(int accidentId);
    int getActiveAccidentsCount() const;

private:
    explicit SimulationManager() = default;
    ~SimulationManager() = default;

    SimulationView* m_simulationView = nullptr;
    AccidentManager* m_accidentManager = nullptr;
};

#endif // SIMULATIONMANAGER_H
