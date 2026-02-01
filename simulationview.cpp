#include "simulationview.h"
#include <QDebug>
#include <QPen>
#include <QBrush>
#include <cmath>

SimulationView::SimulationView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_timeFactor(1.0)
    , m_lastUpdateTime(0)
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);

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
