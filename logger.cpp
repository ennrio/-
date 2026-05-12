#include "logger.h"
#include <QTimer>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QMutex>

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

    closeLogFile();

    if (openLogFile()) {
        m_initialized = true;
        // ✅ Вызываем logInternal, потому что мьютекс уже взят!
        logInternal(LogType::System, "Logger initialized for operator: " + operatorName + ", widget: " + widgetName);
    }
}

bool Logger::openLogFile()
{
    if (m_logFile.isOpen()) {
        closeLogFile();
    }

    QString fileName = generateFileName();

    // Создаём директорию, если нет
    QFileInfo fileInfo(fileName);
    QDir dir(fileInfo.absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        qCritical() << "[Logger] ❌ Не удалось создать папку:" << dir.absolutePath();
        return false;
    }

    m_logFile.setFileName(fileName);

    if (m_logFile.open(QIODevice::Append | QIODevice::Text)) {
        // ✅ Ключевой момент: setDevice ТОЛЬКО после успешного open()
        m_logStream.setDevice(&m_logFile);
        m_currentLogFile = fileName;
        qDebug() << "[Logger] ✅ Файл открыт:" << fileName;
        return true;
    }

    qCritical() << "[Logger] ❌ Ошибка открытия:" << fileName
                << "\n   Причина:" << m_logFile.errorString();
    return false;
}

void Logger::closeLogFile()
{
    // ✅ Порядок критичен: сначала отключаем stream, потом закрываем файл
    if (m_logStream.device() != nullptr) {
        m_logStream.flush();
        m_logStream.setDevice(nullptr);  // ← 1. Отключаем stream
    }

    if (m_logFile.isOpen()) {
        m_logFile.close();  // ← 2. Закрываем файл
    }

    m_currentLogFile.clear();
}

void Logger::logInternal(LogType type, const QString &eventName)
{
    if (!m_logFile.isOpen() && !openLogFile()) {
        return;
    }

    if (m_logStream.device() == nullptr) {
        return;
    }

    QString typeStr = (type == LogType::User) ? "user" : "system";
    QString timeStr = getCurrentTimeStr();

    m_logStream << "[" << typeStr << ", " << timeStr << ", " << eventName << "]" << Qt::endl;
    m_logStream.flush();
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
    if (!m_initialized) return;

    QMutexLocker locker(&m_mutex);  // ← Берём мьютекс здесь
    logInternal(type, eventName);   // ← Вызываем внутреннюю версию
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
