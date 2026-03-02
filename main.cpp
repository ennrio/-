#include "mainwindow.h"
#include "simulationview.h"
#include "osmparser.h"

#include <QApplication>
#include <QDebug>
#include <iostream>
#include <cstdio>
#include <QMap>
#include <QList>
#include <queue>
#include <functional>
#include <limits>

void debugHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    fprintf(stderr, "%s\n", localMsg.constData());
    fflush(stderr);
}


int main(int argc, char *argv[])
{
    qInstallMessageHandler(debugHandler);
    qDebug() << "fdlsjdf";
    QApplication a(argc, argv);
    SimulationView view;
    view.setWindowTitle("Тест");
    view.resize(600, 400);
    view.show();

    view.loadOSM("/home/egor/all/study/6sem/ASUDD/dvortsovaya.osm");

    // Добавляем машину в центр
    view.addVehicle(1, QPointF(0, 0));
    view.startSimulation();

    return a.exec();
}
