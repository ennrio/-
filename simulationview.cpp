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



SimulationView::SimulationView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_timeFactor(1.0)
    , m_lastUpdateTime(0)
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
}

void SimulationView::addNode(int id, double x, double y)
{
    // Добавляем в граф
    m_roadGraph.addNode(id, x, y);

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

void SimulationView::addEdge(int id, int from, int to, double length)
{
    // Добавляем в граф
    m_roadGraph.addEdge(id, from, to, length);

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

void SimulationView::setVehicleRoute(int vehicleId, const QList<int>& nodeIds)
{
    if (!m_vehicles.contains(vehicleId)) {
        qWarning() << "Vehicle" << vehicleId << "not found";
        return;
    }

    // Находим путь через граф
    if (nodeIds.size() < 2) {
        qWarning() << "Route needs at least 2 nodes";
        return;
    }

    // Ищем маршрут через граф с помощью алгоритма поиска пути
    QList<int> path;

    // Проходим по всем узлам и находим путь между ними
    for (int i = 0; i < nodeIds.size() - 1; i++) {
        int start = nodeIds[i];
        int end = nodeIds[i + 1];

        QList<int> segmentPath = m_roadGraph.findRoute(start, end);

        // Если это не первый сегмент, убираем первую точку (она совпадает с последней предыдущего сегмента)
        if (i > 0 && !segmentPath.isEmpty()) {
            segmentPath.removeFirst();
        }

        path.append(segmentPath);
    }

    if (path.isEmpty()) {
        qWarning() << "No valid route found through the graph";
        return;
    }

    QList<QPointF> routePoints;

    // Преобразуем ID узлов в точки
    for (int nodeId : path) {
        routePoints.append(getNodePosition(nodeId));
    }

    // Устанавливаем маршрут транспортному средству
    m_vehicles[vehicleId]->setRoute(routePoints);
    m_vehicles[vehicleId]->setSpeed(20.0); // Скорость 5 м/с

    qDebug() << "Route set for vehicle" << vehicleId << "with" << routePoints.size() << "points";
    qDebug() << "Path nodes:" << path;
}

QPointF SimulationView::getNodePosition(int nodeId) const
{
    // Используем метод из RoadGraph для получения реальной позиции узла
    return m_roadGraph.getNodePosition(nodeId);
}

void SimulationView::startSimulation()
{
    m_lastUpdateTime = m_elapsedTimer.elapsed() / 1000.0;
    m_simulationTimer.start();
    qDebug() << "Simulation started";
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

    // Обновляем сцену
    m_scene->update();
}

void SimulationView::updateVehicleGraphics()
{
    // Эта функция вызывается при изменении графики
    for (auto it = m_vehicleItems.begin(); it != m_vehicleItems.end(); ++it) {
        it.value()->update();
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
                TrafficLight tl;
                tl.id = id;
                tl.position = scenePos;
                tl.direction = direction;
                tl.isPedestrian = isPedestrian;
                m_trafficLights[id] = tl;

                // Визуализируем как кружок
                QGraphicsEllipseItem* tlItem = new QGraphicsEllipseItem(-8, -8, 16, 16);
                tlItem->setPos(scenePos);
                tlItem->setBrush(QBrush(Qt::red)); // Красный = стоп по умолчанию
                tlItem->setPen(QPen(Qt::black, 1));
                tlItem->setZValue(5); // Светофор поверх всего
                m_scene->addItem(tlItem);

                // Добавляем метку
                QGraphicsTextItem* label = new QGraphicsTextItem("🚦");
                label->setPos(scenePos.x() + 10, scenePos.y() - 10);
                label->setZValue(5);
                m_scene->addItem(label);
            }

            // Пропускаем содержимое узла
            while (!(xml.isEndElement() && xml.name() == "node")) {
                xml.readNext();
            }
        }
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
            long long wayId = xml.attributes().value("id").toLongLong();
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
    for (auto it = tempNodes.begin(); it != tempNodes.end(); ++it) {
        long long osmId = it.key();
        double lat = it.value().x();
        double lon = it.value().y();

        // Конвертируем в координаты сцены
        QPointF scenePos = latLonToScene(lat, lon);
        m_osmNodePositions[osmId] = scenePos;

        // Маппинг OSM ID → внутренний ID
        m_osmToInternalId[osmId] = internalIdCounter;

        // Добавляем узел в граф (только если он используется в дорогах)
        for (const auto &way : waysToDraw) {
            if (way.first.contains(osmId)) {
                addNode(internalIdCounter, scenePos.x(), scenePos.y());
                break;
            }
        }

        internalIdCounter++;
    }

    if (!m_osmNodePositions.isEmpty()) {
        double minX = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        for (const QPointF &pos : m_osmNodePositions) {
            minX = qMin(minX, pos.x());
            maxX = qMax(maxX, pos.x());
        //qDebug() << "Centered on:" << QPointF((minX + maxX) / 2.0, (minY + maxY) / 2.0);
        }
    }

    // Этап 4: Рисуем дороги
    int edgeId = 1;
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
                addEdge(edgeId++, fromId, toId, length);
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

void SimulationView::loadOSM(const QString &filename)
{
    qDebug() << "Loading OSM file:" << filename;
    parseOSMFile(filename);
    qDebug() << "OSM loading completed. Nodes:" << m_osmNodePositions.size()
             << "Roads drawn:" << m_edgeItems.size();
}

QPointF SimulationView::convertLatLon(double lat, double lon) const
{
    return latLonToScene(lat, lon);
}
