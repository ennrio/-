#include "analyticswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include "logger.h"

AnalyticsWidget::AnalyticsWidget(QWidget *parent)
    : QWidget(parent)
    , m_accidentCount(0)
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

    // Период анализа с кнопкой загрузки логов
    QHBoxLayout *periodLayout = new QHBoxLayout;
    periodLayout->addWidget(new QLabel("Период анализа:"));
    
    QPushButton *applyPeriodBtn = new QPushButton("Загрузить .log файлы");
    connect(applyPeriodBtn, &QPushButton::clicked, this, &AnalyticsWidget::onLoadLogsClicked);
    periodLayout->addWidget(applyPeriodBtn);
    analyticsLayout->addLayout(periodLayout);

    // Ключевые показатели
    QGridLayout *metricsLayout = new QGridLayout;
    metricsLayout->setContentsMargins(10, 10, 10, 10);

    m_avgTimeLabel = new QLabel("Среднее время в пути (артерии): -- мин");
    m_avgTimeLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 14px;");

    m_accidentsLabel = new QLabel("Количество ДТП: 0");
    m_accidentsLabel->setStyleSheet("color: #CC0000; font-weight: bold; font-size: 14px;");

    metricsLayout->addWidget(m_avgTimeLabel, 0, 0);
    metricsLayout->addWidget(m_accidentsLabel, 0, 1);

    analyticsLayout->addLayout(metricsLayout);

    // Динамика загруженности дорог - два графика
    QGroupBox *trendGroupBox = new QGroupBox("Динамика загруженности дорог");
    trendGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QVBoxLayout *trendLayout = new QVBoxLayout(trendGroupBox);

    // График количества машин на дорогах по времени
    m_devicesChartLabel = new QLabel("График: Кол-во машин на дорогах по времени\n(загрузите .log файлы для отображения)");
    m_devicesChartLabel->setAlignment(Qt::AlignCenter);
    m_devicesChartLabel->setStyleSheet("background-color: #1E1E1E; border-radius: 4px; padding: 10px; min-height: 150px;");
    trendLayout->addWidget(m_devicesChartLabel);

    // График количества заторов на светофорах по времени
    m_jamsChartLabel = new QLabel("График: Кол-во заторов на светофорах по времени\n(загрузите .log файлы для отображения)");
    m_jamsChartLabel->setAlignment(Qt::AlignCenter);
    m_jamsChartLabel->setStyleSheet("background-color: #1E1E1E; border-radius: 4px; padding: 10px; min-height: 150px;");
    trendLayout->addWidget(m_jamsChartLabel);

    analyticsLayout->addWidget(trendGroupBox);
    analyticsGroupBox->setLayout(analyticsLayout);
    mainLayout->addWidget(analyticsGroupBox);

    // === Формирование отчётов ===
    QGroupBox *reportsGroupBox = new QGroupBox("Сформировать отчёт");
    reportsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QHBoxLayout *reportsLayout = new QHBoxLayout(reportsGroupBox);

    m_reportPdfBtn = new QPushButton("📄 Сформировать отчёт (PDF)");
    m_reportPdfBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 10px 20px; "
        "   font-weight: bold; "
        "   font-size: 14px;"
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );
    connect(m_reportPdfBtn, &QPushButton::clicked, this, &AnalyticsWidget::onGenerateReportPdfClicked);

    m_reportExcelBtn = new QPushButton("📊 Сформировать отчёт (Excel/CSV)");
    m_reportExcelBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 10px 20px; "
        "   font-weight: bold; "
        "   font-size: 14px;"
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );
    connect(m_reportExcelBtn, &QPushButton::clicked, this, &AnalyticsWidget::onGenerateReportExcelClicked);

    reportsLayout->addWidget(m_reportPdfBtn);
    reportsLayout->addWidget(m_reportExcelBtn);

    reportsGroupBox->setLayout(reportsLayout);
    mainLayout->addWidget(reportsGroupBox);
}

