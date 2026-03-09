#ifndef ROADGRAPH_H
#define ROADGRAPH_H

#include <QMap>
#include <QPointF>
#include <QList>
#include <queue>
#include <limits>

struct Node {
    int id;
    double x, y;
};

struct Edge {
    int id;
    int fromNodeId;
    int toNodeId;
    double length;
    int lanes{1};
    double speedLimit{50.0}; // км/ч
};

class RoadGraph {
    QMap<int, Node> nodes;
    QMap<int, Edge> edges;
    QMap<int, QList<int>> adjacencyList;  // node_id -> список соседних node_id

public:
    void addNode(int id, double x, double y);

    void addEdge(int id, int fromNodeId, int toNodeId, double length, bool bidirectional = true);

    QPointF getNodePosition(int nodeId) const;
    QList<int> findRoute(int startNode, int endNode);

    // Для отладки
    bool hasNode(int id) const { return nodes.contains(id); }
    bool hasEdge(int id) const { return edges.contains(id); }
    QList<int> getNeighbors(int nodeId) const;

    void clear();
    QMap<int, Node> getNodes() const { return nodes; }
    int nodeCount() const { return nodes.size(); }
    int edgeCount() const { return edges.size(); }
};

#endif // ROADGRAPH_H
