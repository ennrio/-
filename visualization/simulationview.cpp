#include "simulationview.h"
#include <QDebug>
#include <QPen>
#include <QBrush>
#include <cmath>
#include <QFile>
#include <QXmlStreamReader>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QColor>
#include <QtMath>
#include <QStringView>
#include <QRandomGenerator>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QContextMenuEvent>
#include <qmenu.h>
#include "trafficlightitem.h"




SimulationView::SimulationView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_timeFactor(1.0)
    , m_lastUpdateTime(0)
    , m_isLoading(false)
    , m_currentPhase(LoadPhase::ParsingNodes)
    , m_internalIdCounter(1)
    , m_roadGraph(new RoadGraph())
    , m_accidentManager(nullptr)
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);


    setDragMode(QGraphicsView::ScrollHandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setOptimizationFlags(QGraphicsView::DontSavePainterState);

    // Настройка таймера симуляции
    m_simulationTimer.setInterval(16); // ~60 FPS
    connect(&m_simulationTimer, &QTimer::timeout, this, &SimulationView::updateSimulation);

    // Запускаем таймер для измерения времени
    m_elapsedTimer.start();

    // Настраиваем сцену
    m_scene->setSceneRect(-100, -100, 2000, 2000);
    setSceneRect(-100, -100, 2000, 2000);

    // Таймер для спавна автомобилей
    m_vehicleSpawnTimer.setInterval(3000); // Каждые 3 секунды
    connect(&m_vehicleSpawnTimer, &QTimer::timeout, this, &SimulationView::spawnVehicle);


    m_routeCalculationWatcher = new QFutureWatcher<QList<QPointF>>(this);
    connect(m_routeCalculationWatcher, &QFutureWatcher<QList<QPointF>>::finished,
            this, &SimulationView::onRouteCalculationFinished);


    connect(this, &SimulationView::osmLoadingFinished,
            this, &SimulationView::onOSMLoadingFinished,
            Qt::UniqueConnection);

    m_congestionCheckTimer.setInterval(1000);
    connect(&m_congestionCheckTimer, &QTimer::timeout,
            this, &SimulationView::checkTrafficCongestion);
    
    // Инициализация менеджера ДТП
    m_accidentManager = new AccidentManager(this);
    m_accidentManager->setSimulationView(this);
}

SimulationView::~SimulationView()
{
    delete m_accidentManager;
    delete m_roadGraph;
}

void SimulationView::show()
{
    QGraphicsView::show();
    #if defined(Q_OS_WIN)
        setupWindowsSpecific();
    #elif defined(Q_OS_LINUX)
        setupLinuxSpecific();
    #endif

    if (m_scene) {
        m_scene->update(); // Обновить всю сцену
    }
    this->viewport()->update();
}


void SimulationView::addNode(int id, double x, double y)
{
    // Добавляем в граф
    m_roadGraph->addNode(id, x, y);

    // Создаем графический элемент узла
    QGraphicsEllipseItem* nodeItem = new QGraphicsEllipseItem(-3, -3, 6, 6);
    nodeItem->setPos(x, y);
    nodeItem->setBrush(QBrush(Qt::blue));
    nodeItem->setPen(QPen(Qt::black, 1));
    nodeItem->setZValue(2);

    // Добавляем на сцену и сохраняем
    m_scene->addItem(nodeItem);
    m_nodeItems[id] = nodeItem;
}

void SimulationView::addEdge(int id, int from, int to, double length, bool bidirectional)
{
    // Добавляем в граф
    m_roadGraph->addEdge(id, from, to, length, bidirectional);

    // Получаем позиции узлов
    QPointF fromPos = getNodePosition(from);
    QPointF toPos = getNodePosition(to);

    // Создаем графический элемент ребра
    QGraphicsLineItem* edgeItem = new QGraphicsLineItem(
        fromPos.x(), fromPos.y(), toPos.x(), toPos.y());

    edgeItem->setPen(QPen(Qt::gray, 2));
    edgeItem->setZValue(1);

    // Добавляем на сцену и сохраняем
    m_scene->addItem(edgeItem);
    m_edgeItems[id] = edgeItem;
}

void SimulationView::addVehicle(int id, const QPointF& startPosition)
{
    // Создаем логику транспортного средства
    Vehicle* vehicle = new Vehicle(startPosition, id, this);
    m_vehicles[id] = vehicle;

    // Создаем графический элемент
    VehicleItem* vehicleItem = new VehicleItem(vehicle);
    vehicleItem->setZValue(3); // Транспорт должен быть поверх дорог

    // Добавляем на сцену
    m_scene->addItem(vehicleItem);
    m_vehicleItems[id] = vehicleItem;

    qDebug() << "Vehicle" << id << "added at position" << startPosition;
}

void SimulationView::setVehicleRoute(int vehicleId, const QList<QPointF> &routePoints)
{
    if (!m_vehicles.contains(vehicleId)) {
        qWarning() << "Vehicle" << vehicleId << "not found";
        return;
    }

    if (routePoints.size() < 2) {
        qWarning() << "Route needs at least 2 points";
        return;
    }

    // Устанавливаем маршрут транспортному средству
    m_vehicles[vehicleId]->setRoute(routePoints);
    m_vehicles[vehicleId]->setSpeed(11.0 + QRandomGenerator::global()->bounded(6));

    qDebug() << "Route set for vehicle" << vehicleId
             << "with" << routePoints.size() << "points";
}

void SimulationView::setupWindowsSpecific()
{
    this->loadOSM("C:\\Users\\egor\\all\\learning\\6sem\\asud\\graphs\\center.osm");
}

void SimulationView::setupLinuxSpecific()
{
    this->loadOSM("/home/egor/all/study/6sem/ASUDD/center.osm");
}

QPointF SimulationView::getNodePosition(int nodeId) const
{
    // Используем метод из RoadGraph для получения реальной позиции узла
    return m_roadGraph->getNodePosition(nodeId);
}

void SimulationView::startSimulation()
{
    m_lastUpdateTime = m_elapsedTimer.elapsed() / 1000.0;
    m_simulationTimer.start();
    m_vehicleSpawnTimer.start();
    m_congestionCheckTimer.start();
    qDebug() << "Simulation started with vehicle spawning";
}

void SimulationView::stopSimulation()
{
    m_simulationTimer.stop();
    m_vehicleSpawnTimer.stop();
    qDebug() << "Simulation stopped";
}

int SimulationView::getActiveVehicleCount() const
{
    int counter = 0;
    for(auto it:m_vehicles){
        if(it->isActive()){
            counter++;
        }
    }
    return counter; // TODO можно оптимизировать, если правильно упорядочивать массив
}

