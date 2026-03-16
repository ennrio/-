#ifndef TRAFFICLIGHTCONTROLWIDGET_H
#define TRAFFICLIGHTCONTROLWIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include "visualization/simulationview.h"  // Отсюда берём CrossingInfo

class TrafficLightControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrafficLightControlWidget(QWidget *parent = nullptr);
    void syncWithSimulation();

private slots:
    void onModeChanged(const QString &modeText);
    void onCrossingSelected();
    void updateCrossingStatus(long long tlId, bool requiresAttention);

private:
    QGroupBox *m_controlGroupBox;
    QGroupBox *m_paramsGroupBox;
    QGroupBox *m_priorityGroupBox;
    QComboBox *m_modeComboBox;
    QLabel *statusLabel;
    QLineEdit *m_greenInput, *m_yellowInput, *m_redInput;
    QListWidget *m_crossingList;      // ОДИН список для перекрёстков
    QListWidget *m_priorityList;
    QPushButton *m_applyButton, *m_resetButton, *m_diagnosticsButton;

    SimulationView* sv;
    QMap<long long, CrossingInfo> m_crossingsMap;
    long long m_selectedCrossingId;

    void updateCrossingListItem(long long id);
    void loadCrossingParams(long long id);
};

#endif // TRAFFICLIGHTCONTROLWIDGET_H
