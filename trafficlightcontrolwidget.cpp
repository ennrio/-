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

    // === 2. Выбор режима + Сортировка ===
    QHBoxLayout *modeLayout = new QHBoxLayout;
    modeLayout->addWidget(new QLabel("Режим работы:"));

    m_modeComboBox = new QComboBox;
    m_modeComboBox->addItems({"Ручной режим", "Автоматический (рекомендуется)", "Ночной режим"});
    m_modeComboBox->setMaximumWidth(400);
    modeLayout->addWidget(m_modeComboBox, 1);

    m_sortByStatusCheckBox = new QCheckBox("⚠️ Сначала 'Требует внимания'");
    m_sortByStatusCheckBox->setChecked(true);
    m_sortByStatusCheckBox->setStyleSheet("color: #FFAA00; font-weight: bold;");
    modeLayout->addWidget(m_sortByStatusCheckBox);

    connect(m_sortByStatusCheckBox, &QCheckBox::stateChanged,
            this, &TrafficLightControlWidget::onSortByStatusChanged);

    connect(m_modeComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &TrafficLightControlWidget::onModeChanged);

    modeLayout->addStretch();
    mainLayout->addLayout(modeLayout);

    // === 3. Управление объектами ===
    m_controlGroupBox = new QGroupBox("Управление светофорными объектами");
    m_controlGroupBox->setStyleSheet(
        "QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }"
        );
    m_controlGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *controlLayout = new QVBoxLayout(m_controlGroupBox);

    m_crossingList = new QListWidget;
    m_crossingList->setStyleSheet(
        "QListWidget { background-color: #303030; border: 1px solid #404040; color: white; "
        "font-size: 13px; } "
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #444; } "
        "QListWidget::item:selected { background-color: #005577; } "
        "QListWidget::item:hover { background-color: #3a3a3a; }"
        );
    m_crossingList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_crossingList->setMinimumHeight(200);

    connect(m_crossingList, &QListWidget::itemClicked, this, &TrafficLightControlWidget::onCrossingSelected);

    controlLayout->addWidget(m_crossingList, 1);
    m_controlGroupBox->setLayout(controlLayout);
    mainLayout->addWidget(m_controlGroupBox, 2);

    // === 4. Параметры светофора ===
    m_paramsGroupBox = new QGroupBox("Параметры светофора");
    m_paramsGroupBox->setStyleSheet(
        "QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }"
        );
    m_paramsGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *paramsLayout = new QVBoxLayout(m_paramsGroupBox);

    // Длительность фаз
    QGridLayout *phasesLayout = new QGridLayout;
    phasesLayout->addWidget(new QLabel("Длительность зелёного (сек):"), 0, 0);
    m_greenInput = new QLineEdit("30");
    m_greenInput->setEnabled(false);
    m_greenInput->setMaximumWidth(100);
    phasesLayout->addWidget(m_greenInput, 0, 1);

    phasesLayout->addWidget(new QLabel("Длительность жёлтого (сек):"), 1, 0);
    m_yellowInput = new QLineEdit("3");
    m_yellowInput->setEnabled(false);
    m_yellowInput->setMaximumWidth(100);
    phasesLayout->addWidget(m_yellowInput, 1, 1);

    phasesLayout->addWidget(new QLabel("Длительность красного (сек):"), 2, 0);
    m_redInput = new QLineEdit("30");
    m_redInput->setEnabled(false);
    m_redInput->setMaximumWidth(100);
    phasesLayout->addWidget(m_redInput, 2, 1);
    paramsLayout->addLayout(phasesLayout);

    // Приоритетные направления
    m_priorityGroupBox = new QGroupBox("Приоритетные направления:");
    m_priorityGroupBox->setStyleSheet(
        "QGroupBox { border: 1px solid #404040; border-radius: 4px; }"
        );

    QVBoxLayout *priorityLayout = new QVBoxLayout(m_priorityGroupBox);
    m_priorityList = new QListWidget;
    m_priorityList->setStyleSheet("background-color: #303030; border: 1px solid #404040;");
    m_priorityList->setMaximumHeight(100);
    m_priorityList->setEnabled(false);
    m_priorityList->addItem("Основное направление (Север-Юг)");
    m_priorityList->addItem("Второстепенное (Запад-Восток)");
    priorityLayout->addWidget(m_priorityList);
    paramsLayout->addWidget(m_priorityGroupBox);

    // Кнопки
    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_applyButton = new QPushButton("Применить");
    m_applyButton->setEnabled(false);
    m_resetButton = new QPushButton("Сбросить");
    m_resetButton->setEnabled(false);
    m_applyToStreetButton = new QPushButton("Применить ко всей улице");  // НОВАЯ
    m_applyToStreetButton->setEnabled(false);
    m_diagnosticsButton = new QPushButton("Диагностика");

    QString btnStyle = "QPushButton { background-color: #0077CC; color: white; border-radius: 4px; "
                       "padding: 8px 16px; font-weight: bold; } "
                       "QPushButton:hover { background-color: #0066BB; } "
                       "QPushButton:disabled { background-color: #555555; color: #888888; }";
    m_applyButton->setStyleSheet(btnStyle);
    m_resetButton->setStyleSheet(btnStyle);
    m_applyToStreetButton->setStyleSheet(btnStyle);
    m_diagnosticsButton->setStyleSheet(btnStyle);

    btnLayout->addWidget(m_applyButton);
    btnLayout->addWidget(m_resetButton);
    btnLayout->addWidget(m_applyToStreetButton);
    btnLayout->addWidget(m_diagnosticsButton);
    paramsLayout->addLayout(btnLayout);

    connect(m_applyButton, &QPushButton::clicked, this, &TrafficLightControlWidget::onApplyClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &TrafficLightControlWidget::onResetClicked);
    connect(m_applyToStreetButton, &QPushButton::clicked, this, &TrafficLightControlWidget::onApplyToStreetClicked);
    connect(m_diagnosticsButton, &QPushButton::clicked, this, &TrafficLightControlWidget::onApplyClicked);

    m_paramsGroupBox->setLayout(paramsLayout);
    mainLayout->addWidget(m_paramsGroupBox, 1);

    mainLayout->addStretch();

    // === ПОДКЛЮЧЕНИЕ К SIMULATIONVIEW ===
    sv = qobject_cast<SimulationView*>(parent);
    if (!sv) {
        sv = parent->findChild<SimulationView*>();
    }

    if (sv) {
        connect(sv, &SimulationView::trafficLightStatusChanged,
                this, &TrafficLightControlWidget::updateCrossingStatus);
        connect(sv, &SimulationView::osmLoadingFinished,
                this, &TrafficLightControlWidget::syncWithSimulation);
        QTimer::singleShot(100, this, &TrafficLightControlWidget::syncWithSimulation);
    }

    onModeChanged(m_modeComboBox->currentText());
}