int SimulationView::getVehicleCount() const
{
    return m_vehicles.count();
}

double SimulationView::getAverageSpeed() const
{
    if (m_vehicles.isEmpty()) {
        return 0.0;
    }

    double totalSpeed = 0.0;
    int activeCount = 0;

    for (const Vehicle* vehicle : m_vehicles) {
        if (vehicle && vehicle->isActive()) {
            totalSpeed += vehicle->speed();
            ++activeCount;
        }
    }

    return (activeCount > 0) ? (totalSpeed*3600/1000 / activeCount) : 0.0;
}



void SimulationView::drawRoad(long long fromId, long long toId, const QString &type)
{
    if (!m_nodePositions.contains(fromId) || !m_nodePositions.contains(toId))
        return;

    QGraphicsLineItem *line = m_scene->addLine(
        QLineF(m_nodePositions[fromId], m_nodePositions[toId])
        );

    // Цвет по типу дороги
    if (type == "motorway") line->setPen(QPen(Qt::darkBlue, 3));
    else if (type == "trunk") line->setPen(QPen(Qt::blue, 2.5));
    else if (type == "primary") line->setPen(QPen(Qt::green, 2));
    else line->setPen(QPen(Qt::gray, 1.5));
}

double SimulationView::calculateDistance(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000; // радиус Земли в метрах
    double dLat = qDegreesToRadians(lat2 - lat1);
    double dLon = qDegreesToRadians(lon2 - lon1);
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(qDegreesToRadians(lat1)) * cos(qDegreesToRadians(lat2)) *
                   sin(dLon/2) * sin(dLon/2);
    return R * 2 * atan2(sqrt(a), sqrt(1-a));
}

void SimulationView::wheelEvent(QWheelEvent *event)
{
    double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
    event->accept();
}

void SimulationView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    QGraphicsItem *item = itemAt(pos);

    long long clickedTlId = -1;

    // Ищем, кликнули ли мы по светофору (или его дочернему элементу)
    while (item) {
        // Проверяем наш маппинг элементов светофоров
        auto it = m_trafficLightItems.begin();
        while (it != m_trafficLightItems.end()) {
            if (it.value() == item || item->parentItem() == it.value()) {
                clickedTlId = it.key();
                break;
            }
            ++it;
        }
        if (clickedTlId != -1) break;
        item = item->parentItem();
    }

    if (clickedTlId != -1) {
        QMenu menu(this);
        menu.setStyleSheet("QMenu { background-color: #333; color: white; border: 1px solid #555; }");

        QAction* actionAlert = menu.addAction(" Добавить в список 'Требует внимания'");
        QAction* actionReset = menu.addAction(" Сбросить статус (Норма)");
        menu.addSeparator();
        QAction* actionToggle = menu.addAction(" Переключить вручную");

        QAction* selectedAction = menu.exec(event->globalPos());

        if (selectedAction) {
            if (selectedAction == actionAlert) {
                // Принудительно ставим статус "Внимание"
                m_currentAttentionState[clickedTlId] = true;
                emit trafficLightStatusChanged(clickedTlId, true);

                // Визуально помечаем
                if (m_trafficLightItems.contains(clickedTlId))
                    m_trafficLightItems[clickedTlId]->setPen(QPen(Qt::red, 4));

            } else if (selectedAction == actionReset) {
                // Сбрасываем статус
                m_currentAttentionState[clickedTlId] = false;
                emit trafficLightStatusChanged(clickedTlId, false);

                if (m_trafficLightItems.contains(clickedTlId))
                    m_trafficLightItems[clickedTlId]->setPen(QPen(Qt::black, 2));

            } else if (selectedAction == actionToggle) {
                // Ручное переключение фаз (ваша существующая логика)
                cycleTrafficLightState(clickedTlId);
            }
        }
    } else {
        // Если клик не по светофору, стандартное поведение
        QGraphicsView::contextMenuEvent(event);
    }
}

void SimulationView::updateSimulation()
{
    double currentTime = m_elapsedTimer.elapsed() / 1000.0;
    double deltaTime = (currentTime - m_lastUpdateTime) * m_timeFactor;
    m_lastUpdateTime = currentTime;

    // Текущее время в миллисекундах для таймаута
    qint64 currentMsecs = QDateTime::currentMSecsSinceEpoch();

    for (auto it = m_vehicles.begin(); it != m_vehicles.end(); ++it) {
        Vehicle* vehicle = it.value();
        int id = vehicle->id();

        // Обновляем физику только активных машин
        if (vehicle->isActive()) {
            // Проверяем, не влияет ли ДТП на позицию автомобиля
            if (m_accidentManager && m_accidentManager->isPositionAffected(vehicle->position())) {
                // Замедляем автомобиль возле ДТП
                vehicle->setMaxSpeed(2.0); // Очень медленно объезжаем
            } else {
                vehicle->setMaxSpeed(14.0 + QRandomGenerator::global()->bounded(4));
            }
            
            vehicle->update(deltaTime);
        }

        // Обновляем графику
        if (m_vehicleItems.contains(id)) {
            VehicleItem* item = m_vehicleItems[id];

            if (vehicle->isRouteFinished()) {
                // Проверяем, является ли парковка неправильной
                bool isWrongParked = m_wrongParkedVehicles.contains(id);
                
                // Если машина только что завершила маршрут и была кандидатом на неправильную парковку
                if (!isWrongParked && !m_wrongParkedVehicles.contains(id) && 
                    vehicle->property("wrongParkingCandidate").toBool() && m_wrongParkingEnabled) {
                    // Добавляем в список неправильно припаркованных
                    m_wrongParkedVehicles.append(id);
                    isWrongParked = true;
                }
                
                if (isWrongParked) {
                    // Неправильная парковка - красный маркер
                    item->setColor(Qt::red);
                } else {
                    // Правильная парковка - синий цвет
                    item->setColor(Qt::blue);
                }

                if (currentMsecs - vehicle->finishedTimestamp() > VEHICLE_HIDE_TIMEOUT_MS) {
                    item->hide();
                } else {
                    item->show();
                    item->setPos(vehicle->position());
                }
            } else {
                item->setColor(QColor(128, 0, 128));
                item->show();
                item->setPos(vehicle->position());
            }
        }
    }

    // Обновляем маркеры ДТП на карте
    if (m_accidentManager) {
        updateAccidentMarkers();
    }
    
    // Обновляем маркеры неправильной парковки
    if (m_wrongParkingEnabled) {
        updateWrongParkingMarkers();
    }

    m_scene->update();
}

void SimulationView::onTrafficLightClicked(long long id)
{
    qDebug() << "Traffic light clicked:" << id;
    cycleTrafficLightState(id);
}

