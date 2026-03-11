#ifndef ANALYTICSWIDGET_H
#define ANALYTICSWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

class AnalyticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnalyticsWidget(QWidget *parent = nullptr);

private:
    QComboBox *m_periodCombo;
    QLabel *m_avgTimeLabel, *m_capacityLabel, *m_accidentsLabel, *m_timeSavedLabel;
    QPushButton *m_dailyReportBtn, *m_weeklyReportBtn, *m_monthlyReportBtn, *m_customReportBtn;
    QPushButton *m_compareBtn, *m_exportBtn, *m_printBtn;
};

#endif // ANALYTICSWIDGET_H
