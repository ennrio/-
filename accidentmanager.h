#ifndef ACCIDENTMANAGER_H
#define ACCIDENTMANAGER_H

#include <QObject>
#include <QMap>
#include <QPointF>
#include <QTimer>
#include <QRandomGenerator>

// Структура, описывающая ДТП
struct Accident {
    int id;                    // Уникальный ID ДТП
    QPointF position;          // Координаты места ДТП
    long long roadNodeId;      // ID узла дороги, где произошло ДТП
    qint64 timestamp;          // Время возникновения
    QString severity;          // "Лёгкое", "Среднее", "Тяжёлое"
    bool isActive;             // Активно ли ещё ДТП
    int affectedRadius;        // Радиус влияния (в метрах)
    QString description;       // Описание для оператора
};

class SimulationView;

class AccidentManager : public QObject
{
    Q_OBJECT

public:
    explicit AccidentManager(QObject *parent = nullptr);
    
    // Установка ссылки на симуляцию
    void setSimulationView(SimulationView *view);
    
    // Создание ДТП в случайном месте
    void createRandomAccident();
    
    // Создание ДТП в указанном месте
    void createAccident(const QPointF &position, long long nodeId, const QString &severity = "Среднее");
    
    // Удаление ДТП по ID
    void resolveAccident(int accidentId);
    
    // Получить список активных ДТП
    QList<Accident> getActiveAccidents() const;
    
    // Получить количество активных ДТП
    int getActiveAccidentsCount() const;
    
    // Настройка вероятности возникновения ДТП
    void setAccidentProbability(double probability); // 0.0 - 1.0
    
    // Включить/выключить генерацию ДТП
    void setAccidentsEnabled(bool enabled);
    
    // Проверка, влияет ли ДТП на данную позицию
    bool isPositionAffected(const QPointF &position) const;
    
    // Получить ближайшее ДТП к позиции
    Accident* getNearestAccident(const QPointF &position, qreal radius = 100.0);
    
    // Очистить все ДТП (для сброса симуляции)
    void clearAllAccidents();

signals:
    // Сигнал о новом ДТП
    void accidentCreated(int id, const QPointF &position, const QString &severity);
    
    // Сигнал об устранении ДТП
    void accidentResolved(int id);
    
    // Сигнал для обновления UI
    void accidentsUpdated();

private slots:
    // Периодическая проверка на создание случайных ДТП
    void checkAccidentSpawn();

private:
    SimulationView *m_simulationView;
    QMap<int, Accident> m_accidents;
    int m_nextAccidentId;
    double m_accidentProbability; // Вероятность spawns в единицу времени
    bool m_accidentsEnabled;
    QTimer m_spawnTimer;
    
    // Генерация описания ДТП
    QString generateDescription(const QString &severity);
};

#endif // ACCIDENTMANAGER_H