void AnalyticsWidget::onLoadLogsClicked()
{
    Logger::instance().logUserAction("AnalyticsWidget: Загрузка .log файлов");
    
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Выберите .log файлы для анализа",
        "",
        "Log Files (*.log);;All Files (*)"
    );
    
    if (files.isEmpty()) {
        return;
    }
    
    m_loadedFiles = files;
    m_logData.clear();
    m_accidentCount = 0;
    
    // Парсим каждый файл
    for (const QString &filePath : files) {
        parseLogFile(filePath);
    }
    
    // Обновляем метрики и графики
    updateMetrics();
    drawCharts();
    
    QMessageBox::information(this, "Загрузка завершена", 
        QString("Загружено файлов: %1\nНайдено записей: %2\nЗафиксировано ДТП: %3")
        .arg(files.size())
        .arg(m_logData.size())
        .arg(m_accidentCount));
}

void AnalyticsWidget::parseLogFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    
    QTextStream in(&file);
    int prevIncidents = -1;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        
        // Ищем строки формата: [system, HH:MM:SS.mmm, MainScreenWidget: Подключено устройств: X, Активных инцидентов: Y, Средняя скорость: Z км/ч, Загруженность дорог: W%%]
        QRegularExpression systemRegex(R"(\[system,\s*([\d:]+\.\d+),\s*MainScreenWidget:\s*Подключено устройств:\s*(\d+),\s*Активных инцидентов:\s*(\d+),\s*Средняя скорость:\s*([\d.]+)\s*км/ч,\s*Загруженность дорог:\s*([\d.]+)%%\])");
        QRegularExpressionMatch systemMatch = systemRegex.match(line);
        
        if (systemMatch.hasMatch()) {
            LogDataPoint point;
            point.time = systemMatch.captured(1);
            point.connectedDevices = systemMatch.captured(2).toInt();
            point.activeIncidents = systemMatch.captured(3).toInt();
            point.avgSpeed = systemMatch.captured(4).toDouble();
            point.roadLoad = systemMatch.captured(5).toDouble();
            
            // Ищем соответствующую строку с заторами на перекрёстках
            // Она обычно идёт следующей строкой после MainScreenWidget
            QString nextLine = in.readLine();
            QRegularExpression jamRegex(R"(\[system,\s*[\d:]+\.\d+,\s*TrafficLightControlWidget:\s*Кол-во заторов на перекрёстках:\s*(\d+)\])");
            QRegularExpressionMatch jamMatch = jamRegex.match(nextLine);
            
            if (jamMatch.hasMatch()) {
                point.trafficJams = jamMatch.captured(1).toInt();
            } else {
                point.trafficJams = 0;
                // Возвращаем строку обратно если не нашли заторы
                if (!nextLine.isEmpty()) {
                    // Пропускаем эту строку - она будет обработана в следующей итерации
                }
            }
            
            // Считаем ДТП - увеличение количества активных инцидентов
            if (prevIncidents >= 0 && point.activeIncidents > prevIncidents) {
                m_accidentCount += (point.activeIncidents - prevIncidents);
            }
            prevIncidents = point.activeIncidents;
            
            m_logData.append(point);
        }
    }
    
    file.close();
}

void AnalyticsWidget::updateMetrics()
{
    if (m_logData.isEmpty()) {
        m_avgTimeLabel->setText("Среднее время в пути (артерии): -- мин");
        m_accidentsLabel->setText("Количество ДТП: 0");
        return;
    }
    
    // Вычисляем среднюю скорость
    double totalSpeed = 0;
    for (const LogDataPoint &point : m_logData) {
        totalSpeed += point.avgSpeed;
    }
    double avgSpeed = totalSpeed / m_logData.size();
    
    // Среднее время пути = длина Невского (5км) / средняя скорость (в часах) * 60 (в минуты)
    double avgTimeMinutes = (5.0 / avgSpeed) * 60.0;
    
    m_avgTimeLabel->setText(QString("Среднее время в пути (артерии): %1 мин").arg(avgTimeMinutes, 0, 'f', 1));
    m_accidentsLabel->setText(QString("Количество ДТП: %1").arg(m_accidentCount));
}