void SimulationView::cycleTrafficLightState(long long id)
{
    if (!m_controllers.contains(id) || !m_trafficLights.contains(id)) {
        return;
    }

    setTrafficLightManualMode(id, true);

    QTimer::singleShot(30000, this, [this, id]() {
        setTrafficLightManualMode(id, false);
    });

    auto* controller = m_controllers[id];
    auto* trafficLight = m_trafficLights[id];

    // Получаем текущее состояние
    LightState currentState = trafficLight->state();

    // Циклически переключаем: Red -> Green -> Yellow -> Red
    LightState nextState;
    switch (currentState) {
    case LightState::Red:
        nextState = LightState::Green;
        break;
    case LightState::Green:
        nextState = LightState::Yellow;
        break;
    case LightState::Yellow:
        nextState = LightState::Red;
        break;
    default:
        nextState = LightState::Red;
    }

    // Принудительно устанавливаем состояние на 30 секунд
    // После этого вернётся автоматический режим
    controller->forceState(nextState, 15000);

    qDebug() << "Traffic light" << id
             << "manually changed to" << static_cast<int>(nextState)
             << "(will auto-resume in 15s)";
}

void SimulationView::onOSMLoadingFinished()
{
    qDebug() << "=== OSM Loading Finished ===";
    qDebug() << "RoadGraph Nodes:" << m_roadGraph->nodeCount();
    qDebug() << "RoadGraph Edges:" << m_roadGraph->edgeCount();
    qDebug() << "OSM→Internal mappings:" << m_osmToInternalId.size();

    int nodesWithNeighbors = 0;
    int nodesWithoutNeighbors = 0;
    auto nodes = m_roadGraph->getNodes().keys();

    for (int nodeId : nodes) {
        int neighborCount = m_roadGraph->getNeighbors(nodeId).size();
        if (neighborCount > 0) {
            nodesWithNeighbors++;
        } else {
            nodesWithoutNeighbors++;
        }
    }

    qDebug() << "Nodes with neighbors:" << nodesWithNeighbors;
    qDebug() << "Nodes WITHOUT neighbors:" << nodesWithoutNeighbors;  // ❌ Если > 0 — проблема!

    //  Проверка: первые 10 узлов и их соседи
    for (int i = 0; i < qMin(10, nodes.size()); i++) {
        int nodeId = nodes[i];
        auto neighbors = m_roadGraph->getNeighbors(nodeId);
        qDebug() << "Node" << nodeId << "neighbors:" << neighbors;
    }

    QRectF sceneRect = m_scene->itemsBoundingRect();
    sceneRect.adjust(-100, -100, 100, 100);
    m_scene->setSceneRect(sceneRect);
    setSceneRect(sceneRect);

    startSimulation();
}

LightState SimulationView::getTrafficLightStateAtPosition(const QPointF &position, qreal radius)
{
    for (auto* tl : m_trafficLights) {
        qreal distance = QLineF(position, tl->position()).length();
        if (distance <= radius) {
            return tl->state();
        }
    }
    return LightState::Green; // По умолчанию зелёный
}

void SimulationView::spawnVehicle()
{
    if (m_isLoading || m_vehicles.size() >= 5000) return;
    if (m_roadGraph->nodeCount() < 2 || m_roadGraph->edgeCount() < 1) return;

    // Если уже идёт расчёт — пропускаем этот спавн
    if (m_routeCalculationWatcher->isRunning()) {
        return;
    }

    auto future = QtConcurrent::run([this]() {
        return calculateRouteAsync();
    });

    m_routeCalculationWatcher->setFuture(future);
}

void SimulationView::onRouteCalculationFinished()
{
    QList<QPointF> routePoints = m_routeCalculationWatcher->result();


    if (routePoints.size() < 2) {
        // qDebug() << "Route calculation failed";
        return;
    }

    int vehicleId = ++m_vehicleCounter;
    addVehicle(vehicleId, routePoints.first());
    setVehicleRoute(vehicleId, routePoints);

    if (Vehicle* v = m_vehicles.value(vehicleId)) {
        v->setTrafficLightChecker([this](const QPointF& pos, qreal radius) {
            return getTrafficLightStateAtPosition(pos, radius);
        });
        v->setTrafficLightAwareness(true);
        v->setMaxSpeed(14.0 + QRandomGenerator::global()->bounded(4));

        v->setMaxSpeed(14.0 + QRandomGenerator::global()->bounded(4));
        v->setAcceleration(3.5);
        v->setDeceleration(5.0);
        
        // Проверяем вероятность неправильной парковки при создании машины
        if (m_wrongParkingEnabled && !v->isRouteFinished()) {
            double randomValue = QRandomGenerator::global()->generateDouble();
            if (randomValue < m_wrongParkingProbability) {
                // Помечаем машину как кандидат на неправильную парковку
                // Фактическая парковка будет определена когда машина завершит маршрут
                v->setProperty("wrongParkingCandidate", true);
            }
        }
    }

    qDebug() << "Vehicle" << vehicleId << "spawned with route of"
             << routePoints.size() << "points";
}

void SimulationView::checkTrafficCongestion()
{
    const qreal checkRadius = 60.0;
    for (auto it = m_trafficLights.begin(); it != m_trafficLights.end(); ++it) {
        long long tlId = it.key();
        TrafficLight* tl = it.value();
        QPointF tlPos = tl->position();
        int vehicleCount = 0;

        // Считаем активные машины в радиусе перед светофором
        for (auto vIt = m_vehicles.begin(); vIt != m_vehicles.end(); ++vIt) {
            Vehicle* v = vIt.value();
            if (v && v->isActive()) {
                QPointF vPos = v->position();
                if (QLineF(tlPos, vPos).length() < checkRadius) {
                    vehicleCount++;
                }
            }
        }
        bool needsAttention = (vehicleCount >= 3); //TODO колличество

        if (m_currentAttentionState[tlId] != needsAttention) {
            m_currentAttentionState[tlId] = needsAttention;

            // Визуальная подсветка на карте
            if (m_trafficLightItems.contains(tlId)) {
                QGraphicsEllipseItem* item = m_trafficLightItems[tlId];
                if (needsAttention) {
                    item->setPen(QPen(Qt::red, 4));
                } else {
                    item->setPen(QPen(Qt::black, 2));
                }
            }

            emit trafficLightStatusChanged(tlId, needsAttention);
        }
    }
}


