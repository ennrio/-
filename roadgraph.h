#ifndef ROADGRAPH_H
#define ROADGRAPH_H

#include<QMap>
#include<iostream>
#include<queue>
#include<QPointF>
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
    void addNode(int _id, double _x, double _y);

    void addEdge(int _id, double _from, int _to, double _length);

    QPointF getNodePosition(int node_id) const;

    QList<int> findRoute(int startNode, int endNode);
    void clear();
};

#endif // ROADGRAPH_H
