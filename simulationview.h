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
#include "QFile"
#include <QXmlStreamReader>
#include "trafficlight.h"
#include "trafficlightcontroller.h"
#include "trafficlightitem.h"

struct PendingWay {
    QList<long long> nodeRefs;
    QString highwayType;
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
    QPointF convertLatLon(double lat, double lon) const;

protected:
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void updateSimulation();

    // обработка кликов по светофору
    void onTrafficLightClicked(long long id);
    void cycleTrafficLightState(long long id);

private:
    QGraphicsScene *m_scene;
    QMap<int, QGraphicsEllipseItem*> m_nodeItems;
    QMap<int, QGraphicsLineItem*> m_edgeItems;
    QMap<int, Vehicle*> m_vehicles;           // Логика транспортных средств
    QMap<int, VehicleItem*> m_vehicleItems;   // Графические элементы
    QMap<long long, QPointF> m_nodePositions; // кэш позиций узлов в сцене
    QMap<long long, int> m_osmToInternalId;      // Маппинг: OSM ID → внутренний ID
    QMap<long long, QPointF> m_osmNodePositions;
    QMap<int, TrafficLight*> m_trafficLights; // Кэш светофоров
    QMap<int, QGraphicsEllipseItem*> m_trafficLightItems; // view
    QMap<int, TrafficLightController*> m_controllers;  // логика

    QColor colorForState(LightState state);

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

    void loadOSM(const QString &filename);


    // Методы для потоковой обработки
    void startStreamingLoad(const QString &filename);
    void processOsmChunk();

    void setTrafficLightManualMode(long long id, bool manual);

    // --- ПЕРЕМЕННЫЕ ДЛЯ ПОТОКОВОЙ ЗАГРУЗКИ ---

    enum class LoadPhase {
        ParsingNodes,   // Читаем узлы
        ParsingWays,    // Читаем и рисуем дороги
        Finished
    };

    QFile m_osmFile;
    LoadPhase m_currentPhase;
    QXmlStreamReader m_xmlReader;
    bool m_isLoading;

    // Буферы для текущей порции
    QMap<long long, QPointF> m_tempNodes; // Временные узлы текущей порции
    QList<PendingWay> m_tempWays;         // Временные дороги текущей порции
    int m_internalIdCounter;              // Счетчик внутренних ID
};

#endif // SIMULATIONVIEW_H













