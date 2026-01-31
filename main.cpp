#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <iostream>
#include <cstdio>
#include <QMap>
#include <QList>
#include <queue>
#include <functional>
#include <limits>

//узел дороги
struct Node{
    int id;
    double x,y;
};

// сегмент между двумя зулами
struct Edge{
    int id;
    int fromNodeId;
    int toNodeId;
    double length;
    int lanes;
    double speedLimit;
};

//граф дорог
class RoadGraph{
    QMap<int, Node> nodes;
    QMap<int, Edge> edges;
    QMap<int, QList<int>> adjacencyList;
public:
    void addNode(int _id, double _x, double _y){
        Node node;
        node.id = _id;
        node.x = _x;
        node.y = _y;
        nodes.insert(_id,node);
    };

    void addEdge(int _id, double _from, int _to, double _length){
        Edge edge;
        edge.id = _id;
        edge.fromNodeId = _from;
        edge.toNodeId = _to;
        edge.length = _length;
        edges.insert(_id, edge);

        int fromNodeId = (int)_from;
        if (!adjacencyList.contains(fromNodeId)) {
            adjacencyList[fromNodeId] = QList<int>();
        }
        adjacencyList[fromNodeId].append(_id);
    };

    QList<int> findRoute(int startNode, int endNode){
        // Если начальный или конечный узел не существуют
        if (!nodes.contains(startNode) || !nodes.contains(endNode)) {
            fprintf(stderr, "Start or end node not found\n");
            return QList<int>();
        }

        // Если начальный и конечный узел совпадают
        if (startNode == endNode) {
            return QList<int>{startNode};
        }

        // Алгоритм Дейкстры
        QMap<int, double> distances; // Расстояния от начального узла
        QMap<int, int> previousNode; // Предыдущий узел на кратчайшем пути

        // Приоритетная очередь для обработки узлов
        using NodeDistPair = std::pair<double, int>; // <расстояние, id узла>
        std::priority_queue<NodeDistPair, std::vector<NodeDistPair>, std::greater<NodeDistPair>> pq;

        // Инициализация
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            int nodeId = it.key();
            distances[nodeId] = std::numeric_limits<double>::infinity();
            previousNode[nodeId] = -1;
        }

        distances[startNode] = 0.0;
        pq.push(std::make_pair(0.0, startNode));

        while (!pq.empty()) {
            NodeDistPair current = pq.top();
            pq.pop();

            double currentDist = current.first;
            int currentNode = current.second;

            // Если мы достигли конечного узла
            if (currentNode == endNode) {
                fprintf(stderr, "Reached destination node %d\n", currentNode);
                break;
            }

            // Если текущее расстояние больше сохраненного, пропускаем
            if (currentDist > distances[currentNode]) {
                continue;
            }

            // Обрабатываем всех соседей
            if (adjacencyList.contains(currentNode)) {
                for (int edgeId : adjacencyList[currentNode]) {
                    const Edge& edge = edges[edgeId];
                    int neighborNode = edge.toNodeId;

                    // Пропускаем, если ребро ведет обратно
                    if (neighborNode == currentNode) {
                        continue;
                    }

                    double newDist = currentDist + edge.length;

                    if (newDist < distances[neighborNode]) {
                        distances[neighborNode] = newDist;
                        previousNode[neighborNode] = currentNode;
                        pq.push(std::make_pair(newDist, neighborNode));
                        fprintf(stderr, "Updated distance to node %d: %.2f (via node %d)\n",
                                neighborNode, newDist, currentNode);
                    }
                }
            }
        }

        // Восстанавливаем путь
        if (previousNode[endNode] == -1) {
            fprintf(stderr, "No path found from %d to %d\n", startNode, endNode);
            return QList<int>();
        }

        QList<int> path;
        int currentNode = endNode;

        // Восстанавливаем путь в обратном порядке
        while (currentNode != -1) {
            path.prepend(currentNode);
            currentNode = previousNode[currentNode];
        }

        fprintf(stderr, "Path found with %d nodes. Total distance: %.2f\n",
                path.size(), distances[endNode]);

        fprintf(stderr, "Route: ");
        for (int i = 0; i < path.size(); i++) {
            fprintf(stderr, "%d", path[i]);
            if (i < path.size() - 1) {
                fprintf(stderr, " -> ");
            }
        }
        fprintf(stderr, "\n");

        return path;
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // MainWindow w;
    // w.show();


    RoadGraph graph;
    graph.addNode(1,0,0);
    graph.addNode(2,100,0);
    graph.addNode(3,100,100);
    graph.addNode(4,0,100);
    graph.addEdge(1,1,2,100.0);
    graph.addEdge(2,2,3,100.0);
    graph.findRoute(1,3);
    return 0;
    //return a.exec();
}
