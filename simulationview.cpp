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




SimulationView::SimulationView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_timeFactor(1.0)
    , m_lastUpdateTime(0)
    , m_isLoading(false)
    , m_currentPhase(LoadPhase::ParsingNodes)
    , m_internalIdCounter(1)
    , m_roadGraph(new RoadGraph())
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);


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

    connect(this, &SimulationView::osmLoadingFinished,
            this, &SimulationView::onOSMLoadingFinished,
            Qt::UniqueConnection);
}

SimulationView::~SimulationView()
{
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
    m_vehicles[vehicleId]->setSpeed(10.0 + QRandomGenerator::global()->bounded(5)); // 10-15 м/с

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
    qDebug() << "Simulation started with vehicle spawning";
}

void SimulationView::stopSimulation()
{
    m_simulationTimer.stop();
    qDebug() << "Simulation stopped";
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

void SimulationView::updateSimulation()
{
    // Вычисляем дельту времени
    double currentTime = m_elapsedTimer.elapsed() / 1000.0;
    double deltaTime = (currentTime - m_lastUpdateTime) * m_timeFactor;
    m_lastUpdateTime = currentTime;

    // Обновляем все транспортные средства
    for (auto it = m_vehicles.begin(); it != m_vehicles.end(); ++it) {
        it.value()->update(deltaTime);
    }
    updateVehicleGraphics();
    // Обновляем сцену
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
    controller->forceState(nextState, 30000);

    qDebug() << "Traffic light" << id
             << "manually changed to" << static_cast<int>(nextState)
             << "(will auto-resume in 30s)";
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

    // ✅ Проверка: первые 10 узлов и их соседи
    for (int i = 0; i < qMin(10, nodes.size()); i++) {
        int nodeId = nodes[i];
        auto neighbors = m_roadGraph->getNeighbors(nodeId);
        qDebug() << "Node" << nodeId << "neighbors:" << neighbors;
    }

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
    if (m_osmNodePositions.isEmpty() || m_roadGraph->nodeCount() < 2) {
        qDebug() << "Cannot spawn: RoadGraph has" << m_roadGraph->nodeCount() << "nodes";
        return;
    }

    // ✅ Получаем ТОЛЬКО узлы у которых есть соседи
    QList<int> internalIds;
    auto allNodes = m_roadGraph->getNodes().keys();
    for (int nodeId : allNodes) {
        if (m_roadGraph->getNeighbors(nodeId).size() > 0) {
            internalIds.append(nodeId);
        }
    }

    if (internalIds.size() < 2) {
        qDebug() << "Not enough connected nodes for routing:" << internalIds.size();
        return;
    }

    // ✅ Пробуем найти маршрут
    int attempts = 0;
    QList<QPointF> routePoints;

    while (attempts < 5 && routePoints.isEmpty()) {
        attempts++;

        int startIdx = QRandomGenerator::global()->bounded(internalIds.size());
        int endIdx = QRandomGenerator::global()->bounded(internalIds.size());

        if (startIdx == endIdx) continue;

        int startInternal = internalIds[startIdx];
        int endInternal = internalIds[endIdx];

        qDebug() << "Attempt" << attempts << ": Finding route"
                 << startInternal << "->" << endInternal
                 << "(start neighbors:" << m_roadGraph->getNeighbors(startInternal).size() << ")";

        QList<int> path = m_roadGraph->findRoute(startInternal, endInternal);

        if (path.isEmpty()) {
            qDebug() << "No path found, trying again...";
            continue;
        }

        // ✅ Конвертируем путь в координаты
        for (int nodeId : path) {
            long long osmId = -1;
            for (auto it = m_osmToInternalId.begin(); it != m_osmToInternalId.end(); ++it) {
                if (it.value() == nodeId) {
                    osmId = it.key();
                    break;
                }
            }
            if (osmId >= 0 && m_osmNodePositions.contains(osmId)) {
                routePoints.append(m_osmNodePositions[osmId]);
            }
        }
    }

    if (routePoints.size() < 2) {
        qDebug() << "Failed to create route after 5 attempts";
        return;
    }

    int vehicleId = ++m_vehicleCounter;
    QPointF startPos = routePoints.first();
    addVehicle(vehicleId, startPos);
    setVehicleRoute(vehicleId, routePoints);

    if (m_vehicles.contains(vehicleId)) {
        m_vehicles[vehicleId]->setTrafficLightChecker(
            [this](const QPointF& pos, qreal radius) {
                return getTrafficLightStateAtPosition(pos, radius);
            }
            );
        m_vehicles[vehicleId]->setTrafficLightAwareness(true);
    }

    qDebug() << "Vehicle" << vehicleId << "spawned with route of"
             << routePoints.size() << "points";
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
    const double metersPerPixel = 0.5; // 1 пиксель = 0.5 метра (для района 2×2 км)

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

                auto* controller = new TrafficLightController(tl, this);
                controller->setStandardCycle(30000, 5000, 25000); // зелёный/жёлтый/красный
                m_controllers[tl->id()] = controller;

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
        int edgeId = 1;  // ✅ ОБЪЯВЛЯЕМ edgeId ЗДЕСЬ

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
                            auto* tl = new TrafficLight(id, scenePos, direction, isPedestrian, this);
                            m_trafficLights[tl->id()] = tl;

                            // Создаём контроллер с дефолтным циклом
                            auto* controller = new TrafficLightController(tl, this);
                            controller->setStandardCycle(30000, 5000, 25000);
                            m_controllers[tl->id()] = controller;

                            // Визуализация
                            auto* item = new TrafficLightItem(
                                tl->id(),
                                [this](long long id) {
                                    onTrafficLightClicked(id);  // ✅ Вызов метода SimulationView
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
                        }
                    }
                }

                waysFoundInChunk++;

                if (isRoad && nodeRefs.size() >= 2) {
                    bool allNodesLoaded = true;
                    for (long long ref : nodeRefs) {
                        if (!m_osmNodePositions.contains(ref)) {
                            allNodesLoaded = false;
                            break;
                        }
                    }

                    if (allNodesLoaded) {
                        drawOSMRoad(nodeRefs, highwayType);

                        // ✅ ДОБАВЛЯЕМ РЁБРА В ROADGRAPH
                        for (int i = 0; i < nodeRefs.size() - 1; ++i) {
                            long long fromOsm = nodeRefs[i];
                            long long toOsm = nodeRefs[i+1];

                            if (m_osmToInternalId.contains(fromOsm) && m_osmToInternalId.contains(toOsm)) {
                                int fromInternal = m_osmToInternalId[fromOsm];
                                int toInternal = m_osmToInternalId[toOsm];

                                // ✅ Проверка: не добавлено ли уже
                                auto existingNeighbors = m_roadGraph->getNeighbors(fromInternal);
                                if (!existingNeighbors.contains(toInternal)) {
                                    m_roadGraph->addEdge(
                                        m_edgeIdCounter++,
                                        fromInternal,
                                        toInternal,
                                        1.0,
                                        true
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
