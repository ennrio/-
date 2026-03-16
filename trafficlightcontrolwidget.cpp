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
    m_crossingList->setMaximumHeight(200);
    connect(m_crossingList, &QListWidget::itemClicked, this, &TrafficLightControlWidget::onCrossingSelected);

    controlLayout->addWidget(m_crossingList);
    m_controlGroupBox->setLayout(controlLayout);
    mainLayout->addWidget(m_controlGroupBox);

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
    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_applyButton = new QPushButton("Применить изменения");
    m_applyButton->setEnabled(false);
    m_resetButton = new QPushButton("Сбросить");
    m_resetButton->setEnabled(false);
    m_diagnosticsButton = new QPushButton("Диагностика");

    QString btnStyle = "QPushButton { background-color: #0077CC; color: white; border-radius: 4px; padding: 8px 16px; font-weight: bold; } "
                       "QPushButton:hover { background-color: #0066BB; } "
                       "QPushButton:disabled { background-color: #555555; color: #888888; }";
    m_applyButton->setStyleSheet(btnStyle);
    m_resetButton->setStyleSheet(btnStyle);
    m_diagnosticsButton->setStyleSheet(btnStyle);

    btnLayout->addWidget(m_applyButton);
    btnLayout->addWidget(m_resetButton);
    btnLayout->addWidget(m_diagnosticsButton);
    paramsLayout->addLayout(btnLayout);

    m_paramsGroupBox->setLayout(paramsLayout);
    mainLayout->addWidget(m_paramsGroupBox);

    mainLayout->addStretch();

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
    } else {
        m_controlGroupBox->show();
        if (m_selectedCrossingId >= 0) m_paramsGroupBox->show();
        else m_paramsGroupBox->hide();
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
    m_greenInput->setText("30");
    m_yellowInput->setText("3");
    m_redInput->setText("30");

    m_priorityList->clear();
    m_priorityList->addItem("Основное направление (Север-Юг)");
    m_priorityList->addItem("Второстепенное (Запад-Восток)");
}
