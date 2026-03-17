#include "trafficlightcontrolwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QMessageBox>
#include "constants.h"
#include <QDebug>

TrafficLightControlWidget::TrafficLightControlWidget(QWidget *parent)
    : QWidget(parent)
    , sv(nullptr)
    , m_selectedCrossingId(-1)
{
    setStyleSheet("background-color: #2B2B2B; color: #FFFFFF;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // === 1. Статус ===
    statusLabel = new QLabel("Ручной режим");
    statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusLabel->setMinimumHeight(40);
    statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statusLabel->setStyleSheet(
        "font-weight: bold; "
        "color: #FFAA00; "
        "background-color: #202020; "
        "border-radius: 4px; "
        "padding: 8px; "
        "font-size: 14px;"
        );
    mainLayout->addWidget(statusLabel);

    // === 2. Выбор режима ===
    QHBoxLayout *modeLayout = new QHBoxLayout;
    modeLayout->addWidget(new QLabel("Режим работы:"));

    m_modeComboBox = new QComboBox;
    m_modeComboBox->addItems({"Ручной режим", "Автоматический (рекомендуется)", "Ночной режим"});
    m_modeComboBox->setMaximumWidth(400);
    modeLayout->addWidget(m_modeComboBox);
    modeLayout->addStretch();
    mainLayout->addLayout(modeLayout);

    connect(m_modeComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &TrafficLightControlWidget::onModeChanged);

    // === 3. Управление объектами (ОДИН СПИСОК) ===
    m_controlGroupBox = new QGroupBox("Управление светофорными объектами");
    m_controlGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");

    QVBoxLayout *controlLayout = new QVBoxLayout(m_controlGroupBox);

    m_crossingList = new QListWidget;
    m_crossingList->setStyleSheet(
        "QListWidget { background-color: #303030; border: 1px solid #404040; color: white; } "
        "QListWidget::item { padding: 5px; border-bottom: 1px solid #444; } "
        "QListWidget::item:selected { background-color: #005577; } "
        "QListWidget::item:hover { background-color: #3a3a3a; }"
        );
    //m_crossingList->setMaximumHeight(200);
    connect(m_crossingList, &QListWidget::itemClicked, this, &TrafficLightControlWidget::onCrossingSelected);

    controlLayout->addWidget(m_crossingList);
    m_controlGroupBox->setLayout(controlLayout);

    // === 4. Параметры светофора ===
    m_paramsGroupBox = new QGroupBox("Параметры светофора");
    m_paramsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");

    QVBoxLayout *paramsLayout = new QVBoxLayout(m_paramsGroupBox);

    // Длительность фаз
    QGridLayout *phasesLayout = new QGridLayout;
    phasesLayout->addWidget(new QLabel("Длительность зелёного (сек):"), 0, 0);
    m_greenInput = new QLineEdit("30");
    m_greenInput->setEnabled(false);
    phasesLayout->addWidget(m_greenInput, 0, 1);

    phasesLayout->addWidget(new QLabel("Длительность жёлтого (сек):"), 1, 0);
    m_yellowInput = new QLineEdit("3");
    m_yellowInput->setEnabled(false);
    phasesLayout->addWidget(m_yellowInput, 1, 1);

    phasesLayout->addWidget(new QLabel("Длительность красного (сек):"), 2, 0);
    m_redInput = new QLineEdit("30");
    m_redInput->setEnabled(false);
    phasesLayout->addWidget(m_redInput, 2, 1);
    paramsLayout->addLayout(phasesLayout);

    // Приоритетные направления
    m_priorityGroupBox = new QGroupBox("Приоритетные направления:");
    m_priorityGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");

    QVBoxLayout *priorityLayout = new QVBoxLayout(m_priorityGroupBox);
    m_priorityList = new QListWidget;
    m_priorityList->setStyleSheet("background-color: #303030; border: 1px solid #404040;");
    m_priorityList->setMaximumHeight(100);
    m_priorityList->setEnabled(false);
    priorityLayout->addWidget(m_priorityList);
    paramsLayout->addWidget(m_priorityGroupBox);

    // Кнопки
    buttonsContainer = new QWidget;
    buttonsContainer->setContentsMargins(0, 0, 0, 0);

    btnLayout = new QHBoxLayout(buttonsContainer);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    m_applyButton = new QPushButton("Применить изменения");
    m_applyButton->setEnabled(false);
    m_resetButton = new QPushButton("Сбросить");
    m_resetButton->setEnabled(false);

    connect(m_applyButton, &QPushButton::clicked, this, &TrafficLightControlWidget::onApplyClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &TrafficLightControlWidget::onResetClicked);

    QString btnStyle = "QPushButton { background-color: #0077CC; color: white; border-radius: 4px; padding: 8px 16px; font-weight: bold; } "
                       "QPushButton:hover { background-color: #0066BB; } "
                       "QPushButton:disabled { background-color: #555555; color: #888888; }";
    m_applyButton->setStyleSheet(btnStyle);
    m_resetButton->setStyleSheet(btnStyle);

    btnLayout->addWidget(m_applyButton);
    btnLayout->addWidget(m_resetButton);

    mainLayout->addWidget(m_controlGroupBox);
    mainLayout->addWidget(m_paramsGroupBox);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonsContainer);


    // === ПОДКЛЮЧЕНИЕ К SIMULATIONVIEW ===
    // Находим SimulationView через родительский виджет
    sv = qobject_cast<SimulationView*>(parent);
    if (!sv) {
        // Пробуем найти через иерархию
        sv = parent->findChild<SimulationView*>();
    }

    if (sv) {
        connect(sv, &SimulationView::trafficLightStatusChanged,
                this, &TrafficLightControlWidget::updateCrossingStatus);

        // Синхронизируем после загрузки OSM
        connect(sv, &SimulationView::osmLoadingFinished,
                this, &TrafficLightControlWidget::syncWithSimulation);

        // Первичная синхронизация
        syncWithSimulation();
    }

    onModeChanged(m_modeComboBox->currentText());
}

