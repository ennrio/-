#include "mainwindow.h"
#include "simulationview.h"

#include <QApplication>
#include <QDebug>
#include <iostream>
#include <cstdio>
#include <QMap>
#include <QList>
#include <queue>
#include <functional>
#include <limits>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // MainWindow w;
    // w.show();


    SimulationView view;
    view.setWindowTitle("Traffic Simulation");
    view.resize(800, 600);
    view.show();

    // Создаем дорожную сеть
    view.addNode(1, 0, 0);
    view.addNode(2, 100, 0);
    view.addNode(3, 100, 100);
    view.addNode(4, 0, 100);

    view.addEdge(1, 1, 2, 100.0);
    view.addEdge(2, 2, 3, 100.0);
    view.addEdge(3, 3, 4, 100.0);
    view.addEdge(4, 4, 1, 100.0);

    // Добавляем транспортные средства
    view.addVehicle(1, QPointF(0, 0));
    view.addVehicle(2, QPointF(50, 50));

    // Задаем маршруты
    view.setVehicleRoute(1, QList<int>{1, 2, 3, 4});
    view.setVehicleRoute(2, QList<int>{4, 3, 2, 1});

    //Запускаем симуляцию
    view.startSimulation();
    //return 0;
    return a.exec();
}