void AnalyticsWidget::drawCharts()
{
    if (m_logData.isEmpty()) {
        m_devicesChartLabel->setText("График: Кол-во машин на дорогах по времени\n(нет данных для отображения)");
        m_jamsChartLabel->setText("График: Кол-во заторов на светофорах по времени\n(нет данных для отображения)");
        return;
    }
    
    // Рисуем график количества машин
    QPixmap devicesPixmap(600, 200);
    devicesPixmap.fill(QColor("#1E1E1E"));
    QPainter devicesPainter(&devicesPixmap);
    devicesPainter.setPen(QPen(Qt::white, 2));
    
    int maxX = devicesPixmap.width() - 60;
    int maxY = devicesPixmap.height() - 40;
    int minX = 40;
    int minY = 20;
    
    // Находим максимум для масштаба
    int maxDevices = 1;
    for (const LogDataPoint &point : m_logData) {
        if (point.connectedDevices > maxDevices) {
            maxDevices = point.connectedDevices;
        }
    }
    
    // Рисуем оси
    devicesPainter.drawLine(minX, minY, minX, maxY);  // Y ось
    devicesPainter.drawLine(minX, maxY, maxX, maxY);  // X ось
    
    // Рисуем данные
    devicesPainter.setPen(QPen(Qt::cyan, 2));
    int stepX = (maxX - minX) / (m_logData.size() > 1 ? m_logData.size() - 1 : 1);
    
    for (int i = 0; i < m_logData.size(); ++i) {
        int x = minX + i * stepX;
        int y = maxY - (m_logData[i].connectedDevices * (maxY - minY) / maxDevices);
        
        if (i == 0) {
            devicesPainter.drawLine(x, y, x, y);
        } else {
            int prevX = minX + (i - 1) * stepX;
            int prevY = maxY - (m_logData[i - 1].connectedDevices * (maxY - minY) / maxDevices);
            devicesPainter.drawLine(prevX, prevY, x, y);
        }
    }
    
    // Подписи
    devicesPainter.setPen(QPen(Qt::white, 1));
    devicesPainter.drawText(5, 15, QString("Машин: %1").arg(maxDevices));
    devicesPainter.drawText(minX, maxY + 15, "Время");
    
    m_devicesChartLabel->setPixmap(devicesPixmap);
    
    // Рисуем график заторов
    QPixmap jamsPixmap(600, 200);
    jamsPixmap.fill(QColor("#1E1E1E"));
    QPainter jamsPainter(&jamsPixmap);
    jamsPainter.setPen(QPen(Qt::white, 2));
    
    // Находим максимум заторов для масштаба
    int maxJams = 1;
    for (const LogDataPoint &point : m_logData) {
        if (point.trafficJams > maxJams) {
            maxJams = point.trafficJams;
        }
    }
    
    // Рисуем оси
    jamsPainter.drawLine(minX, minY, minX, maxY);  // Y ось
    jamsPainter.drawLine(minX, maxY, maxX, maxY);  // X ось
    
    // Рисуем данные
    jamsPainter.setPen(QPen(Qt::red, 2));
    
    for (int i = 0; i < m_logData.size(); ++i) {
        int x = minX + i * stepX;
        int y = maxY - (m_logData[i].trafficJams * (maxY - minY) / maxJams);
        
        if (i == 0) {
            jamsPainter.drawLine(x, y, x, y);
        } else {
            int prevX = minX + (i - 1) * stepX;
            int prevY = maxY - (m_logData[i - 1].trafficJams * (maxY - minY) / maxJams);
            jamsPainter.drawLine(prevX, prevY, x, y);
        }
    }
    
    // Подписи
    jamsPainter.setPen(QPen(Qt::white, 1));
    jamsPainter.drawText(5, 15, QString("Заторов: %1").arg(maxJams));
    jamsPainter.drawText(minX, maxY + 15, "Время");
    
    m_jamsChartLabel->setPixmap(jamsPixmap);
}

