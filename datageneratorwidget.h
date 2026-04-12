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

class DataGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataGeneratorWidget(QWidget *parent = nullptr);
    void updateStatus(bool isGenerating, int activeAccidents);

private slots:
    void startGeneration();
    void stopGeneration();
    void onAccidentToggled(bool checked);
    void onProbabilityChanged(const QString &text);
    void onSaveProfile();
    void onLoadProfile();
    void updateAccidentCount();

private:
    QComboBox *m_profileCombo;
    QComboBox *m_intensityCombo;
    QLineEdit *m_intervalInput;
    QCheckBox *m_accidentCheck;
    QComboBox *m_probabilityCombo;  // Вероятность ДТП
    QPushButton *m_startBtn, *m_stopBtn;
    QPushButton *m_saveProfileBtn, *m_loadProfileBtn;
    QLabel *m_statusLabel;
    QLabel *m_accidentCountLabel;      // Метка количества активных ДТП
    QTimer *m_updateTimer;
    
    bool m_isGenerating;
};

#endif // DATAGENERATORWIDGET_H
