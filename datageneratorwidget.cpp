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
    connect(m_accidentCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            onStartAccidentSimulation();
        } else {
            onStopAccidentSimulation();
        }
    });
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
    connect(m_frequencyCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &DataGeneratorWidget::onProbabilityChanged);
    eventsLayout->addWidget(m_frequencyCombo);
    
    // Вероятность возникновения ДТП
    QLabel *probabilityLabel = new QLabel("Вероятность ДТП:");
    probabilityLabel->setStyleSheet("color: #FFFFFF;");
    eventsLayout->addWidget(probabilityLabel);
    
    m_probabilityCombo = new QComboBox;
    m_probabilityCombo->addItems({"0%", "5%", "10%", "15%", "20%", "25%", "30%"});
    connect(m_probabilityCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &DataGeneratorWidget::onProbabilityChanged);
    eventsLayout->addWidget(m_probabilityCombo);
    
    // Статус количества активных ДТП
    m_accidentCountLabel = new QLabel("Активных ДТП: 0");
    m_accidentCountLabel->setStyleSheet("color: #CC0000; font-weight: bold; font-size: 13px;");
    eventsLayout->addWidget(m_accidentCountLabel);

    settingsLayout->addWidget(eventsGroupBox);

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
    
    m_createAccidentBtn = new QPushButton("Создать ДТП вручную");
    m_createAccidentBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #CC0000; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #AA0000; }"
    );
    connect(m_createAccidentBtn, &QPushButton::clicked, this, &DataGeneratorWidget::onCreateAccident);

    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addWidget(m_generateBtn);
    btnLayout->addWidget(m_createAccidentBtn);


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

void DataGeneratorWidget::onStartAccidentSimulation()
{
    auto& manager = SimulationManager::instance();
    if (manager.accidentManager()) {
        manager.accidentManager()->setAccidentsEnabled(true);
        
        // Устанавливаем вероятность из комбобокса
        QString probText = m_probabilityCombo->currentText();
        onProbabilityChanged(probText);
        
        qDebug() << "Accident simulation started";
    }
}

void DataGeneratorWidget::onStopAccidentSimulation()
{
    auto& manager = SimulationManager::instance();
    if (manager.accidentManager()) {
        manager.accidentManager()->setAccidentsEnabled(false);
        qDebug() << "Accident simulation stopped";
    }
}

void DataGeneratorWidget::onCreateAccident()
{
    auto& manager = SimulationManager::instance();
    SimulationView* view = manager.simulationView();
    
    if (view && manager.accidentManager()) {
        // Создаём ДТП в случайном месте
        manager.createAccident(QPointF(), -1, "Среднее");
        qDebug() << "Manual accident created";
    }
}

void DataGeneratorWidget::onProbabilityChanged(const QString &text)
{
    auto& manager = SimulationManager::instance();
    if (manager.accidentManager()) {
        // Парсим текст вероятности (например, "10%" -> 0.1)
        double probability = 0.0;
        if (text.contains("%")) {
            probability = text.remove("%").toDouble() / 100.0;
        }
        manager.accidentManager()->setAccidentProbability(probability);
        qDebug() << "Accident probability set to:" << probability;
    }
}
