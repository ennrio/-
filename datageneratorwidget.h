#ifndef DATAGENERATORWIDGET_H
#define DATAGENERATORWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>

class DataGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataGeneratorWidget(QWidget *parent = nullptr);

private slots:
    void stopGeneration();
    void onStartAccidentSimulation();
    void onStopAccidentSimulation();
    void onCreateAccident();
    void onProbabilityChanged(const QString &text);

private:
    QComboBox *m_profileCombo;
    QComboBox *m_intensityCombo;
    QComboBox *m_weatherCombo;
    QComboBox *m_timeCombo;
    QLineEdit *m_intervalInput;
    QCheckBox *m_accidentCheck;
    QCheckBox *m_repairCheck;
    QCheckBox *m_eventsCheck;
    QCheckBox *m_equipmentCheck;
    QComboBox *m_frequencyCombo;
    QComboBox *m_probabilityCombo;  // Вероятность ДТП
    QPushButton *m_startBtn, *m_stopBtn, *m_generateBtn;
    QPushButton *m_createAccidentBtn;  // Кнопка создания ДТП
    QLabel *m_accidentCountLabel;      // Метка количества активных ДТП
};

#endif // DATAGENERATORWIDGET_H
