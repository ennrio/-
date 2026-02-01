#ifndef SIMULATIONVIEW_H
#define SIMULATIONVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMap>
#include <QTimer>
#include <QElapsedTimer>
#include "roadgraph.h"
#include "vehicle.h"
#include "vehicleitem.h"

class SimulationView : public QGraphicsView
{
    Q_OBJECT
public:
    SimulationView(QWidget *parent = nullptr);

    void addNode(int id, double x, double y);
    void addEdge(int id, int from, int to, double length);
    void addVehicle(int id, const QPointF& startPosition);
    void setVehicleRoute(int vehicleId, const QList<int>& nodeIds);

    void startSimulation();
    void stopSimulation();
    void setSimulationSpeed(double factor) { m_timeFactor = factor; }

private slots:
    void updateSimulation();

private:
    QGraphicsScene *m_scene;
    QMap<int, QGraphicsEllipseItem*> m_nodeItems;
    QMap<int, QGraphicsLineItem*> m_edgeItems;
    QMap<int, Vehicle*> m_vehicles;           // Логика транспортных средств
    QMap<int, VehicleItem*> m_vehicleItems;   // Графические элементы

    RoadGraph m_roadGraph;
    QTimer m_simulationTimer;
    QElapsedTimer m_elapsedTimer;
    double m_lastUpdateTime;
    double m_timeFactor; // Коэффициент скорости симуляции

    void drawRoadGraph();
    QPointF getNodePosition(int nodeId) const;
    void updateVehicleGraphics();
};

#endif // SIMULATIONVIEW_H