QList<QPointF> SimulationView::calculateRouteAsync()
{

    // Получаем только узлы с соседями
    QList<int> connectedIds;
    auto allNodes = m_roadGraph->getNodes().keys();
    for (int nodeId : allNodes) {
        if (!m_roadGraph->getNeighbors(nodeId).isEmpty()) {
            connectedIds.append(nodeId);
        }
    }

    if (connectedIds.size() < 2) {
        return QList<QPointF>();  // Пустой маршрут = ошибка
    }

    // Пробуем найти маршрут (макс. 3 попытки)
    for (int attempt = 0; attempt < 3; ++attempt) {
        int start = connectedIds[QRandomGenerator::global()->bounded(connectedIds.size())];
        int end = connectedIds[QRandomGenerator::global()->bounded(connectedIds.size())];
        if (start == end) continue;

        QList<int> path = m_roadGraph->findRoute(start, end);
        if (path.isEmpty()) continue;

        // Конвертируем путь в координаты
        QList<QPointF> routePoints;
        for (int nodeId : path) {
            //  Доступ к m_osmToInternalId и m_osmNodePositions — только чтение, это безопасно
            for (auto it = m_osmToInternalId.begin(); it != m_osmToInternalId.end(); ++it) {
                if (it.value() == nodeId && m_osmNodePositions.contains(it.key())) {
                    routePoints.append(m_osmNodePositions[it.key()]);
                    break;
                }
            }
        }

        if (routePoints.size() >= 2) {
            return routePoints;
        }
    }

    return QList<QPointF>();
}

QColor SimulationView::colorForState(LightState state)
{
    switch (state) {
    case LightState::Green: return Qt::green;
    case LightState::Yellow: return Qt::yellow;
    case LightState::Red: return Qt::red;
    default: return Qt::gray;
    }
}

void SimulationView::updateVehicleGraphics()
{
    for (auto it = m_vehicles.begin(); it != m_vehicles.end(); ++it) {
        int id = it.value()->id();
        if (m_vehicleItems.contains(id)) {
            m_vehicleItems[id]->setPos(it.value()->position());
        }
    }
}

QPointF SimulationView::latLonToScene(double lat, double lon) const
{
    const double centerLat = 59.9386;
    const double centerLon = 30.3141;
    const double metersPerPixel = 1;

    // Расстояние в метрах от центра
    double dx = (lon - centerLon) * 111320.0 * cos(qDegreesToRadians(centerLat));
    double dy = (lat - centerLat) * 110540.0;

    // Конвертируем метры в пиксели + центрируем сцену
    return QPointF(dx / metersPerPixel, -dy / metersPerPixel);
}

void SimulationView::parseOSMFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open OSM file:" << filename << file.errorString();
        return;
    }

    QXmlStreamReader xml(&file);
    QMap<long long, QPointF> tempNodes; // Временное хранилище узлов
    QList<QPair<QList<long long>, QString>> waysToDraw; // (список узлов, тип дороги)

    // Этап 1: Парсим все узлы
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == "node") {
            long long id = xml.attributes().value("id").toLongLong();
            double lat = xml.attributes().value("lat").toDouble();
            double lon = xml.attributes().value("lon").toDouble();

            // Проверяем теги узла на предмет светофора
            bool isTrafficLight = false;
            QString direction;
            bool isPedestrian = false;

            while (!(xml.isEndElement() && xml.name() == "node")) {
                xml.readNext();
                if (xml.isStartElement() && xml.name() == "tag") {
                    QString key = xml.attributes().value("k").toString();
                    QString value = xml.attributes().value("v").toString();

                    if (key == "highway" && value == "traffic_signals") {
                        isTrafficLight = true;
                    } else if (key == "traffic_signals:direction") {
                        direction = value;
                    } else if (key == "traffic_signals" && value == "pedestrian") {
                        isPedestrian = true;
                    }
                }
            }

            tempNodes[id] = QPointF(lat, lon);

            // TODO Если это светофор — сохраняем отдельно
            if (isTrafficLight) {
                QPointF scenePos = latLonToScene(lat, lon);
                auto* tl = new TrafficLight(id, scenePos, direction, isPedestrian, this);
                m_trafficLights[tl->id()] = tl;

                auto* light_controller = new TrafficLightController(tl, this);
                light_controller->setStandardCycle(30000, 5000, 25000); // зелёный/жёлтый/красный
                m_controllers[tl->id()] = light_controller;

                auto* item = new QGraphicsEllipseItem(-6, -6, 12, 12);
                item->setPos(scenePos);
                item->setBrush(QBrush(colorForState(tl->state()))); // начальный цвет
                item->setPen(QPen(Qt::black, 1));
                item->setZValue(10); // поверх дорог
                m_scene->addItem(item);
                m_trafficLightItems[tl->id()] = item;  // ← Сохраняем указатель на item

                // Метка 🚦 (опционально)
                auto* label = new QGraphicsTextItem("🚦");
                label->setPos(scenePos.x() + 8, scenePos.y() - 8);
                label->setZValue(11);
                m_scene->addItem(label);

                // === СВЯЗЬ: Модель → View ===
                connect(tl, &TrafficLight::stateChanged,
                        this, [this](int id, LightState state) {
                            if (m_trafficLightItems.contains(id)) {
                                m_trafficLightItems[id]->setBrush(QBrush(colorForState(state)));
                            }
                        });
            }

            // Пропускаем содержимое узла
            while (!(xml.isEndElement() && xml.name() == "node")) {
                xml.readNext();
            }
        }
    }

    qDebug() << "[DEBUG] Temp nodes loaded:" << tempNodes.size();
    qDebug() << "[DEBUG] Ways to draw:" << waysToDraw.size();
    if (!waysToDraw.isEmpty()) {
        qDebug() << "[DEBUG] First way has" << waysToDraw.first().first.size() << "node refs";
    }

    if (xml.hasError()) {
        qWarning() << "XML error during node parsing:" << xml.errorString();
        return;
    }

    file.seek(0);
    xml.clear();
    xml.setDevice(&file);

    // Этап 2: Парсим пути (дороги)
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == "way") {
            //long long wayId = xml.attributes().value("id").toLongLong();
            QList<long long> nodeRefs;
            QString highwayType;
            bool isRoad = false;
            bool isOneWay = false;

            while (!(xml.isEndElement() && xml.name() == "way")) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == "nd") {
                        nodeRefs.append(xml.attributes().value("ref").toLongLong());
                    } else if (xml.name() == "tag") {
                        QString key = xml.attributes().value("k").toString();
                        QString value = xml.attributes().value("v").toString();
                        if (key == "highway") {
                            highwayType = value;
                            // Фильтруем только автомобильные дороги
                            if (!value.contains("footway") &&
                                !value.contains("path") &&
                                !value.contains("cycleway") &&
                                !value.contains("steps")) {
                                isRoad = true;
                            }
                        }

                        //  Проверка oneway
                        else if (key == "oneway" && (value == "yes" || value == "1" || value == "true")) {
                            isOneWay = true;
                        }
                        else if (key == "oneway" && value == "-1") {
                            isOneWay = true;
                            std::reverse(nodeRefs.begin(), nodeRefs.end());
                        }
                    }
                }
            }

            if (isRoad && nodeRefs.size() >= 2) {
                waysToDraw.append(qMakePair(nodeRefs, highwayType));
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML error during way parsing:" << xml.errorString();
        return;
    }

        // Этап 3: Добавляем узлы в граф и сцену
        int internalIdCounter = 1;
        int nodesAdded = 0;
        int edgeId = 1;  //  ОБЪЯВЛЯЕМ edgeId ЗДЕСЬ

        for (auto it = tempNodes.begin(); it != tempNodes.end(); ++it) {
            long long osmId = it.key();
            double lat = it.value().x();
            double lon = it.value().y();

            QPointF scenePos = latLonToScene(lat, lon);
            m_osmNodePositions[osmId] = scenePos;
            m_osmToInternalId[osmId] = internalIdCounter;

            // Проверяем, используется ли узел в дорогах
            bool usedInWay = false;
            for (const auto &way : waysToDraw) {
                if (way.first.contains(osmId)) {
                    usedInWay = true;
                    break;
                }
            }

            if (usedInWay) {
                //  Добавляем узел в граф
                m_roadGraph->addNode(internalIdCounter, scenePos.x(), scenePos.y());
                nodesAdded++;
            }
            internalIdCounter++;
        }

        //  Теперь добавляем рёбра (edgeId уже объявлен)
        for (const auto &way : waysToDraw) {
            const QList<long long> &nodeRefs = way.first;

            for (int i = 0; i < nodeRefs.size() - 1; ++i) {
                long long fromOsmId = nodeRefs[i];
                long long toOsmId = nodeRefs[i + 1];

                if (m_osmToInternalId.contains(fromOsmId) && m_osmToInternalId.contains(toOsmId)) {
                    int fromInternal = m_osmToInternalId[fromOsmId];
                    int toInternal = m_osmToInternalId[toOsmId];

                    QPointF fromPos = m_osmNodePositions[fromOsmId];
                    QPointF toPos = m_osmNodePositions[toOsmId];
                    double length = QLineF(fromPos, toPos).length();

                    m_roadGraph->addEdge(edgeId++, fromInternal, toInternal, length, true);
                }
            }
        }

        internalIdCounter++;




    if (!m_osmNodePositions.isEmpty()) {
        double minX = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        for (QPointF& pos : m_osmNodePositions) {
            minX = qMin(minX, pos.x());
            maxX = qMax(maxX, pos.x());
        //qDebug() << "Centered on:" << QPointF((minX + maxX) / 2.0, (minY + maxY) / 2.0);
        }
    }

    // Этап 4: Рисуем дороги
    for (const auto &way : waysToDraw) {
        const QList<long long> &nodeRefs = way.first;
        const QString &highwayType = way.second;

        for (int i = 0; i < nodeRefs.size() - 1; ++i) {
            long long fromOsmId = nodeRefs[i];
            long long toOsmId = nodeRefs[i + 1];

            if (m_osmToInternalId.contains(fromOsmId) && m_osmToInternalId.contains(toOsmId)) {
                int fromId = m_osmToInternalId[fromOsmId];
                int toId = m_osmToInternalId[toOsmId];

                // Рассчитываем длину сегмента
                QPointF fromPos = m_osmNodePositions[fromOsmId];
                QPointF toPos = m_osmNodePositions[toOsmId];
                double length = qSqrt(qPow(toPos.x() - fromPos.x(), 2) +
                                      qPow(toPos.y() - fromPos.y(), 2));

                // Добавляем ребро в граф
                addEdge(edgeId++, fromId, toId, length, true);
            }
        }

        // Визуализируем всю дорогу целиком (для красоты)
        drawOSMRoad(nodeRefs, highwayType);


    }
}

