#ifndef TRAFFICLIGHTCONTROLWIDGET_H
#define TRAFFICLIGHTCONTROLWIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include "visualization/simulationview.h"


class TrafficLightControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrafficLightControlWidget(QWidget *parent = nullptr);

private slots:
    void onModeChanged(const QString &modeText);

private:  
    QListWidget *m_crossingList;
    QGroupBox *m_crossingListGroupBox;
    QGroupBox *m_controlGroupBox;
    QGroupBox *m_paramsGroupBox;
    QGroupBox *m_priorityGroupBox;
    QComboBox *m_modeComboBox;
    QLabel *m_greenLabel, *m_yellowLabel, *m_redLabel;
    QLabel *statusLabel;
    QLineEdit *m_greenInput, *m_yellowInput, *m_redInput;
    QListWidget *m_priorityList;
    QPushButton *m_applyButton, *m_resetButton, *m_diagnosticsButton;
    LightMode lm;
    SimulationView* sv;
};

#endif // TRAFFICLIGHTCONTROLWIDGET_H
