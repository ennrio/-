#include "datageneratorwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include "simulationmanager.h"

DataGeneratorWidget::DataGeneratorWidget(QWidget *parent)
    : QWidget(parent)
    , m_isGenerating(false)
    , view(nullptr)
{
    setStyleSheet("background-color: #2B2B2B; color: #FFFFFF;");

    // Инициализируем указатель на SimulationView
    view = SimulationManager::instance().simulationView();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // === Верхняя панель статуса ===
    m_statusLabel = new QLabel("Генерация приостановлена");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(
        "font-weight: bold; "
        "color: #CC0000; "
        "background-color: #202020; "
        "border-radius: 4px; "
        "padding: 8px; "
        "font-size: 14px;"
    );
    mainLayout->addWidget(m_statusLabel);

    // === Настройка генерации тестовых данных ===
    QGroupBox *settingsGroupBox = new QGroupBox("Настройка генерации тестовых данных");
    settingsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroupBox);

    // Профили генерации
    QGroupBox *profilesGroupBox = new QGroupBox("Профили генерации");
    profilesGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QHBoxLayout *profilesLayout = new QHBoxLayout(profilesGroupBox);

    m_profileCombo = new QComboBox;
    m_profileCombo->addItems({"Профиль 1", "Профиль 2", "Профиль 3", "Профиль 4", "Профиль 5"});

    profilesLayout->addWidget(m_profileCombo);
    
    m_saveProfileBtn = new QPushButton("Сохранить профиль");
    m_saveProfileBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
    );
    connect(m_saveProfileBtn, &QPushButton::clicked, this, &DataGeneratorWidget::onSaveProfile);
    profilesLayout->addWidget(m_saveProfileBtn);
    
    m_loadProfileBtn = new QPushButton("Загрузить профиль");
    m_loadProfileBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
    );
    connect(m_loadProfileBtn, &QPushButton::clicked, this, &DataGeneratorWidget::onLoadProfile);
    profilesLayout->addWidget(m_loadProfileBtn);

    settingsLayout->addWidget(profilesGroupBox);

    // Параметры генерации
    QGroupBox *paramsGroupBox = new QGroupBox("Параметры генерации");
    paramsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QVBoxLayout *paramsLayout = new QVBoxLayout(paramsGroupBox);

    paramsLayout->addWidget(new QLabel("Интенсивность трафика:"));
    m_intensityCombo = new QComboBox;
    m_intensityCombo->addItems({"Низкая (до 500 авт/ч)", "Средняя (500-1500 авт/ч)", "Высокая (1500-3000 авт/ч)", "Очень высокая (3000+ авт/ч)"});
    paramsLayout->addWidget(m_intensityCombo);

    settingsLayout->addWidget(paramsGroupBox);

    // Особые события - только ДТП и неправильная парковка
    QGroupBox *eventsGroupBox = new QGroupBox("Особые события");
    eventsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    QVBoxLayout *eventsLayout = new QVBoxLayout(eventsGroupBox);

    m_accidentCheck = new QCheckBox("Имитировать ДТП");
    m_accidentCheck->setStyleSheet("color: #0077CC;");
    connect(m_accidentCheck, &QCheckBox::toggled, this, &DataGeneratorWidget::onAccidentToggled);
    eventsLayout->addWidget(m_accidentCheck);

    // Вероятность возникновения ДТП
    QLabel *probabilityLabel = new QLabel("Вероятность ДТП:");
    probabilityLabel->setStyleSheet("color: #FFFFFF;");
    eventsLayout->addWidget(probabilityLabel);
    
    m_probabilityCombo = new QComboBox;
    m_probabilityCombo->addItems({"0%", "5%", "10%", "15%", "20%", "25%", "30%"});
    connect(m_probabilityCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &DataGeneratorWidget::onProbabilityChanged);
    eventsLayout->addWidget(m_probabilityCombo);
    
    // Неправильная парковка
    m_parkingCheck = new QCheckBox("Имитировать неправильную парковку");
    m_parkingCheck->setStyleSheet("color: #0077CC;");
    connect(m_parkingCheck, &QCheckBox::toggled, this, &DataGeneratorWidget::onParkingToggled);
    eventsLayout->addWidget(m_parkingCheck);
    
    // Вероятность неправильной парковки
    QLabel *parkingProbabilityLabel = new QLabel("Вероятность неправильной парковки:");
    parkingProbabilityLabel->setStyleSheet("color: #FFFFFF;");
    eventsLayout->addWidget(parkingProbabilityLabel);
    
    m_parkingProbabilityCombo = new QComboBox;
    m_parkingProbabilityCombo->addItems({"0%", "5%", "10%", "15%", "20%", "25%", "30%"});
    connect(m_parkingProbabilityCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &DataGeneratorWidget::onParkingProbabilityChanged);
    eventsLayout->addWidget(m_parkingProbabilityCombo);
    
    // Статус количества активных ДТП и неправильных парковок
    m_accidentCountLabel = new QLabel("Активных ДТП: 0");
    m_accidentCountLabel->setStyleSheet("color: #CC0000; font-weight: bold; font-size: 13px;");
    eventsLayout->addWidget(m_accidentCountLabel);
    
    m_parkingCountLabel = new QLabel("Неправильных парковок: 0");
    m_parkingCountLabel->setStyleSheet("color: #0077CC; font-weight: bold; font-size: 13px;");
    eventsLayout->addWidget(m_parkingCountLabel);

    settingsLayout->addWidget(eventsGroupBox);

    // Кнопки управления
    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_startBtn = new QPushButton("Начать генерацию");
    m_startBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #00AA00; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #009900; }"
    );
    connect(m_startBtn, &QPushButton::clicked, this, &DataGeneratorWidget::startGeneration);

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
        "QPushButton:disabled { background-color: #555555; color: #888888; }"
    );
    connect(m_stopBtn, &QPushButton::clicked, this, &DataGeneratorWidget::stopGeneration);
    m_stopBtn->setEnabled(false);

    m_restartBtn = new QPushButton("Перезапустить генерацию");
    m_restartBtn->setStyleSheet(
        "QPushButton { "
        "   background-color: #FF8800; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #EE7700; }"
        "QPushButton:disabled { background-color: #555555; color: #888888; }"
    );
    connect(m_restartBtn, &QPushButton::clicked, this, &DataGeneratorWidget::restartGeneration);
    m_restartBtn->setEnabled(false);

    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addWidget(m_restartBtn);

    settingsLayout->addLayout(btnLayout);
    settingsGroupBox->setLayout(settingsLayout);
    mainLayout->addWidget(settingsGroupBox);

    // Таймер для обновления статистики ДТП
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &DataGeneratorWidget::updateAccidentCount);
    m_updateTimer->start(1000); // Обновляем каждую секунду
}