void SimulationView::drawOSMRoad(const QList<long long> &nodeRefs, const QString &highwayType)
{
    if (nodeRefs.size() < 2) return;

    // Создаём polyline для всей дороги
    QPolygonF polygon;
    for (long long nodeId : nodeRefs) {
        if (m_osmNodePositions.contains(nodeId)) {
            polygon.append(m_osmNodePositions[nodeId]);
        }
    }

    if (polygon.size() < 2) return;

    QGraphicsPathItem *pathItem = new QGraphicsPathItem();
    QPainterPath path;
    path.addPolygon(polygon);
    pathItem->setPath(path);

    // Цвет и толщина по типу дороги
    QPen pen;
    if (highwayType == "motorway" || highwayType == "trunk") {
        pen = QPen(QColor(0, 64, 128), 4); // Тёмно-синий
    } else if (highwayType == "primary") {
        pen = QPen(QColor(0, 128, 0), 3);  // Зелёный
    } else if (highwayType == "secondary") {
        pen = QPen(QColor(128, 128, 0), 2.5); // Жёлтый
    } else if (highwayType == "tertiary") {
        pen = QPen(QColor(192, 192, 192), 2); // Серый
    } else {
        pen = QPen(QColor(220, 220, 220), 1.5); // Светло-серый
    }

    pathItem->setPen(pen);
    pathItem->setZValue(0);
    m_scene->addItem(pathItem);
}

void SimulationView::startStreamingLoad(const QString &filename)
{
    qDebug() << Q_FUNC_INFO;
    if (m_isLoading) return;

    m_osmFile.setFileName(filename);
    if (!m_osmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Не удалось открыть файл:" << filename;
        return;
    }

    m_xmlReader.setDevice(&m_osmFile);
    m_isLoading = true;
    m_currentPhase = LoadPhase::ParsingNodes; // Стартуем с фазы узлов
    m_internalIdCounter = 1;

    m_tempNodes.clear();
    m_tempWays.clear();

    // Очищаем сцену
    m_scene->clear();
    m_nodeItems.clear();
    m_edgeItems.clear();
    m_osmNodePositions.clear();
    m_osmToInternalId.clear();
    m_roadGraph->clear();

    m_internalIdCounter = 1;
    m_edgeIdCounter = 1;
    qDebug() << "Начало потоковой загрузки OSM (Фаза: Узлы)...";

    QTimer::singleShot(0, this, &SimulationView::processOsmChunk);
}

