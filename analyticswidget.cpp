#include "analyticswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>

AnalyticsWidget::AnalyticsWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #2B2B2B; color: #FFFFFF;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // === Верхняя панель статуса ===
    QLabel *statusLabel = new QLabel("Данные актуальны");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "font-weight: bold; "
        "color: #00AA00; "
        "background-color: #202020; "
        "border-radius: 4px; "
        "padding: 8px; "
        "font-size: 14px;"
        );
    mainLayout->addWidget(statusLabel);

    // === Аналитика эффективности дорожного движения ===
    QGroupBox *analyticsGroupBox = new QGroupBox("Аналитика эффективности дорожного движения");
    analyticsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QVBoxLayout *analyticsLayout = new QVBoxLayout(analyticsGroupBox);

    // Период анализа
    QHBoxLayout *periodLayout = new QHBoxLayout;
    periodLayout->addWidget(new QLabel("Период анализа:"));
    m_periodCombo = new QComboBox;
    m_periodCombo->addItems({
        "Текущая смена (22.10.2025 08:00-14:30)",
        "Сегодня (22.10.2025)",
        "Вчера (21.10.2025)",
        "Текущая неделя",
        "Текущий месяц",
        "Произвольный период"
    });
    periodLayout->addWidget(m_periodCombo);
    periodLayout->addWidget(new QPushButton("Применить"));
    analyticsLayout->addLayout(periodLayout);

    // Ключевые показатели
    QGridLayout *metricsLayout = new QGridLayout;
    metricsLayout->setContentsMargins(10, 10, 10, 10);

    m_avgTimeLabel = new QLabel("Среднее время в пути (артерии) 32 мин ▼ 8 мин (20%)");
    m_avgTimeLabel->setStyleSheet("color: #00AA00; font-weight: bold; font-size: 14px;");

    m_capacityLabel = new QLabel("Пропускная способность 2,240 авт/ч ▲ 240 авт/ч (12%)");
    m_capacityLabel->setStyleSheet("color: #00AA00; font-weight: bold; font-size: 14px;");

    m_accidentsLabel = new QLabel("Количество ДТП 2 ▼ 1 (33%)");
    m_accidentsLabel->setStyleSheet("color: #CC0000; font-weight: bold; font-size: 14px;");

    m_timeSavedLabel = new QLabel("Экономия времени 1,240 ч ▲ 320 ч (35");
    m_timeSavedLabel->setStyleSheet("color: #00AA00; font-weight: bold; font-size: 14px;");

    metricsLayout->addWidget(m_avgTimeLabel, 0, 0);
    metricsLayout->addWidget(m_capacityLabel, 0, 1);
    metricsLayout->addWidget(m_accidentsLabel, 1, 0);
    metricsLayout->addWidget(m_timeSavedLabel, 1, 1);

    analyticsLayout->addLayout(metricsLayout);

    // Динамика загруженности дорог
    QGroupBox *trendGroupBox = new QGroupBox("Динамика загруженности дорог");
    trendGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QVBoxLayout *trendLayout = new QVBoxLayout(trendGroupBox);

    // Имитация графика
    QLabel *graphPlaceholder = new QLabel("06:00  09:00  12:00  15:00  18:00  21:00  00:00");
    graphPlaceholder->setAlignment(Qt::AlignCenter);
    graphPlaceholder->setStyleSheet("background-color: #1E1E1E; border-radius: 4px; padding: 10px;");
    trendLayout->addWidget(graphPlaceholder);

    analyticsLayout->addWidget(trendGroupBox);
    analyticsGroupBox->setLayout(analyticsLayout);
    mainLayout->addWidget(analyticsGroupBox);

    // === Формирование отчётов ===
    QGroupBox *reportsGroupBox = new QGroupBox("Сформировать отчёт");
    reportsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QVBoxLayout *reportsLayout = new QVBoxLayout(reportsGroupBox);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_dailyReportBtn = new QPushButton("📊 За смену PDF, Excel");
    m_dailyReportBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    m_weeklyReportBtn = new QPushButton("📈 За сутки PDF, Excel");
    m_weeklyReportBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    m_monthlyReportBtn = new QPushButton("📉 За неделю PDF, Excel");
    m_monthlyReportBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    m_customReportBtn = new QPushButton("📋 За месяц PDF, Excel, CSV");
    m_customReportBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    btnLayout->addWidget(m_dailyReportBtn);
    btnLayout->addWidget(m_weeklyReportBtn);
    btnLayout->addWidget(m_monthlyReportBtn);
    btnLayout->addWidget(m_customReportBtn);

    reportsLayout->addLayout(btnLayout);

    QHBoxLayout *moreBtnLayout = new QHBoxLayout;
    moreBtnLayout->addWidget(new QPushButton("Сравнить с прошлым периодом"));
    moreBtnLayout->addWidget(new QPushButton("Экспорт сырых данных (CSV)"));
    moreBtnLayout->addWidget(new QPushButton("Печать отчёта"));

    reportsLayout->addLayout(moreBtnLayout);
    reportsGroupBox->setLayout(reportsLayout);
    mainLayout->addWidget(reportsGroupBox);
}