void DataGeneratorWidget::updateStatus(bool isGenerating, int activeAccidents)
{
    m_isGenerating = isGenerating;
    
    // Обновляем указатель на SimulationView при каждом вызове
    if (!view) {
        view = SimulationManager::instance().simulationView();
    }
    
    if (isGenerating) {
        m_statusLabel->setText("Генерация активна");
        m_statusLabel->setStyleSheet(
            "font-weight: bold; "
            "color: #00AA00; "
            "background-color: #202020; "
            "border-radius: 4px; "
            "padding: 8px; "
            "font-size: 14px;"
        );
        m_startBtn->setEnabled(false);
        m_stopBtn->setEnabled(true);
        m_restartBtn->setEnabled(true);
    } else {
        m_statusLabel->setText("Генерация приостановлена");
        m_statusLabel->setStyleSheet(
            "font-weight: bold; "
            "color: #CC0000; "
            "background-color: #202020; "
            "border-radius: 4px; "
            "padding: 8px; "
            "font-size: 14px;"
        );
        m_startBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
        m_restartBtn->setEnabled(false);
    }
    
    m_accidentCountLabel->setText(QString("Активных ДТП: %1").arg(activeAccidents));
    
    // Обновляем количество неправильных парковок
    int parkingCount = 0;
    if (view) {
        parkingCount = view->getWrongParkingCount();
    }
    m_parkingCountLabel->setText(QString("Неправильных парковок: %1").arg(parkingCount));
}

void DataGeneratorWidget::startGeneration()
{
    SimulationView* view = SimulationManager::instance().simulationView();
    if (view) {
        view->startSimulation();
        updateStatus(true, SimulationManager::instance().getActiveAccidentsCount());
        
        // Если включена имитация ДТП - запускаем её
        if (m_accidentCheck->isChecked()) {
            onAccidentToggled(true);
        }
    }
}

void DataGeneratorWidget::stopGeneration()
{
    SimulationView* view = SimulationManager::instance().simulationView();
    if (view) {
        view->stopSimulation();
        updateStatus(false, SimulationManager::instance().getActiveAccidentsCount());
    }
}