void SimulationView::processOsmChunk()
{
    if (!m_isLoading || !m_osmFile.isOpen()) {
        return;
    }

    const int ELEMENTS_PER_CHUNK = 1000;
    int processedCount = 0;
    int nodesFoundInChunk = 0;
    int waysFoundInChunk = 0;
    int waysDrawnInChunk = 0;
    int trafficLightsFound = 0;

    // --- ФАЗА 1: Чтение узлов + Светофоры ---
    if (m_currentPhase == LoadPhase::ParsingNodes) {
        while (!m_xmlReader.atEnd() && processedCount < ELEMENTS_PER_CHUNK) {
            m_xmlReader.readNext();

            if (m_xmlReader.isStartElement()) {
                QStringView name = m_xmlReader.name();

                if (name == QLatin1String("node")) {
                    QXmlStreamAttributes attrs = m_xmlReader.attributes();
                    long long id = attrs.value("id").toLongLong();
                    double lat = attrs.value("lat").toDouble();
                    double lon = attrs.value("lon").toDouble();

                    // Переменные для светофора
                    bool isTrafficLight = false;
                    QString direction;
                    bool isPedestrian = false;

                    // Читаем внутренности node, ищем теги
                    while (!(m_xmlReader.isEndElement() && m_xmlReader.name() == QLatin1String("node"))) {
                        m_xmlReader.readNext();
                        if (m_xmlReader.isStartElement() && m_xmlReader.name() == QLatin1String("tag")) {
                            QString key = m_xmlReader.attributes().value("k").toString();
                            QString value = m_xmlReader.attributes().value("v").toString();

                            if (key == "highway" && value == "traffic_signals") {
                                isTrafficLight = true;
                            } else if (key == "traffic_signals:direction") {
                                direction = value;
                            } else if (key == "traffic_signals" && value == "pedestrian") {
                                isPedestrian = true;
                            }
                        }
                    }

                    QPointF scenePos = latLonToScene(lat, lon);

                    if (!m_osmNodePositions.contains(id)) {
                        m_osmNodePositions[id] = scenePos;
                        m_osmToInternalId[id] = m_internalIdCounter;
                        m_roadGraph->addNode(m_internalIdCounter, scenePos.x(), scenePos.y());

                        // === ОТРИСОВКА СВЕТОФОРА ===
                        if (isTrafficLight) {
                            m_nodesWithTrafficLights.insert(id);  // Сохраняем узел со светофором
                            
                            auto* tl = new TrafficLight(id, scenePos, direction, isPedestrian, this);
                            m_trafficLights[tl->id()] = tl;
                            m_trafficLightOsmId[tl->id()] = id;

                            // Создаём контроллер с дефолтным циклом
                            auto* controller = new TrafficLightController(tl, this);
                            controller->setStandardCycle(30000, 5000, 25000);
                            m_controllers[tl->id()] = controller;

                            // Визуализация
                            auto* item = new TrafficLightItem(
                                tl->id(),
                                [this](long long id) {
                                    onTrafficLightClicked(id);  //  Вызов метода SimulationView
                                },
                                nullptr  // родитель будет сцена при addItem
                                );
                            item->setPos(scenePos);
                            item->setBrush(QBrush(colorForState(tl->state())));
                            item->setPen(QPen(Qt::black, 2));
                            item->setZValue(10);
                            m_scene->addItem(item);
                            m_trafficLightItems[tl->id()] = item;

                            // ← Ключевая связь: сигнал → обновление UI
                            connect(tl, &TrafficLight::stateChanged,
                                    this, [this](int id, LightState state) {
                                        if (m_trafficLightItems.contains(id)) {
                                            m_trafficLightItems[id]->setBrush(QBrush(colorForState(state)));
                                        }
                                    });
                        }
                        m_internalIdCounter++;
                    }
                    nodesFoundInChunk++;
                    processedCount++;
                }
                else if (name == QLatin1String("way")) {
                    // qDebug() << "[INFO] Встречен первый <way>. Узлов:" << m_osmNodePositions.size() << "Светофоров:" << m_trafficLights.size();
                    m_currentPhase = LoadPhase::ParsingWays;
                    break;
                }
            }
        }

        if (nodesFoundInChunk > 0 || trafficLightsFound > 0) {
            // qDebug() << "[DEBUG] Фаза 1: Узлов:" << nodesFoundInChunk << ", Светофоров найдено:" << trafficLightsFound;
        }

        if (m_currentPhase == LoadPhase::ParsingNodes && !m_xmlReader.atEnd()) {
            QTimer::singleShot(0, this, &SimulationView::processOsmChunk);
            return;
        }
    }

    // --- ФАЗА 2: Чтение и отрисовка дорог ---
    if (m_currentPhase == LoadPhase::ParsingWays) {
        while (!m_xmlReader.atEnd() && processedCount < ELEMENTS_PER_CHUNK) {
            m_xmlReader.readNext();

            if (m_xmlReader.isStartElement() && m_xmlReader.name() == QLatin1String("way")) {
                QList<long long> nodeRefs;
                QString highwayType;
                bool isRoad = false;
                bool isOneWay = false;
                QString wayName;

                while (!(m_xmlReader.isEndElement() && m_xmlReader.name() == QLatin1String("way"))) {
                    m_xmlReader.readNext();
                    if (m_xmlReader.isStartElement()) {
                        if (m_xmlReader.name() == QLatin1String("nd")) {
                            nodeRefs.append(m_xmlReader.attributes().value("ref").toLongLong());
                        } else if (m_xmlReader.name() == QLatin1String("tag")) {
                            QString key = m_xmlReader.attributes().value("k").toString();
                            QString value = m_xmlReader.attributes().value("v").toString();
                            if (key == "highway") {
                                highwayType = value;
                                if (!value.contains("footway") && !value.contains("path") &&
                                    !value.contains("cycleway") && !value.contains("steps") &&
                                    !value.contains("bridleway") && !value.contains("service")) {
                                    isRoad = true;
                                }
                            }

                            else if (key == "name") {
                                wayName = value;
                            }

                            //  ПРОВЕРКА НА ОДНОСТОРОННЕЕ ДВИЖЕНИЕ
                            else if (key == "oneway") {
                                if (value == "yes" || value == "1" || value == "true") {
                                    isOneWay = true;
                                } else if (value == "-1") {
                                    isOneWay = true;
                                    std::reverse(nodeRefs.begin(), nodeRefs.end());
                                }
                            }
                        }
                    }
                }

                //парсинг для ночного режима сценарий из ПДД (п. 13.3 ПДД РФ) //TODO config_file
                bool isMainStreet = (wayName == "Невский проспект" || wayName == "Nevsky Prospekt" ||
                                     wayName == "Московский проспект" || wayName == "Moskovsky Prospekt");
                if (isMainStreet) {
                    for (long long nodeId : nodeRefs) {
                        m_mainStreetNodeIds.insert(nodeId);
                    }
                }


                if (isRoad && nodeRefs.size() >= 2) {

                    PendingWay pending;
                    pending.nodeRefs = nodeRefs;
                    pending.highwayType = highwayType;
                    pending.name = wayName;  // Сохраняем имя дороги
                    pending.isOneWay = isOneWay;
                    m_tempWays.append(pending);
                    waysFoundInChunk++;

                    bool allNodesLoaded = true;
                    for (long long ref : nodeRefs) {
                        if (!m_osmNodePositions.contains(ref)) {
                            allNodesLoaded = false;
                            break;
                        }
                    }

                    if (allNodesLoaded) {
                        drawOSMRoad(nodeRefs, highwayType);

                        //  ДОБАВЛЯЕМ РЁБРА В ROADGRAPH
                        for (int i = 0; i < nodeRefs.size() - 1; ++i) {
                            long long fromOsm = nodeRefs[i];
                            long long toOsm = nodeRefs[i+1];

                            if (m_osmToInternalId.contains(fromOsm) && m_osmToInternalId.contains(toOsm)) {
                                int fromInternal = m_osmToInternalId[fromOsm];
                                int toInternal = m_osmToInternalId[toOsm];

                                //  Проверка: не добавлено ли уже
                                auto existingNeighbors = m_roadGraph->getNeighbors(fromInternal);
                                if (!existingNeighbors.contains(toInternal)) {
                                    m_roadGraph->addEdge(
                                        m_edgeIdCounter++,
                                        fromInternal,
                                        toInternal,
                                        1.0,
                                        !isOneWay
                                        );
                                }
                            }
                        }
                        waysDrawnInChunk++;
                    }
                }
                processedCount++;
            }
        }

        if (waysDrawnInChunk > 0) {
            viewport()->update();
        }
    }

    // --- ФИНАЛ ---
    if (m_xmlReader.atEnd()) {
        m_isLoading = false;
        m_osmFile.close();
        qDebug() << "[DONE] Загрузка завершена. Узлов:" << m_osmNodePositions.size()
                 << ", Дорог:" << m_edgeItems.size()
                 << ", Светофоров:" << m_trafficLights.size();

        qDebug() << "RoadGraph status before signal:"
                 << "Nodes:" << m_roadGraph->nodeCount()
                 << "Edges:" << m_roadGraph->edgeCount();
        emit osmLoadingFinished();
        if (!m_osmNodePositions.isEmpty()) {
            fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        }
    } else {
        QTimer::singleShot(0, this, &SimulationView::processOsmChunk);
    }
}

