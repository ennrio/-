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
{
    setStyleSheet("background-color: #2B2B2B; color: #FFFFFF;");

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
    QGridLayout *paramsLayout = new QGridLayout(paramsGroupBox);

    paramsLayout->addWidget(new QLabel("Интенсивность трафика:"), 0, 0);
    m_intensityCombo = new QComboBox;
    m_intensityCombo->addItems({"Низкая (до 500 авт/ч)", "Средняя (500-1500 авт/ч)", "Высокая (1500-3000 авт/ч)", "Очень высокая (3000+ авт/ч)"});
    paramsLayout->addWidget(m_intensityCombo, 0, 1);

    paramsLayout->addWidget(new QLabel("Интервал обновления данных (сек):"), 1, 0);
    m_intervalInput = new QLineEdit("30");
    paramsLayout->addWidget(m_intervalInput, 1, 1);

    settingsLayout->addWidget(paramsGroupBox);

    // Особые события - только ДТП
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

    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);

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
    }
    
    m_accidentCountLabel->setText(QString("Активных ДТП: %1").arg(activeAccidents));
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
        if (text.contains("%")) {
            probability = text.remove("%").toDouble() / 100.0;
        }
        manager.accidentManager()->setAccidentProbability(probability);
        qDebug() << "Accident probability set to:" << probability;
    }
}

void DataGeneratorWidget::onSaveProfile()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить профиль", 
                                                     QString(), "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    
    QJsonObject profile;
    profile["intensity"] = m_intensityCombo->currentIndex();
    profile["interval"] = m_intervalInput->text().toInt();
    profile["accidents_enabled"] = m_accidentCheck->isChecked();
    profile["accident_probability"] = m_probabilityCombo->currentIndex();
    
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
        if (profile.contains("interval")) {
            m_intervalInput->setText(QString::number(profile["interval"].toInt()));
        }
        if (profile.contains("accidents_enabled")) {
            m_accidentCheck->setChecked(profile["accidents_enabled"].toBool());
        }
        if (profile.contains("accident_probability")) {
            m_probabilityCombo->setCurrentIndex(profile["accident_probability"].toInt());
        }
        
        qDebug() << "Profile loaded from:" << fileName;
    }
}

void DataGeneratorWidget::updateAccidentCount()
{
    int count = SimulationManager::instance().getActiveAccidentsCount();
    m_accidentCountLabel->setText(QString("Активных ДТП: %1").arg(count));
}
