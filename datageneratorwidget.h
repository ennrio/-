#ifndef DATAGENERATORWIDGET_H
#define DATAGENERATORWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>

class DataGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataGeneratorWidget(QWidget *parent = nullptr);

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
    QPushButton *m_startBtn, *m_stopBtn, *m_generateBtn;
};

#endif // DATAGENERATORWIDGET_H