void TrafficLightControlWidget::syncWithSimulation()
{
    if (!sv) return;

    m_crossingList->clear();
    m_crossingsMap.clear();

    auto list = sv->getTrafficLightsList();

    qDebug() << "[SYNC] Found" << list.size() << "traffic lights";

    for (auto it = list.begin(); it != list.end(); ++it) {
        long long id = it.key();
        CrossingInfo info = it.value();

        m_crossingsMap[id] = info;

        QListWidgetItem *item = new QListWidgetItem(info.name);
        item->setData(Qt::UserRole, id);
        m_crossingList->addItem(item);
        updateCrossingListItem(id);
    }
    sortCrossingList();
}

void TrafficLightControlWidget::onModeChanged(const QString &modeText)
{
    QString newText;
    QString colorCode;
    QString bgColor = "#202020";

    bool isNight = modeText.contains("Ночной");
    bool isAuto = modeText.contains("Автоматический");
    bool isManual = modeText.contains("Ручной");

    // Логика видимости
    if (isNight || isAuto) {
        m_controlGroupBox->hide();
        m_paramsGroupBox->hide();
        buttonsContainer->hide();
    } else {
        m_controlGroupBox->show();
        m_paramsGroupBox->show();
        buttonsContainer->show();
    }

    // Логика текста и цвета
    if (isAuto) {
        newText = "Автоматический режим";
        colorCode = "#00AA00";
        if(sv) sv->setLightMode(LightMode::autoMode);
    }
    else if (isManual) {
        newText = "Ручное управление";
        colorCode = "#FFAA00";
        if(sv) sv->setLightMode(LightMode::manualMode);
    }
    else if (isNight) {
        newText = "Ночной режим";
        colorCode = "#00AAFF";
        if(sv) sv->setLightMode(LightMode::nightMode);
    }

    statusLabel->setText(newText);
    statusLabel->setStyleSheet(
        "font-weight: bold; "
        "color: " + colorCode + "; "
                      "background-color: " + bgColor + "; "
                    "border-radius: 4px; "
                    "padding: 8px; "
                    "font-size: 14px;"
        );

    layout()->update();
}

void TrafficLightControlWidget::onCrossingSelected()
{
    QListWidgetItem *item = m_crossingList->currentItem();
    if (!item) return;

    long long id = item->data(Qt::UserRole).toLongLong();
    m_selectedCrossingId = id;

    if (m_crossingsMap.contains(id)) {
        m_paramsGroupBox->setTitle("Параметры светофора: " + m_crossingsMap[id].name);
        loadCrossingParams(id);
        m_greenInput->setEnabled(true);
        m_yellowInput->setEnabled(true);
        m_redInput->setEnabled(true);
        m_priorityList->setEnabled(true);
        m_applyButton->setEnabled(true);
        m_resetButton->setEnabled(true);
    }
}

void TrafficLightControlWidget::updateCrossingStatus(long long tlId, bool requiresAttention)
{
    if (!m_crossingsMap.contains(tlId)) {
        CrossingInfo info;
        info.id = tlId;
        info.name = QString("Светофор #%1").arg(tlId);
        info.requiresAttention = requiresAttention;
        m_crossingsMap[tlId] = info;

        QListWidgetItem *item = new QListWidgetItem(info.name);
        item->setData(Qt::UserRole, tlId);
        m_crossingList->addItem(item);
    } else {
        m_crossingsMap[tlId].requiresAttention = requiresAttention;
    }

    updateCrossingListItem(tlId);
    sortCrossingList();
}

