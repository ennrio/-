#ifndef DATAGENERATORWIDGET_H
#define DATAGENERATORWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QTimer>
#include "visualization/simulationview.h"

class DataGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataGeneratorWidget(QWidget *parent = nullptr);
    void updateStatus(bool isGenerating, int activeAccidents);

private slots:
    void startGeneration();
    void stopGeneration();
    void restartGeneration();
    void onAccidentToggled(bool checked);
    void onProbabilityChanged(const QString &text);
    void onParkingToggled(bool checked);
    void onParkingProbabilityChanged(const QString &text);
    void onSaveProfile();
    void onLoadProfile();
    void updateAccidentCount();

private:
    QComboBox *m_profileCombo;
    QComboBox *m_intensityCombo;
    QCheckBox *m_accidentCheck;
    QComboBox *m_probabilityCombo;  // Вероятность ДТП
    QCheckBox *m_parkingCheck;      // Неправильная парковка
    QComboBox *m_parkingProbabilityCombo;  // Вероятность неправильной парковки
    QPushButton *m_startBtn, *m_stopBtn, *m_restartBtn;
    QPushButton *m_saveProfileBtn, *m_loadProfileBtn;
    QLabel *m_statusLabel;
    QLabel *m_accidentCountLabel;      // Метка количества активных ДТП
    QLabel *m_parkingCountLabel;       // Метка количества неправильных парковок
    QTimer *m_updateTimer;
    
    bool m_isGenerating;
    SimulationView* view;
};

#endif // DATAGENERATORWIDGET_H
