#include "logger.h"
#include <QTimer>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QRegularExpression>

// Статический экземпляр singleton
static Logger* g_loggerInstance = nullptr;

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_systemLogTimer(nullptr)
{
    // Создаем директорию reports в корне проекта
    QString reportsDir = QDir::currentPath() + "/reports";
    QDir dir(reportsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

Logger::~Logger()
{
    stopSystemLogging();
    closeLogFile();
}

Logger& Logger::instance()
{
    if (!g_loggerInstance) {
        g_loggerInstance = new Logger();
    }
    return *g_loggerInstance;
}

void Logger::initialize(const QString& operatorName, const QString& widgetName)
{
    QMutexLocker locker(&m_mutex);
    
    m_operatorName = operatorName;
    m_widgetName = widgetName;
    
    // Закрываем предыдущий файл если был открыт
    closeLogFile();
    
    // Открываем новый файл
    if (openLogFile()) {
        m_initialized = true;
        logSystemEvent("Logger initialized for operator: " + operatorName + ", widget: " + widgetName);
    }
}

bool Logger::openLogFile()
{
    if (m_logFile.isOpen()) {
        closeLogFile();
    }
    
    QString fileName = generateFileName();
    m_logFile.setFileName(fileName);
    
    // Открываем в режиме добавления (Append)
    if (m_logFile.open(QIODevice::Append | QIODevice::Text)) {
        // Устанавливаем устройство для потока только после успешного открытия
        m_logStream.setDevice(&m_logFile);
        m_currentLogFile = fileName;
        return true;
    }
    
    // Если не удалось открыть файл, убеждаемся что поток не имеет устройства
    m_logStream.setDevice(nullptr);
    m_currentLogFile.clear();
    return false;
}

void Logger::closeLogFile()
{
    if (m_logFile.isOpen()) {
        m_logStream.flush();
        m_logFile.close();
    }
    m_logStream.setDevice(nullptr);
    m_currentLogFile.clear();
}

QString Logger::generateFileName() const
{
    // Формат: <widget>_<operator>_<date>.log
    // Заменяем пробелы и спецсимволы на подчеркивания
    QString cleanOperator = m_operatorName;
    cleanOperator.replace(QRegularExpression("[^a-zA-Z0-9а-яА-ЯёЁ]"), "_");
    
    QString cleanWidget = m_widgetName;
    cleanWidget.replace(QRegularExpression("[^a-zA-Z0-9а-яА-ЯёЁ]"), "_");
    
    QString dateStr = getCurrentDateStr();
    
    return QDir::currentPath() + "/reports/" + 
           cleanWidget + "_" + 
           cleanOperator + "_" + 
           dateStr + ".log";
}

QString Logger::getCurrentDateStr() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd");
}

QString Logger::getCurrentTimeStr() const
{
    return QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
}

void Logger::log(LogType type, const QString& eventName)
{
    if (!m_initialized) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // Проверяем, открыт ли файл, и пытаемся открыть если нет
    if (!m_logFile.isOpen()) {
        if (!openLogFile()) {
            // Не удалось открыть файл - выходим без записи
            return;
        }
    }
    
    // Дополнительная проверка: устройство потока должно быть валидным
    if (m_logStream.device() == nullptr) {
        return;
    }
    
    QString typeStr = (type == LogType::User) ? "user" : "system";
    QString timeStr = getCurrentTimeStr();
    
    // Формат: [тип, время, имя_кнопки]
    m_logStream << "[" << typeStr << ", " << timeStr << ", " << eventName << "]" << Qt::endl;
    m_logStream.flush();
}

void Logger::logUserAction(const QString& buttonName)
{
    log(LogType::User, buttonName);
}

void Logger::logSystemEvent(const QString& systemEvent)
{
    log(LogType::System, systemEvent);
}

void Logger::setSystemLogInterval(int intervalMs)
{
    if (!m_systemLogTimer) {
        m_systemLogTimer = new QTimer(this);
        connect(m_systemLogTimer, &QTimer::timeout, this, [this]() {
            // Периодически логируем системные параметры
            logSystemEvent("System heartbeat - active session");
        });
    }
    m_systemLogTimer->start(intervalMs);
}

void Logger::stopSystemLogging()
{
    if (m_systemLogTimer) {
        m_systemLogTimer->stop();
    }
}

QString Logger::currentLogFile() const
{
    return m_currentLogFile;
}
