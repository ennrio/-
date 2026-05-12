#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDir>

/**
 * @brief Класс для логирования действий пользователя и системных событий
 * 
 * Создает log-файлы в папке reports/ с именем формата:
 * <имя_виджета>_<оператор>_<дата>.log
 * 
 * Формат записи: [тип, время, имя_кнопки/события]
 * Типы событий:
 * - user - действия пользователя (нажатия кнопок и т.д.)
 * - system - системные события (параметры системы)
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    enum class LogType
    {
        User,    // Действия пользователя
        System   // Системные параметры
    };

    /**
     * @brief Получить единственный экземпляр логгера (Singleton)
     */
    static Logger& instance();

    /**
     * @brief Инициализировать логгер с данными оператора
     * @param operatorName Имя оператора
     * @param widgetName Имя виджета/экрана
     */
    void initialize(const QString& operatorName, const QString& widgetName);

    /**
     * @brief Записать событие в лог
     * @param type Тип события (user/system)
     * @param eventName Имя события (кнопки, действия)
     */
    void log(LogType type, const QString& eventName);

    /**
     * @brief Записать действие пользователя (удобная обертка)
     * @param buttonName Имя кнопки или действия
     */
    void logUserAction(const QString& buttonName);

    /**
     * @brief Записать системное событие (удобная обертка)
     * @param systemEvent Название системного события
     */
    void logSystemEvent(const QString& systemEvent);

    /**
     * @brief Установить интервал для периодических системных логов
     * @param intervalMs Интервал в миллисекундах
     */
    void setSystemLogInterval(int intervalMs);

    /**
     * @brief Остановить периодическое логирование системы
     */
    void stopSystemLogging();

    /**
     * @brief Получить текущее имя файла лога
     */
    QString currentLogFile() const;

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger();
    
    // Запрет копирования
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Открыть файл лога для записи
     */
    bool openLogFile();

    /**
     * @brief Закрыть текущий файл лога
     */
    void closeLogFile();

    /**
     * @brief Сформировать имя файла лога
     */
    QString generateFileName() const;

    /**
     * @brief Получить текущую дату в формате для имени файла
     */
    QString getCurrentDateStr() const;

    /**
     * @brief Получить текущее время в формате для лога
     */
    QString getCurrentTimeStr() const;

private:
    QFile m_logFile;
    QTextStream m_logStream;
    QString m_operatorName;
    QString m_widgetName;
    QString m_currentLogFile;
    QMutex m_mutex;
    bool m_initialized;
    
    // Для периодического системного логирования
    QTimer* m_systemLogTimer;
};

#endif // LOGGER_H