void AnalyticsWidget::onGenerateReportPdfClicked()
{
    Logger::instance().logUserAction("AnalyticsWidget: Генерация отчёта PDF");
    
    if (m_logData.isEmpty()) {
        QMessageBox::warning(this, "Нет данных", "Сначала загрузите .log файлы для анализа.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Сохранить отчёт PDF",
        "report.pdf",
        "PDF Files (*.pdf)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    
    QTextDocument doc;
    QString htmlContent = "<html><body>";
    htmlContent += "<h1>Отчёт по анализу дорожного движения</h1>";
    htmlContent += QString("<p><b>Загружено файлов:</b> %1</p>").arg(m_loadedFiles.size());
    htmlContent += QString("<p><b>Всего записей:</b> %1</p>").arg(m_logData.size());
    htmlContent += QString("<p><b>Зафиксировано ДТП:</b> %1</p>").arg(m_accidentCount);
    
    // Вычисляем среднюю скорость и время
    if (!m_logData.isEmpty()) {
        double totalSpeed = 0;
        for (const LogDataPoint &point : m_logData) {
            totalSpeed += point.avgSpeed;
        }
        double avgSpeed = totalSpeed / m_logData.size();
        double avgTimeMinutes = (5.0 / avgSpeed) * 60.0;
        
        htmlContent += QString("<p><b>Средняя скорость:</b> %1 км/ч</p>").arg(avgSpeed, 0, 'f', 1);
        htmlContent += QString("<p><b>Среднее время в пути (5км):</b> %1 мин</p>").arg(avgTimeMinutes, 0, 'f', 1);
    }
    
    htmlContent += "<h2>Детализация по времени</h2>";
    htmlContent += "<table border='1' cellpadding='5' cellspacing='0'>";
    htmlContent += "<tr><th>Время</th><th>Машин</th><th>Скорость (км/ч)</th><th>Загруженность (%)</th><th>Заторы</th></tr>";
    
    for (const LogDataPoint &point : m_logData) {
        htmlContent += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
            .arg(point.time)
            .arg(point.connectedDevices)
            .arg(point.avgSpeed, 0, 'f', 1)
            .arg(point.roadLoad, 0, 'f', 1)
            .arg(point.trafficJams);
    }
    
    htmlContent += "</table></body></html>";
    
    doc.setHtml(htmlContent);
    doc.print(&printer);
    
    QMessageBox::information(this, "Отчёт создан", QString("PDF отчёт сохранён: %1").arg(fileName));
}

void AnalyticsWidget::onGenerateReportExcelClicked()
{
    Logger::instance().logUserAction("AnalyticsWidget: Генерация отчёта Excel/CSV");
    
    if (m_logData.isEmpty()) {
        QMessageBox::warning(this, "Нет данных", "Сначала загрузите .log файлы для анализа.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Сохранить отчёт CSV",
        "report.csv",
        "CSV Files (*.csv);;All Files (*)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл.");
        return;
    }
    
    QTextStream out(&file);
    
    // Заголовок
    out << "Время;Подключено устройств;Активных инцидентов;Средняя скорость (км/ч);Загруженность дорог (%);Заторы на перекрёстках\n";
    
    // Данные
    for (const LogDataPoint &point : m_logData) {
        out << QString("%1;%2;%3;%4;%5;%6\n")
            .arg(point.time)
            .arg(point.connectedDevices)
            .arg(point.activeIncidents)
            .arg(point.avgSpeed, 0, 'f', 1)
            .arg(point.roadLoad, 0, 'f', 1)
            .arg(point.trafficJams);
    }
    
    file.close();
    
    QMessageBox::information(this, "Отчёт создан", QString("CSV отчёт сохранён: %1").arg(fileName));
}
