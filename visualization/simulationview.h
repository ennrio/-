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
#include <QtConcurrent>
#include <QFutureWatcher>
#include "../constants.h"

struct PendingWay {
    QList<long long> nodeRefs;
    QString highwayType;
    bool isOneWay{false};
};

struct CrossingInfo {
    long long id;             // ID светофора в системе
    QString name;             // Название (например, "Невский / Литейный")
    bool requiresAttention;   // Флаг "Требует внимания"
};


class SimulationView : public QGraphicsView
{
    Q_OBJECT

//METHODS
public:
    SimulationView(QWidget *parent = nullptr);
    ~SimulationView();
    void show();
    void addNode(int id, double x, double y);
    void addEdge(int id, int from, int to, double length, bool bidirectional);
    void addVehicle(int id, const QPointF& startPosition);
    void setVehicleRoute(int vehicleId, const QList<QPointF>& routePoints);


    //условная компиляция
    void setupWindowsSpecific();
    void setupLinuxSpecific();

    void startSimulation();
    void stopSimulation();
    int getActiveVehicleCount() const;
    int getVehicleCount() const;
    double getAverageSpeed() const;

    void setSimulationSpeed(double factor) { m_timeFactor = factor; }
    void drawRoad(long long fromId, long long toId, const QString &type);
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    QPointF convertLatLon(double lat, double lon) const;

    //управление свотофорами
    void setLightMode(LightMode mode);
    void setLightAutoMode();
    void setLightManualOperation();
    void setLightNightMode();

    QMap<long long, CrossingInfo> getTrafficLightsList() const;
    void setTrafficLightAttention(long long id, bool attention);

//MEMBERS
public:

protected:
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void updateSimulation();
    // обработка кликов по светофору
    void onTrafficLightClicked(long long id);
    void cycleTrafficLightState(long long id);
    void onOSMLoadingFinished();
    void spawnVehicle();
    void onRouteCalculationFinished();
    void checkTrafficCongestion();

//METHODS
private:
    QList<QPointF> calculateRouteAsync();
    LightState getTrafficLightStateAtPosition(const QPointF& position, qreal radius);
    void updateVehicleGraphics();
    QFutureWatcher<QList<QPointF>>* m_routeCalculationWatcher;


//MEMBERS
private:
    QGraphicsScene *m_scene;
    QMap<int, QGraphicsEllipseItem*> m_nodeItems;
    QMap<int, QGraphicsLineItem*> m_edgeItems;
    QMap<int, Vehicle*> m_vehicles;           // Логика транспортных средств
    QMap<int, VehicleItem*> m_vehicleItems;   // Графические элементы
    QMap<long long, QPointF> m_nodePositions; // кэш позиций узлов в сцене
    QMap<long long, int> m_osmToInternalId;      // Маппинг: OSM ID → внутренний ID
    QMap<long long, QPointF> m_osmNodePositions;
    QSet<long long> m_mainStreetNodeIds;
    QMap<int, long long> m_trafficLightOsmId;
    QMap<int, TrafficLight*> m_trafficLights; // Кэш светофоров
    QMap<int, QGraphicsEllipseItem*> m_trafficLightItems; // view
    QMap<int, TrafficLightController*> m_controllers;  // логика
    static constexpr qint64 VEHICLE_HIDE_TIMEOUT_MS = 30 * 60 * 1000;

    QColor colorForState(LightState state);

    RoadGraph* m_roadGraph;
    QTimer m_simulationTimer;
    QElapsedTimer m_elapsedTimer;
    double m_lastUpdateTime;
    double m_timeFactor; // Коэффициент скорости симуляции

    void drawRoadGraph();
    QPointF getNodePosition(int nodeId) const;
    QPointF latLonToScene(double lat, double lon) const;

    // Вспомогательные методы парсинга
    void parseOSMFile(const QString &filename);
    void drawOSMRoad(const QList<long long> &nodeRefs, const QString &highwayType);

    void loadOSM(const QString &filename);
    LightMode lm;


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

    // МАШИНЫ
    QTimer m_vehicleSpawnTimer;
    int m_vehicleCounter{0};
    int m_edgeIdCounter{1};

    // Кэш текущего состояния "внимания", чтобы не слать сигналы лишний раз
    QTimer m_congestionCheckTimer;
    QMap<long long, bool> m_currentAttentionState;
signals:
    void osmLoadingFinished();
    void trafficLightStatusChanged(long long tlId, bool requiresAttention);
};

#endif // SIMULATIONVIEW_H













