#include "mainscreenwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QColor>
#include <QMessageBox>
#include <QTimer>

#include "registerdialog.h"

MainScreenWidget::MainScreenWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #2B2B2B; color: #FFFFFF;");


    // === Общая структура ===
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // === Верхняя панель статуса ===
    QLabel *statusLabel = new QLabel("Система активна");
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

    // === Информация о сессии ===
    QGroupBox *sessionBox = new QGroupBox();
    sessionBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QVBoxLayout *sessionLayout = new QVBoxLayout(sessionBox);
    sessionLayout->setContentsMargins(10, 10, 10, 10);


    m_operatorLabel = new QLabel("Оператор: " + name + " (" + role + ")");
    m_operatorLabel->setStyleSheet("font-size: 13px;");

    m_startTimeLabel = new QLabel("Начало смены: " + startTime);
    m_startTimeLabel->setStyleSheet("font-size: 13px;");

    m_durationLabel = new QLabel(QString("Длительность: %1 ч %2 мин")
                                     .arg(hours).arg(minutes, 2, 10, QChar('0')));
    m_durationLabel->setStyleSheet("font-size: 13px;");
ъ

    sessionLayout->addWidget(m_operatorLabel);
    sessionLayout->addWidget(m_startTimeLabel);
    sessionLayout->addWidget(m_durationLabel);
    mainLayout->addWidget(sessionBox);

    // === Показатели ЦУДД ===
    QGridLayout *metricsLayout = new QGridLayout;
    metricsLayout->setContentsMargins(0, 0, 0, 0);
    metricsLayout->setVerticalSpacing(8);

    // Подключённые устройства
    QLabel *devicesLabel = new QLabel("Подключённые устройства");
    devicesLabel->setStyleSheet("color: #0077CC; font-weight: bold;");
    QLabel *devicesCount = new QLabel("42/45 ▲ 3 устройства активны");
    devicesCount->setStyleSheet("color: #0077CC; font-size: 12px;");
    metricsLayout->addWidget(devicesLabel, 0, 0);
    metricsLayout->addWidget(devicesCount, 0, 1);

    // Активные инциденты
    QLabel *incidentsLabel = new QLabel("Активные инциденты");
    incidentsLabel->setStyleSheet("color: #CC0000; font-weight: bold;");
    QLabel *incidentsCount = new QLabel("2 ▼ 1 с прошлого часа");
    incidentsCount->setStyleSheet("color: #CC0000; font-size: 12px;");
    metricsLayout->addWidget(incidentsLabel, 1, 0);
    metricsLayout->addWidget(incidentsCount, 1, 1);

    // Средняя скорость
    QLabel *speedLabel = new QLabel("Средняя скорость");
    speedLabel->setStyleSheet("color: #00AA00; font-weight: bold;");
    QLabel *speedCount = new QLabel("48 км/ч ▲ 5 км/ч");
    speedCount->setStyleSheet("color: #00AA00; font-size: 12px;");
    metricsLayout->addWidget(speedLabel, 2, 0);
    metricsLayout->addWidget(speedCount, 2, 1);

    // Загруженность дорог
    QLabel *loadLabel = new QLabel("Загруженность дорог");
    loadLabel->setStyleSheet("color: #FF9900; font-weight: bold;");
    QLabel *loadCount = new QLabel("72% ▼ 8% с утра");
    loadCount->setStyleSheet("color: #FF9900; font-size: 12px;");
    metricsLayout->addWidget(loadLabel, 3, 0);
    metricsLayout->addWidget(loadCount, 3, 1);

    QGroupBox *metricsBox = new QGroupBox("Центр управления дорожным движением");
    metricsBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    metricsBox->setLayout(metricsLayout);
    mainLayout->addWidget(metricsBox);

    // === Блок внимания ===
    QLabel *alertLabel = new QLabel(
        "Внимание: Обнаружено снижение скорости на Московском проспекте "
        "(участок от пл. Победы до Московских ворот). Рекомендуется проверить состояние оборудования."
        );
    alertLabel->setWordWrap(true);
    alertLabel->setStyleSheet(
        "background-color: #FFF8E1; "
        "border-left: 4px solid #FFC107; "
        "padding: 10px; "
        "color: #856404; "
        "font-weight: bold;"
        );
    mainLayout->addWidget(alertLabel);

    // === Карта (ваша SimulationView) ===
    m_simulationView = new SimulationView(this);
    m_simulationView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_simulationView->setStyleSheet("background-color: #1E1E1E;");
    mainLayout->addWidget(m_simulationView, 1); // stretch factor = 1

    // === Быстрые действия ===
    QGroupBox *actionsBox = new QGroupBox("Быстрые действия");
    actionsBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QHBoxLayout *actionsLayout = new QHBoxLayout(actionsBox);
    actionsLayout->setContentsMargins(10, 10, 10, 10);

    QPushButton *btnNewShift = new QPushButton("Начать новую смену");
    btnNewShift->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    QPushButton *btnExport = new QPushButton("Экспорт логов");
    btnExport->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    QPushButton *btnDiagnostic = new QPushButton("Диагностика системы");
    btnDiagnostic->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );


    int buttonWidth = 200;
    btnNewShift->setFixedSize(buttonWidth, 40);
    btnExport->setFixedSize(buttonWidth, 40);
    btnDiagnostic->setFixedSize(buttonWidth, 40);

    actionsLayout->addWidget(btnNewShift);
    actionsLayout->addWidget(btnExport);
    actionsLayout->addWidget(btnDiagnostic);
    mainLayout->addWidget(actionsBox);


    connect(btnNewShift, &QPushButton::clicked, this, &MainScreenWidget::onStartNewShift);
    // Загружаем
    m_simulationView->setWindowTitle("Тест");
    m_simulationView->resize(600, 400);
    m_simulationView->show();

}

void MainScreenWidget::onStartNewShift()
{
    RegisterDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        // Регистрация успешна — обновляем интерфейс
        this->name = dialog.operatorName();
        this->role = dialog.operatorRole();
        this->startTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm");

        this->m_operatorLabel->setText("Оператор: " + name + " (" + role + ")");
        this->m_startTimeLabel->setText("Начало смены: " + startTime);

        m_shiftTimer = new QTimer(this); // родитель this — таймер удалится автоматически
        m_shiftStartTime = QDateTime::currentDateTime(); // сохраняем время начала

        connect(m_shiftTimer, &QTimer::timeout, this, [this]() {
            // Этот код выполняется каждый тик таймера
            qint64 seconds = m_shiftStartTime.secsTo(QDateTime::currentDateTime());
            this->hours = seconds / 3600;
            this->minutes = (seconds % 3600) / 60;

            m_durationLabel->setText(QString("Длительность: %1 ч %2 мин")
                                         .arg(hours).arg(minutes, 2, 10, QChar('0')));
        });

        m_shiftTimer->start(60000);
        // this->durationLabel->setText("Длительность: 0 ч 0 мин");


        // Пример вывода в консоль для отладки:
        qDebug() << "Новая смена:" << name << role << startTime;

        // Показываем уведомление
        QMessageBox::information(this, "Смена начата",
                                 "Оператор: " + name + "\n"
                                                       "Роль: " + role + "\n"
                                              "Время начала: " + startTime);

        // TODO
        // 1. Сохранить данные в БД
        // 2. Сбросить статистику предыдущей смены
        // 3. Запустить таймер длительности смены
    }
}
