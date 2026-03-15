#ifndef MAINSCREENWIDGET_H
#define MAINSCREENWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QTreeWidget>
#include <QTabWidget>
#include <QTimer>
#include "visualization/simulationview.h"

class MainScreenWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainScreenWidget(QWidget *parent = nullptr);

private:
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_topBar;
    QLabel *m_statusLabel;
    QPushButton *m_newShiftBtn, *m_exportLogsBtn, *m_diagnosticBtn;

    // Информация о сессии
    QGroupBox *m_sessionBox;
    QVBoxLayout *m_sessionLayout;
    QLabel *m_operator, *m_startTimeLabel, *m_durationLabel;
    QLabel *m_devicesLabel, *m_incidentsLabel, *m_speedLabel, *m_loadLabel;
    QLabel *devicesCount;
    QLabel *speedLabel;
    QLabel *speedCount;
    QLabel *loadCount;
    QLabel *m_operatorLabel;
    SimulationView* m_simulationView;
    QLabel *m_alertLabel;

    // Быстрые действия
    QGroupBox *m_actionsBox;
    QHBoxLayout *m_actionsLayout;
    QPushButton *m_startNewShift, *m_exportLogs, *m_diagnostic;

    // Карта (ваша SimulationView)
    QWidget *m_mapContainer;

    QTimer *m_updateTimer;
    QTimer *m_shiftTimer;
    QDateTime m_shiftStartTime;

    // регистрационные параметры
    QString name = "";
    QString role = "";
    QString startTime = "";
    int hours = 0;
    int minutes = 0;
    int maxCountVehicle = 0;

private slots:
    void onStartNewShift();
    void updateMainScreen();
};

#endif // MAINSCREENWIDGET_H