void SimulationView::setTrafficLightManualMode(long long id, bool manual)
{
    if (!m_trafficLightItems.contains(id)) {
        return;
    }

    auto* item = m_trafficLightItems[id];

    if (manual) {
        // Жёлтая обводка = ручной режим
        item->setPen(QPen(Qt::yellow, 3));
    } else {
        // Чёрная обводка = автоматический режим
        item->setPen(QPen(Qt::black, 2));
    }
}

void SimulationView::loadOSM(const QString &filename)
{
    qDebug() << "Loading OSM file:" << filename;
    startStreamingLoad(filename);
    //parseOSMFile(filename);
    qDebug() << "OSM loading completed. Nodes:" << m_osmNodePositions.size()
             << "Roads drawn:" << m_edgeItems.size();
}

QPointF SimulationView::convertLatLon(double lat, double lon) const
{
    return latLonToScene(lat, lon);
}

void SimulationView::setLightMode(LightMode mode)
{
    lm = mode;
    if(lm == nightMode){
        qDebug() << "Всего светофоров в системе:" << m_trafficLights.size();
        qDebug() << "Записей в карте связи (TL ID -> OSM ID):" << m_trafficLightOsmId.size();
        qDebug() << "Узлов Невского проспекта в списке:" << m_mainStreetNodeIds.size();

        if (!m_mainStreetNodeIds.isEmpty()) {
            qDebug() << "Пример IDs узлов Невского (первые 5):"
                     << m_mainStreetNodeIds.values().mid(0, 5);
        }
        //

        qint64 infiniteDuration = 3600000; // TODO обратный вызов для сброса режима

        int countBlinking = 0;
        int countNormal = 0;

        for (auto it = m_trafficLights.begin(); it != m_trafficLights.end(); ++it) {
            TrafficLight* tl = it.value();
            TrafficLightController* controller = m_controllers.value(tl->id());
            if (!controller) continue;

            // Проверяем, стоит ли светофор на главной улице (Невский)
            // Нам нужно знать OSM ID этого светофора.
            long long osmNodeId = m_trafficLightOsmId.value(tl->id(), -1);
            bool isOnMainStreet = m_mainStreetNodeIds.contains(osmNodeId);

            if (isOnMainStreet) {
                // === НЕВСКИЙ ПРОСПЕКТ ===
                // Оставляем автоматический режим (сброс форсирования)
                controller->restartCycle();

                // Визуально: обычная рамка или синяя
                if (m_trafficLightItems.contains(tl->id())) {
                    m_trafficLightItems[tl->id()]->setPen(QPen(Qt::blue, 2));
                }
                countNormal++;
            } else {

                controller->forceState(LightState::Yellow, infiniteDuration);

                controller->setMode(lm);

                if (m_trafficLightItems.contains(tl->id())) {
                    m_trafficLightItems[tl->id()]->setPen(QPen(Qt::yellow, 4));
                }
                countBlinking++;
            }
        }

        qDebug() << "Ночной режим активирован. Мигают желтым:" << countBlinking
                 << ", Работают обычно (Невский):" << countNormal;
    }else{
        for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
            TrafficLightController* controller = it.value();
            long long tlId = it.key();

            controller->restartCycle();
            controller->setMode(lm);

            if (m_trafficLightItems.contains(tlId)) {
                m_trafficLightItems[tlId]->setPen(QPen(Qt::black, 2));
            }
        }

    }
}

QMap<long long, CrossingInfo> SimulationView::getTrafficLightsList() const
{
    QMap<long long, CrossingInfo> result;

    // m_trafficLights использует int как ключ, но нам нужен long long (OSM ID)
    for (auto it = m_trafficLights.begin(); it != m_trafficLights.end(); ++it) {
        long long tlId = it.key();
        TrafficLight* tl = it.value();

        CrossingInfo info;
        info.id = tlId;
        info.name = QString("Перекрёсток #%1").arg(tlId);
        info.requiresAttention = m_currentAttentionState.value(tlId, false);

        result[tlId] = info;
    }

    return result;
}

void SimulationView::setTrafficLightAttention(long long id, bool attention)
{
    if (!m_trafficLights.contains(id)) return;

    m_currentAttentionState[id] = attention;
    emit trafficLightStatusChanged(id, attention);

    // Визуальное обновление на карте
    if (m_trafficLightItems.contains(id)) {
        m_trafficLightItems[id]->setPen(
            attention ? QPen(Qt::red, 4) : QPen(Qt::black, 2)
            );
    }
}

