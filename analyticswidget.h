#ifndef ANALYTICSWIDGET_H
#define ANALYTICSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QVector>
#include <QStringList>
#include "logger.h"

struct LogDataPoint {
    QString time;
    int connectedDevices;
    int activeIncidents;
    double avgSpeed;
    double roadLoad;
    int trafficJams;
};

class AnalyticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnalyticsWidget(QWidget *parent = nullptr);

private slots:
    void onLoadLogsClicked();
    void onGenerateReportPdfClicked();
    void onGenerateReportExcelClicked();

private:
    void parseLogFile(const QString &filePath);
    void updateMetrics();
    void drawCharts();

    QPushButton *m_loadLogsBtn;
    QLabel *m_avgTimeLabel, *m_accidentsLabel;
    QLabel *m_devicesChartLabel, *m_jamsChartLabel;
    QPushButton *m_reportPdfBtn, *m_reportExcelBtn;
    
    QVector<LogDataPoint> m_logData;
    QStringList m_loadedFiles;
    int m_accidentCount;
};

#endif // ANALYTICSWIDGET_H
