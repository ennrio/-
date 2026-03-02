#ifndef SIMULATIONVIEW_H
#define SIMULATIONVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMap>
#include <QTimer>
#include <QElapsedTimer>
#include <QWheelEvent>
#include "roadgraph.h"
#include "vehicle.h"
#include "vehicleitem.h"

struct TrafficLight {
    int id;
    QPointF position;
    QString direction; // "forward", "backward", "all"
    bool isPedestrian;
};

class SimulationView : public QGraphicsView
{
    Q_OBJECT
public:
    SimulationView(QWidget *parent = nullptr);

    void show();
    void addNode(int id, double x, double y);
    void addEdge(int id, int from, int to, double length);
    void addVehicle(int id, const QPointF& startPosition);
    void setVehicleRoute(int vehicleId, const QList<int>& nodeIds);


    //условная компиляция
    void setupWindowsSpecific();
    void setupLinuxSpecific();

    void startSimulation();
    void stopSimulation();
    void setSimulationSpeed(double factor) { m_timeFactor = factor; }
    void drawRoad(long long fromId, long long toId, const QString &type);
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    void loadOSM(const QString &filename);
    QPointF convertLatLon(double lat, double lon) const;

protected:
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void updateSimulation();

private:
    QGraphicsScene *m_scene;
    QMap<int, QGraphicsEllipseItem*> m_nodeItems;
    QMap<int, QGraphicsLineItem*> m_edgeItems;
    QMap<int, Vehicle*> m_vehicles;           // Логика транспортных средств
    QMap<int, VehicleItem*> m_vehicleItems;   // Графические элементы
    QMap<long long, QPointF> m_nodePositions; // кэш позиций узлов в сцене
    QMap<long long, int> m_osmToInternalId;      // Маппинг: OSM ID → внутренний ID
    QMap<long long, QPointF> m_osmNodePositions;
    QMap<int, TrafficLight> m_trafficLights; // Кэш светофоров

    RoadGraph m_roadGraph;
    QTimer m_simulationTimer;
    QElapsedTimer m_elapsedTimer;
    double m_lastUpdateTime;
    double m_timeFactor; // Коэффициент скорости симуляции

    void drawRoadGraph();
    QPointF getNodePosition(int nodeId) const;
    void updateVehicleGraphics();
    QPointF latLonToScene(double lat, double lon) const;

    // Вспомогательные методы парсинга
    void parseOSMFile(const QString &filename);
    void drawOSMRoad(const QList<long long> &nodeRefs, const QString &highwayType);
};

#endif // SIMULATIONVIEW_H













