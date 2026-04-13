#include "accidentmanager.h"
#include "visualization/simulationview.h"
#include <QDebug>
#include <QDateTime>

AccidentManager::AccidentManager(QObject *parent)
    : QObject(parent)
    , m_simulationView(nullptr)
    , m_nextAccidentId(1)
    , m_accidentProbability(0.0) // По умолчанию ДТП не создаются автоматически
    , m_accidentsEnabled(false)
{
    // Таймер для проверки случайных событий (каждые 10 секунд)
    m_spawnTimer.setInterval(10000);
    connect(&m_spawnTimer, &QTimer::timeout, this, &AccidentManager::checkAccidentSpawn);
}

void AccidentManager::setSimulationView(SimulationView *view)
{
    m_simulationView = view;
}

void AccidentManager::createRandomAccident()
{
    if (!m_simulationView || !m_accidentsEnabled) {
        return;
    }
    
    // Получаем случайную позицию из доступных узлов дороги
    auto nodePositions = m_simulationView->getAllNodePositions();
    if (nodePositions.isEmpty()) {
        qWarning() << "AccidentManager: No road nodes available for accident spawn";
        return;
    }
    
    // Выбираем случайный узел
    int randomIndex = QRandomGenerator::global()->bounded(nodePositions.size());
    long long nodeId = nodePositions.keys().at(randomIndex);
    QPointF position = nodePositions.value(nodeId);
    
    // Генерируем тяжесть ДТП
    QStringList severities = {"Лёгкое", "Среднее", "Тяжёлое"};
    QString severity = severities.at(QRandomGenerator::global()->bounded(severities.size()));
    
    createAccident(position, nodeId, severity);
}

void AccidentManager::createAccident(const QPointF &position, long long nodeId, const QString &severity)
{
    Accident accident;
    accident.id = m_nextAccidentId++;
    accident.position = position;
    accident.roadNodeId = nodeId;
    accident.timestamp = QDateTime::currentMSecsSinceEpoch();
    accident.severity = severity;
    accident.isActive = true;
    accident.description = generateDescription(severity);
    
    // Радиус влияния в зависимости от тяжести
    if (severity == "Лёгкое") {
        accident.affectedRadius = 50;   // 50 метров
    } else if (severity == "Среднее") {
        accident.affectedRadius = 100;  // 100 метров
    } else { // Тяжёлое
        accident.affectedRadius = 200;  // 200 метров
    }
    
    m_accidents[accident.id] = accident;
    
    qDebug() << "Accident created:" << accident.id 
             << "Position:" << position 
             << "Severity:" << severity
             << "Description:" << accident.description;
    
    emit accidentCreated(accident.id, position, severity);
    emit accidentsUpdated();
    
    // Если это первое активное ДТП — запускаем таймер спавна
    if (m_accidentsEnabled && !m_spawnTimer.isActive()) {
        m_spawnTimer.start();
    }
}

void AccidentManager::resolveAccident(int accidentId)
{
    if (!m_accidents.contains(accidentId)) {
        qWarning() << "AccidentManager: Accident" << accidentId << "not found";
        return;
    }
    
    m_accidents[accidentId].isActive = false;
    
    qDebug() << "Accident resolved:" << accidentId;
    
    emit accidentResolved(accidentId);
    emit accidentsUpdated();
    
    // Удаляем старые неактивные ДТП (старше 1 часа)
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 oneHourMs = 60 * 60 * 1000;
    
    QList<int> toRemove;
    for (auto it = m_accidents.begin(); it != m_accidents.end(); ++it) {
        if (!it->isActive && (currentTime - it->timestamp > oneHourMs)) {
            toRemove.append(it.key());
        }
    }
    
    for (int id : toRemove) {
        m_accidents.remove(id);
    }
}

QList<Accident> AccidentManager::getActiveAccidents() const
{
    QList<Accident> result;
    for (const Accident &accident : m_accidents) {
        if (accident.isActive) {
            result.append(accident);
        }
    }
    return result;
}

int AccidentManager::getActiveAccidentsCount() const
{
    int count = 0;
    for (const Accident &accident : m_accidents) {
        if (accident.isActive) {
            count++;
        }
    }
    return count;
}

void AccidentManager::setAccidentProbability(double probability)
{
    m_accidentProbability = qBound(0.0, probability, 1.0);
    qDebug() << "Accident probability set to:" << m_accidentProbability;
}

void AccidentManager::setAccidentsEnabled(bool enabled)
{
    m_accidentsEnabled = enabled;
    if (enabled && !m_spawnTimer.isActive()) {
        m_spawnTimer.start();
    } else if (!enabled && m_spawnTimer.isActive()) {
        m_spawnTimer.stop();
    }
    qDebug() << "Accidents" << (enabled ? "enabled" : "disabled");
}

bool AccidentManager::isPositionAffected(const QPointF &position) const
{
    for (const Accident &accident : m_accidents) {
        if (accident.isActive) {
            qreal distance = QLineF(position, accident.position).length();
            if (distance <= accident.affectedRadius) {
                return true;
            }
        }
    }
    return false;
}

Accident* AccidentManager::getNearestAccident(const QPointF &position, qreal radius)
{
    Accident *nearest = nullptr;
    qreal minDistance = radius;
    
    for (auto it = m_accidents.begin(); it != m_accidents.end(); ++it) {
        if (it->isActive) {
            qreal distance = QLineF(position, it->position).length();
            if (distance < minDistance) {
                minDistance = distance;
                nearest = &it.value();
            }
        }
    }
    
    return nearest;
}

void AccidentManager::clearAllAccidents()
{
    // Очищаем все ДТП
    m_accidents.clear();
    
    // Останавливаем таймер спавна
    if (m_spawnTimer.isActive()) {
        m_spawnTimer.stop();
    }
    
    qDebug() << "All accidents cleared";
    
    emit accidentsUpdated();
}

void AccidentManager::checkAccidentSpawn()
{
    if (!m_accidentsEnabled || !m_simulationView) {
        return;
    }
    
    // Проверяем вероятность создания ДТП
    double randomValue = QRandomGenerator::global()->generateDouble();
    if (randomValue < m_accidentProbability) {
        createRandomAccident();
    }
}

QString AccidentManager::generateDescription(const QString &severity)
{
    QStringList lightDescriptions = {
        "Мелкое столкновение без пострадавших",
        "Царапины на кузове",
        "Незначительное повреждение бампера"
    };
    
    QStringList mediumDescriptions = {
        "Столкновение двух транспортных средств",
        "Повреждение ходовой части",
        "Блокировка одной полосы движения"
    };
    
    QStringList heavyDescriptions = {
        "Массовое ДТП с участием нескольких автомобилей",
        "Тяжёлое столкновение с пострадавшими",
        "Полная блокировка дорожного движения"
    };
    
    if (severity == "Лёгкое") {
        return lightDescriptions.at(QRandomGenerator::global()->bounded(lightDescriptions.size()));
    } else if (severity == "Среднее") {
        return mediumDescriptions.at(QRandomGenerator::global()->bounded(mediumDescriptions.size()));
    } else {
        return heavyDescriptions.at(QRandomGenerator::global()->bounded(heavyDescriptions.size()));
    }
}
