#include "visualization/simulationview.h"

#include <QApplication>
#include <QDebug>
#include <cstdio>
#include <QMap>
#include <QList>
#include <QPalette>

void setDarkPalette(QApplication &app)
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(43, 43, 43));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(60, 60, 60));
    darkPalette.setColor(QPalette::AlternateBase, QColor(43, 43, 43));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(60, 60, 60));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::black);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    app.setPalette(darkPalette);
    app.setStyle("Fusion");
}

void debugHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    fprintf(stderr, "%s\n", localMsg.constData());
    fflush(stderr);
}


int main(int argc, char *argv[])
{
    qInstallMessageHandler(debugHandler);
    qDebug() << "хочу квадратик 2";
    QApplication a(argc, argv);
    setDarkPalette(a);
    SimulationView view;
    view.setWindowTitle("Тест");
    view.resize(600, 400);
    view.show();


    // // Добавляем машину в центр
    // view.addVehicle(1, QPointF(0, 0));
    // view.startSimulation();

    return a.exec();
}