void TrafficLightControlWidget::onApplyClicked()
{
    if (!m_selectedCrossingId || !sv) {
        QMessageBox::warning(this, "Ошибка", "Светофор не выбран");
        return;
    }

    int green = m_greenInput->text().toInt();
    int yellow = m_yellowInput->text().toInt();
    int red = m_redInput->text().toInt();

    // Валидация
    if (green < 5 || green > 120) {
        QMessageBox::warning(this, "Ошибка", "Зелёный сигнал должен быть от 5 до 120 секунд");
        return;
    }
    if (yellow < 2 || yellow > 10) {
        QMessageBox::warning(this, "Ошибка", "Жёлтый сигнал должен быть от 2 до 10 секунд");
        return;
    }
    if (red < 5 || red > 120) {
        QMessageBox::warning(this, "Ошибка", "Красный сигнал должен быть от 5 до 120 секунд");
        return;
    }

    // Отправляем в SimulationView (в миллисекундах)
    sv->setTrafficLightCycle(m_selectedCrossingId, green * 1000, yellow * 1000, red * 1000);

    QMessageBox::information(this, "Применено",
                             QString("Параметры для %1 обновлены:\n"
                                     "🟢 Зелёный: %2 сек\n"
                                     "🟡 Жёлтый: %3 сек\n"
                                     "🔴 Красный: %4 сек")
                                 .arg(m_crossingsMap[m_selectedCrossingId].name)
                                 .arg(green).arg(yellow).arg(red));

    qDebug() << "[APPLY] TL" << m_selectedCrossingId
             << "G:" << green << "s Y:" << yellow << "s R:" << red;
}

void TrafficLightControlWidget::onResetClicked()
{
    if (m_selectedCrossingId < 0 || !sv) {
        QMessageBox::warning(this, "Ошибка", "Светофор не выбран");
        return;
    }

    // Сбрасываем к стандартным значениям (30/3/30)
    sv->resetTrafficLightCycle(m_selectedCrossingId);

    m_greenInput->setText("30");
    m_yellowInput->setText("3");
    m_redInput->setText("30");

    QMessageBox::information(this, "Сброшено",
                             "Параметры светофора возвращены к значениям по умолчанию:\n"
                             "🟢 Зелёный: 30 сек\n"
                             "🟡 Жёлтый: 3 сек\n"
                             "🔴 Красный: 30 сек");

    qDebug() << "[RESET] TL" << m_selectedCrossingId << "reset to default";
}

void TrafficLightControlWidget::updateCrossingListItem(long long id)
{
    if (!m_crossingsMap.contains(id)) return;

    const CrossingInfo &info = m_crossingsMap[id];

    for (int i = 0; i < m_crossingList->count(); ++i) {
        QListWidgetItem *item = m_crossingList->item(i);
        if (item->data(Qt::UserRole).toLongLong() == id) {
            QString text = info.name;
            if (info.requiresAttention) {
                text += " ⚠️";
                item->setForeground(QColor("#FF3333"));
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            } else {
                item->setForeground(QColor("#00AA00"));
                QFont f = item->font();
                f.setBold(false);
                item->setFont(f);
            }
            item->setText(text);
            break;
        }
    }
}

void TrafficLightControlWidget::loadCrossingParams(long long id)
{
    if (sv) {
        auto params = sv->getTrafficLightCycle(id);
        m_greenInput->setText(QString::number(params.green / 1000));
        m_yellowInput->setText(QString::number(params.yellow / 1000));
        m_redInput->setText(QString::number(params.red / 1000));
    } else {
        m_greenInput->setText("30");
        m_yellowInput->setText("3");
        m_redInput->setText("30");
    }

    m_priorityList->clear();
    m_priorityList->addItem("Основное направление (Север-Юг)");
    m_priorityList->addItem("Второстепенное (Запад-Восток)");
}

void TrafficLightControlWidget::sortCrossingList()
{
    QList<QPair<QListWidgetItem*, bool>> itemsWithStatus;

    while (m_crossingList->count() > 0) {
        QListWidgetItem *item = m_crossingList->takeItem(0);  // Извлекаем, не удаляя
        long long id = item->data(Qt::UserRole).toLongLong();
        bool requiresAttention = m_crossingsMap.value(id).requiresAttention;
        itemsWithStatus.append(qMakePair(item, requiresAttention));
    }

    // Сортируем: true (требует внимания) перед false
    std::sort(itemsWithStatus.begin(), itemsWithStatus.end(),
              [](const QPair<QListWidgetItem*, bool>& a, const QPair<QListWidgetItem*, bool>& b) {
                  return a.second > b.second;
              });

    // Возвращаем элементы в отсортированном порядке
    for (const auto& pair : itemsWithStatus) {
        m_crossingList->addItem(pair.first);
    }
}