void TrafficLightControlWidget::syncWithSimulation()
{
    if (!sv) {
        qDebug() << "[SYNC] SimulationView is null!";
        return;
    }

    m_crossingList->clear();
    m_crossingsMap.clear();

    auto list = sv->getTrafficLightsList();

    qDebug() << "[SYNC] Found" << list.size() << "traffic lights";

    if (list.isEmpty()) {
        qDebug() << "[SYNC] No traffic lights found in SimulationView!";
        QListWidgetItem *item = new QListWidgetItem("Светофоры не найдены. Проверьте OSM файл.");
        item->setForeground(QColor("#FFAA00"));
        m_crossingList->addItem(item);
        return;
    }

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

    if (isNight || isAuto) {
        m_controlGroupBox->hide();
        m_paramsGroupBox->hide();
    } else {
        m_controlGroupBox->show();
        if (m_selectedCrossingId >= 0) m_paramsGroupBox->show();
        else m_paramsGroupBox->hide();
    }

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
        m_applyToStreetButton->setEnabled(true);
    }
}

void TrafficLightControlWidget::onApplyClicked()
{
    if (m_selectedCrossingId < 0 || !sv) return;

    int green = m_greenInput->text().toInt() * 1000;
    int yellow = m_yellowInput->text().toInt() * 1000;
    int red = m_redInput->text().toInt() * 1000;

   // sv->setTrafficLightCycle(m_selectedCrossingId, green, yellow, red); //TODO

    QMessageBox::information(this, "Применено",
                             QString("Параметры для %1 обновлены:\nЗелёный: %2с\nЖёлтый: %3с\nКрасный: %4с")
                                 .arg(m_crossingsMap[m_selectedCrossingId].name)
                                 .arg(green/1000).arg(yellow/1000).arg(red/1000));
}

