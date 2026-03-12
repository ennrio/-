#include "datageneratorwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include "simulationmanager.h"

DataGeneratorWidget::DataGeneratorWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #2B2B2B; color: #FFFFFF;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // === Верхняя панель статуса ===
    QLabel *statusLabel = new QLabel("Генерация приостановлена");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "font-weight: bold; "
        "color: #CC0000; "
        "background-color: #202020; "
        "border-radius: 4px; "
        "padding: 8px; "
        "font-size: 14px;"
    );
    mainLayout->addWidget(statusLabel);

    // === Настройка генерации тестовых данных ===
    QGroupBox *settingsGroupBox = new QGroupBox("Настройка генерации тестовых данных");
    settingsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroupBox);

    // Профили генерации
    QGroupBox *profilesGroupBox = new QGroupBox("Профили генерации");
    profilesGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QHBoxLayout *profilesLayout = new QHBoxLayout(profilesGroupBox);

    m_profileCombo = new QComboBox;
    m_profileCombo->addItems({"У Утро час пик", "Д День обычный", "В Вечер час пик", "Н Ночной режим", "Ч Чрезвычайная ситуация"});

    profilesLayout->addWidget(m_profileCombo);
    profilesLayout->addWidget(new QPushButton("Сохранить профиль"));
    profilesLayout->addWidget(new QPushButton("Загрузить профиль"));

    settingsLayout->addWidget(profilesGroupBox);

    // Параметры генерации
    QGroupBox *paramsGroupBox = new QGroupBox("Параметры генерации");
    paramsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QGridLayout *paramsLayout = new QGridLayout(paramsGroupBox);

    paramsLayout->addWidget(new QLabel("Интенсивность трафика:"), 0, 0);
    m_intensityCombo = new QComboBox;
    m_intensityCombo->addItems({"Низкая (до 500 авт/ч)", "Средняя (500-1500 авт/ч)", "Высокая (1500-3000 авт/ч)", "Очень высокая (3000+ авт/ч)"});
    paramsLayout->addWidget(m_intensityCombo, 0, 1);

    paramsLayout->addWidget(new QLabel("Погодные условия:"), 1, 0);
    m_weatherCombo = new QComboBox;
    m_weatherCombo->addItems({"Ясно", "Дождь", "Снег", "Туман", "Гололёд"});
    paramsLayout->addWidget(m_weatherCombo, 1, 1);

    paramsLayout->addWidget(new QLabel("Время суток:"), 2, 0);
    m_timeCombo = new QComboBox;
    m_timeCombo->addItems({"Утро (06:00-10:00)", "День (10:00-16:00)", "Вечер (16:00-00)", "Ночь (22:00-06:00)"});
    paramsLayout->addWidget(m_timeCombo, 2, 1);

    paramsLayout->addWidget(new QLabel("Интервал обновления данных (сек):"), 3, 0);
    m_intervalInput = new QLineEdit("30");
    paramsLayout->addWidget(m_intervalInput, 3, 1);

    settingsLayout->addWidget(paramsGroupBox);

    // Особые события
    QGroupBox *eventsGroupBox = new QGroupBox("Особые события");
    eventsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QVBoxLayout *eventsLayout = new QVBoxLayout(eventsGroupBox);

    m_accidentCheck = new QCheckBox("Имитировать ДТП");
    m_accidentCheck->setStyleSheet("color: #0077CC;");
    eventsLayout->addWidget(m_accidentCheck);

    m_repairCheck = new QCheckBox("Имитировать ремонтные работы");
    m_repairCheck->setStyleSheet("color: #0077CC;");
    eventsLayout->addWidget(m_repairCheck);

    m_eventsCheck = new QCheckBox("Имитировать массовые мероприятия");
    m_eventsCheck->setStyleSheet("color: #0077CC;");
    eventsLayout->addWidget(m_eventsCheck);

    m_equipmentCheck = new QCheckBox("Имитировать неисправность оборудования");
    m_equipmentCheck->setStyleSheet("color: #0077CC;");
    eventsLayout->addWidget(m_equipmentCheck);

    m_frequencyCombo = new QComboBox;
    m_frequencyCombo->addItems({"Редко (1 событие в час)", "Средне (2-3 события в час)", "Часто (4+ событий в час)"});
    eventsLayout->addWidget(m_frequencyCombo);

    settingsLayout->addWidget(eventsGroupBox);

    // Источники данных
    QGroupBox *sourcesGroupBox = new QGroupBox("Источники данных");
    sourcesGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QVBoxLayout *sourcesLayout = new QVBoxLayout(sourcesGroupBox);

    sourcesLayout->addWidget(new QLabel("Камеры видеонаблюдения 42 устройства (имитация)"));
    sourcesLayout->addWidget(new QLabel("Датчики движения 18 устройств (имитация)"));
    sourcesLayout->addWidget(new QLabel("GPS-трекеры транспорта Отключено (имитация)"));
    sourcesLayout->addWidget(new QLabel("Метеостанция Активна (имитация)"));

    settingsLayout->addWidget(sourcesGroupBox);

    // Кнопки управления
    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_startBtn = new QPushButton("Начать генерацию");
    m_startBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
    );

    m_stopBtn = new QPushButton("Остановить генерацию");
    m_stopBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #CC0000; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #AA0000; }"
    );

    m_generateBtn = new QPushButton("Сгенерировать тестовый набор");
    m_generateBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
    );

    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addWidget(m_generateBtn);


    settingsLayout->addLayout(btnLayout);
    settingsGroupBox->setLayout(settingsLayout);
    mainLayout->addWidget(settingsGroupBox);

    connect(m_stopBtn, &QPushButton::clicked, this, &DataGeneratorWidget::stopGeneration);
}

void DataGeneratorWidget::stopGeneration()
{
    SimulationView* view = SimulationManager::instance().simulationView();
    if (view) {
        view->stopSimulation();
    }
}
