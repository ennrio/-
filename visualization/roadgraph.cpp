#include "roadgraph.h"
#include <QDebug>

void RoadGraph::addNode(int id, double x, double y)
{
    Node node{ id, x, y };
    nodes.insert(id, node);

    // Инициализируем список смежности для нового узла
    if (!adjacencyList.contains(id)) {
        adjacencyList[id] = QList<int>();
    }
}

void RoadGraph::addEdge(int id, int fromNodeId, int toNodeId, double length, bool bidirectional)
{
    // Создаём ребро
    Edge edge{ id, fromNodeId, toNodeId, length };
    edges.insert(id, edge);

    //  Добавляем ID узла-соседа, а не ребра!
    if (!adjacencyList.contains(fromNodeId)) {
        adjacencyList[fromNodeId] = QList<int>();
    }
    adjacencyList[fromNodeId].append(toNodeId);

    //  Если дорога двусторонняя — добавляем обратное ребро
    if (bidirectional) {
        Edge reverseEdge{ -id, toNodeId, fromNodeId, length }; // отрицательный ID для обратного
        edges.insert(-id, reverseEdge);

        if (!adjacencyList.contains(toNodeId)) {
            adjacencyList[toNodeId] = QList<int>();
        }
        adjacencyList[toNodeId].append(fromNodeId);
    }

    static int debugCount = 0;
    if (debugCount < 10) {
        qDebug() << "Edge added:" << fromNodeId << "->" << toNodeId;
        qDebug() << "  Adjacency list for" << fromNodeId << ":" << adjacencyList[fromNodeId];
        debugCount++;
    }
}

QPointF RoadGraph::getNodePosition(int nodeId) const
{
    if (nodes.contains(nodeId)) {
        const Node& node = nodes.value(nodeId);
        return QPointF(node.x, node.y);
    }
    return QPointF(0, 0);
}

QList<int> RoadGraph::getNeighbors(int nodeId) const
{
    return adjacencyList.value(nodeId);
}

QList<int> RoadGraph::findRoute(int startNode, int endNode)
{
    // Создаём локальные копии данных для потокобезопасности
    QMap<int, Node> localNodes = nodes;
    QMap<int, QList<int>> localAdjacency = adjacencyList;
    QMap<int, Edge> localEdges = edges;
    
    if (!localNodes.contains(startNode)) {
        qWarning() << "RoadGraph: Start node" << startNode << "not found!";
        return QList<int>();
    }
    if (!localNodes.contains(endNode)) {
        qWarning() << "RoadGraph: End node" << endNode << "not found!";
        return QList<int>();
    }
    if (startNode == endNode) {
        return QList<int>{ startNode };
    }

    // Алгоритм Дейкстры
    QMap<int, double> distances;
    QMap<int, int> previous;

    using NodeDist = std::pair<double, int>;
    std::priority_queue<NodeDist, std::vector<NodeDist>, std::greater<NodeDist>> pq;

    // Инициализация - теперь работаем с локальной копией
    for (auto it = localNodes.begin(); it != localNodes.end(); ++it) {
        distances[it.key()] = std::numeric_limits<double>::infinity();
        previous[it.key()] = -1;
    }

    distances[startNode] = 0.0;
    pq.push({ 0.0, startNode });

    int visited = 0;
    while (!pq.empty()) {
        auto [currDist, currNode] = pq.top();
        pq.pop();
        visited++;

        if (currNode == endNode) break;
        if (currDist > distances[currNode]) continue;

        if (localAdjacency.contains(currNode)) {
            for (int neighbor : localAdjacency[currNode]) {
                // Ищем ребро между currNode и neighbor
                double edgeLength = -1;
                for (const Edge& edge : localEdges) {
                    if (edge.fromNodeId == currNode && edge.toNodeId == neighbor) {
                        edgeLength = edge.length;
                        break;
                    }
                }
                if (edgeLength < 0) continue; // Ребро не найдено

                double newDist = currDist + edgeLength;
                if (newDist < distances[neighbor]) {
                    distances[neighbor] = newDist;
                    previous[neighbor] = currNode;
                    pq.push({ newDist, neighbor });
                }
            }
        }
    }

    // Восстановление пути
    if (previous[endNode] == -1 && startNode != endNode) {
        return QList<int>();
    }

    QList<int> path;
    for (int curr = endNode; curr != -1; curr = previous[curr]) {
        path.prepend(curr);
    }

    return path;
}

void RoadGraph::clear()
{
    nodes.clear();
    edges.clear();
    adjacencyList.clear();
}