void SimulationView::setTrafficLightCycle(long long id, int greenMs, int yellowMs, int redMs)
{
    if (!m_controllers.contains(id)) {
        qWarning() << "[TL] Controller" << id << "not found!";
        return;
    }

    auto* controller = m_controllers[id];
    controller->setStandardCycle(greenMs, yellowMs, redMs);

    qDebug() << "[TL] Cycle updated for TL" << id
             << "G:" << greenMs/1000 << "s Y:" << yellowMs/1000 << "s R:" << redMs/1000 << "s";
}

void SimulationView::resetTrafficLightCycle(long long id)
{
     setTrafficLightCycle(id, 30000, 5000, 25000);
}

long long SimulationView::getTrafficLightOsmNodeId(long long tlId) const
{
    if (m_trafficLightOsmId.contains(tlId)) {
        return m_trafficLightOsmId[tlId];
    }
    return -1;
}

QList<PendingWay> SimulationView::getAllWays() const
{
    // Возвращаем только дороги, на которых есть светофоры
    QList<PendingWay> waysWithTrafficLights;
    
    for (const PendingWay &way : m_tempWays) {
        if (hasTrafficLightsOnWay(way.nodeRefs)) {
            waysWithTrafficLights.append(way);
        }
    }
    
    return waysWithTrafficLights;
}

bool SimulationView::hasTrafficLightsOnWay(const QList<long long> &nodeRefs) const
{
    // Проверяем, есть ли хотя бы один узел дороги в наборе узлов со светофорами
    for (long long nodeId : nodeRefs) {
        if (m_nodesWithTrafficLights.contains(nodeId)) {
            return true;
        }
    }
    return false;
}

QMap<long long, QPointF> SimulationView::getAllNodePositions() const
{
    // Возвращаем копию всех позиций узлов для использования в AccidentManager
    return m_osmNodePositions;
}

// Маркеры ДТП на карте
void SimulationView::updateAccidentMarkers()
{
    if (!m_accidentManager) return;
    
    // Получаем список активных ДТП
    QList<Accident> activeAccidents = m_accidentManager->getActiveAccidents();
    
    // Временное хранение ID маркеров для текущего кадра
    static QMap<int, QGraphicsEllipseItem*> accidentMarkers;
    
    // Удаляем маркеры для неактивных ДТП
    QList<int> toRemove;
    for (auto it = accidentMarkers.begin(); it != accidentMarkers.end(); ++it) {
        bool found = false;
        for (const Accident &accident : activeAccidents) {
            if (accident.id == it.key()) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_scene->removeItem(it.value());
            delete it.value();
            toRemove.append(it.key());
        }
    }
    for (int id : toRemove) {
        accidentMarkers.remove(id);
    }
    
    // Создаём или обновляем маркеры для активных ДТП
    for (const Accident &accident : activeAccidents) {
        if (!accidentMarkers.contains(accident.id)) {
            // Создаём новый маркер - маленький красный круг (фиксированный размер 20px)
            int markerSize = 20; // Фиксированный небольшой размер
            QGraphicsEllipseItem *marker = new QGraphicsEllipseItem(
                -markerSize/2, 
                -markerSize/2,
                markerSize,
                markerSize
            );
            marker->setPos(accident.position);
            marker->setPen(QPen(Qt::red, 2));
            marker->setBrush(QColor(255, 0, 0, 150)); // Более непрозрачный красный
            marker->setZValue(10); // Выше машин
            m_scene->addItem(marker);
            accidentMarkers[accident.id] = marker;
            
            qDebug() << "Accident marker created for ID:" << accident.id << "at" << accident.position;
        } else {
            // Обновляем позицию существующего маркера
            QGraphicsEllipseItem *marker = accidentMarkers[accident.id];
            marker->setPos(accident.position);
        }
    }
}

SimulationView::TrafficLightCycle SimulationView::getTrafficLightCycle(long long id) const
{
    TrafficLightCycle cycle = {30000, 5000, 25000};

    if (m_controllers.contains(id)) {
        auto* controller = m_controllers[id];
        // Предполагается, что у TrafficLightController есть эти методы
        cycle.green = controller->getGreenDuration();
        cycle.yellow = controller->getYellowDuration();
        cycle.red = controller->getRedDuration();
    }

    return cycle;
}

void SimulationView::setWrongParkingEnabled(bool enabled)
{
    m_wrongParkingEnabled = enabled;
    qDebug() << "Wrong parking simulation" << (enabled ? "enabled" : "disabled");
}

void SimulationView::setWrongParkingProbability(double probability)
{
    m_wrongParkingProbability = qBound(0.0, probability, 1.0);
    qDebug() << "Wrong parking probability set to:" << m_wrongParkingProbability;
}

// Маркеры неправильной парковки
void SimulationView::updateWrongParkingMarkers()
{
    // Статический кэш маркеров неправильной парковки
    static QMap<int, QGraphicsEllipseItem*> parkingMarkers;
    
    // Удаляем маркеры для машин, которые больше не припаркованы неправильно
    QList<int> toRemove;
    for (auto it = parkingMarkers.begin(); it != parkingMarkers.end(); ++it) {
        if (!m_wrongParkedVehicles.contains(it.key())) {
            m_scene->removeItem(it.value());
            delete it.value();
            toRemove.append(it.key());
        }
    }
    for (int id : toRemove) {
        parkingMarkers.remove(id);
    }
    
    // Создаём или обновляем маркеры для неправильно припаркованных машин
    for (int vehicleId : m_wrongParkedVehicles) {
        if (m_vehicles.contains(vehicleId)) {
            Vehicle* vehicle = m_vehicles[vehicleId];
            
            if (!parkingMarkers.contains(vehicleId)) {
                // Создаём новый маркер - красный круг меньшего размера
                QGraphicsEllipseItem *marker = new QGraphicsEllipseItem(
                    -8,   // x: -half width
                    -8,   // y: -half height  
                    16,   // width: 16 pixels
                    16    // height: 16 pixels
                );
                marker->setPos(vehicle->position());
                marker->setPen(QPen(Qt::red, 2));
                marker->setBrush(QColor(255, 0, 0, 100)); // Полупрозрачный красный
                marker->setZValue(11); // Выше машин и ДТП
                m_scene->addItem(marker);
                parkingMarkers[vehicleId] = marker;
                
                qDebug() << "Wrong parking marker created for vehicle:" << vehicleId;
            } else {
                // Обновляем позицию существующего маркера
                QGraphicsEllipseItem *marker = parkingMarkers[vehicleId];
                marker->setPos(vehicle->position());
            }
        }
    }
}

