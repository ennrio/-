#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMenuBar>
#include "mainscreenwidget.h"
#include "trafficlightcontrolwidget.h"
#include "datageneratorwidget.h"
#include "analyticswidget.h"
// TODO #include "trafficlightcontrolwidget.h" и др.

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void showMainScreen();
    void showTrafficLights();
    void showDataGenerator();
    void showAnalytics();

private:
    QStackedWidget *m_stackedWidget;
    MainScreenWidget *m_mainScreen;
    TrafficLightControlWidget *m_trafficlightControllerWidget;
    DataGeneratorWidget *m_datageneratorwidget;
    AnalyticsWidget *m_analyticsWidget;
    //TODO TrafficLightControlWidget *m_trafficLights; и др.
};

#endif // MAINWINDOW_H