void DataGeneratorWidget::restartGeneration()
{
    SimulationView* view = SimulationManager::instance().simulationView();
    if (view) {
        // Полностью сбрасываем симуляцию - очищаем все данные
        view->resetSimulation();
        
        // Небольшая задержка перед перезапуском для корректной остановки таймеров
        QTimer::singleShot(100, this, [this, view]() {
            view->startSimulation();
            updateStatus(true, SimulationManager::instance().getActiveAccidentsCount());
            
            // Если включена имитация ДТП - перезапускаем её
            if (m_accidentCheck->isChecked()) {
                onAccidentToggled(true);
            }
            
            // Если включена имитация неправильной парковки - перезапускаем её
            if (m_parkingCheck->isChecked()) {
                onParkingToggled(true);
            }
            qDebug() << "Generation restarted - simulation reset complete";
        });
    }
}

void DataGeneratorWidget::onAccidentToggled(bool checked)
{
    auto& manager = SimulationManager::instance();
    if (manager.accidentManager()) {
        manager.accidentManager()->setAccidentsEnabled(checked);
        
        // Устанавливаем вероятность из комбобокса
        QString probText = m_probabilityCombo->currentText();
        onProbabilityChanged(probText);
        
        qDebug() << "Accident simulation" << (checked ? "started" : "stopped");
    }
}

void DataGeneratorWidget::onProbabilityChanged(const QString &text)
{
    auto& manager = SimulationManager::instance();
    if (manager.accidentManager()) {
        // Парсим текст вероятности (например, "10%" -> 0.1)
        double probability = 0.0;
        auto a = text;
        if (a.contains("%")) {
            probability = a.remove("%").toDouble() / 100.0;
        }
        manager.accidentManager()->setAccidentProbability(probability);
        qDebug() << "Accident probability set to:" << probability;
    }
}

void DataGeneratorWidget::onParkingToggled(bool checked)
{
    auto& manager = SimulationManager::instance();
    if (manager.simulationView()) {
        manager.simulationView()->setWrongParkingEnabled(checked);
        
        // Устанавливаем вероятность из комбобокса
        QString probText = m_parkingProbabilityCombo->currentText();
        onParkingProbabilityChanged(probText);
        
        qDebug() << "Wrong parking simulation" << (checked ? "started" : "stopped");
    }
}

void DataGeneratorWidget::onParkingProbabilityChanged(const QString &text)
{
    auto& manager = SimulationManager::instance();
    if (manager.simulationView()) {
        // Парсим текст вероятности (например, "10%" -> 0.1)
        double probability = 0.0;
        auto a = text;
        if (a.contains("%")) {
            probability = a.remove("%").toDouble() / 100.0;
        }
        manager.simulationView()->setWrongParkingProbability(probability);
        qDebug() << "Wrong parking probability set to:" << probability;
    }
}

void DataGeneratorWidget::onSaveProfile()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить профиль", 
                                                     QString(), "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    
    QJsonObject profile;
    profile["intensity"] = m_intensityCombo->currentIndex();
    profile["accidents_enabled"] = m_accidentCheck->isChecked();
    profile["accident_probability"] = m_probabilityCombo->currentIndex();
    profile["parking_enabled"] = m_parkingCheck->isChecked();
    profile["parking_probability"] = m_parkingProbabilityCombo->currentIndex();
    
    QJsonDocument doc(profile);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Profile saved to:" << fileName;
    }
}

void DataGeneratorWidget::onLoadProfile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Загрузить профиль", 
                                                     QString(), "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject profile = doc.object();
        
        if (profile.contains("intensity")) {
            m_intensityCombo->setCurrentIndex(profile["intensity"].toInt());
        }
        if (profile.contains("accidents_enabled")) {
            m_accidentCheck->setChecked(profile["accidents_enabled"].toBool());
        }
        if (profile.contains("accident_probability")) {
            m_probabilityCombo->setCurrentIndex(profile["accident_probability"].toInt());
        }
        if (profile.contains("parking_enabled")) {
            m_parkingCheck->setChecked(profile["parking_enabled"].toBool());
        }
        if (profile.contains("parking_probability")) {
            m_parkingProbabilityCombo->setCurrentIndex(profile["parking_probability"].toInt());
        }
        
        qDebug() << "Profile loaded from:" << fileName;
    }
}

void DataGeneratorWidget::updateAccidentCount()
{
    int count = SimulationManager::instance().getActiveAccidentsCount();
    m_accidentCountLabel->setText(QString("Активных ДТП: %1").arg(count));
    
    // Обновляем количество неправильных парковок
    SimulationView* view = SimulationManager::instance().simulationView();
    if (view) {
        int parkingCount = view->getWrongParkingCount();
        m_parkingCountLabel->setText(QString("Неправильных парковок: %1").arg(parkingCount));
    }
}