void TrafficLightControlWidget::onResetClicked()
{
    if (m_selectedCrossingId < 0 || !sv) return;

    //sv->resetTrafficLightCycle(m_selectedCrossingId); //TODO

    m_greenInput->setText("30");
    m_yellowInput->setText("3");
    m_redInput->setText("30");

    QMessageBox::information(this, "Сброшено", "Параметры возвращены к значениям по умолчанию.");
}

void TrafficLightControlWidget::onApplyToStreetClicked()
{
    if (m_selectedCrossingId < 0 || !sv) return;

    int green = m_greenInput->text().toInt() * 1000;
    int yellow = m_yellowInput->text().toInt() * 1000;
    int red = m_redInput->text().toInt() * 1000;

   // int count = sv->setTrafficLightCycleForStreet(m_selectedCrossingId, green, yellow, red); //TODO

    // QMessageBox::information(this, "Применено ко всей улице",
    //                          QString("Параметры применены к %1 светофорам на этой улице")
    //                              .arg(count));
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

    if (m_sortByStatusCheckBox->isChecked()) {
        sortCrossingList();
    }
}

void TrafficLightControlWidget::onSortByStatusChanged(int state)
{
    Q_UNUSED(state);
    sortCrossingList();
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
                f.setPointSize(13);
                item->setFont(f);
                item->setBackground(QColor("#402020"));
            } else {
                item->setForeground(QColor("#00AA00"));
                QFont f = item->font();
                f.setBold(false);
                f.setPointSize(12);
                item->setFont(f);
                item->setBackground(QColor());
            }
            item->setText(text);
            break;
        }
    }
}

void TrafficLightControlWidget::loadCrossingParams(long long id)
{
    // Запрос текущих параметров у SimulationView
    if (sv) {
       // auto params = sv->getTrafficLightCycle(id); //TODO
        // m_greenInput->setText(QString::number(params.green / 1000));
        // m_yellowInput->setText(QString::number(params.yellow / 1000));
        // m_redInput->setText(QString::number(params.red / 1000));
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
    if (!m_sortByStatusCheckBox->isChecked()) {
        return;
    }

    QList<QPair<QListWidgetItem*, bool>> itemsWithStatus;
    for (int i = 0; i < m_crossingList->count(); ++i) {
        QListWidgetItem *item = m_crossingList->item(i);
        long long id = item->data(Qt::UserRole).toLongLong();
        bool requiresAttention = m_crossingsMap.value(id).requiresAttention;
        itemsWithStatus.append(qMakePair(item, requiresAttention));
    }

    std::sort(itemsWithStatus.begin(), itemsWithStatus.end(),
              [](const QPair<QListWidgetItem*, bool>& a, const QPair<QListWidgetItem*, bool>& b) {
                  return a.second > b.second;
              });

    m_crossingList->clear();
    for (const auto& pair : itemsWithStatus) {
        m_crossingList->addItem(pair.first);
    }
}
