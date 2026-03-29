#include "trafficlightcontrolwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QMessageBox>
#include <QCheckBox>
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

    // Направления
    m_priorityGroupBox = new QGroupBox("Направления:");
    m_priorityGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");

    QVBoxLayout *priorityLayout = new QVBoxLayout(m_priorityGroupBox);
    
    m_priorityList = new QListWidget;
    m_priorityList->setStyleSheet("background-color: #303030; border: 1px solid #404040;");
    m_priorityList->setMaximumHeight(150);
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
    if (!sv) {
        QMessageBox::warning(this, "Ошибка", "SimulationView не найден");
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

    // Проверяем, есть ли выбранные направления в списке
    QList<long long> selectedWayIds;
    for (int i = 0; i < m_priorityList->count(); ++i) {
        QListWidgetItem *wayItem = m_priorityList->item(i);
        if (wayItem->checkState() == Qt::Checked) {
            selectedWayIds.append(wayItem->data(Qt::UserRole).toLongLong());
        }
    }

    // Если ни одно направление не выбрано, применяем ко всем светофорам на участке
    bool applyToAll = selectedWayIds.isEmpty();

    if (applyToAll) {
        // Применяем ко всем светофорам на участке
        for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
            long long tlId = it.key();
            sv->setTrafficLightCycle(tlId, green * 1000, yellow * 1000, red * 1000);
        }

        QMessageBox::information(this, "Применено",
                                 QString("Параметры обновлены для всех светофоров (%1 шт.):\n"
                                         "🟢 Зелёный: %2 сек\n"
                                         "🟡 Жёлтый: %3 сек\n"
                                         "🔴 Красный: %4 сек")
                                     .arg(m_crossingsMap.size())
                                     .arg(green).arg(yellow).arg(red));

        qDebug() << "[APPLY ALL] Applied to" << m_crossingsMap.size() << "traffic lights"
                 << "G:" << green << "s Y:" << yellow << "s R:" << red;
    } else {
        // Применяем только к выбранным направлениям (светофорам на этих дорогах)
        for (long long wayId : selectedWayIds) {
            // Находим все светофоры, связанные с этим направлением
            for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
                long long tlId = it.key();
                CrossingInfo info = it.value();
                // Проверяем, относится ли светофор к этому направлению
                if (info.name.contains(QString::number(wayId)) || wayId == tlId) {
                    sv->setTrafficLightCycle(tlId, green * 1000, yellow * 1000, red * 1000);
                }
            }
        }

        QMessageBox::information(this, "Применено",
                                 QString("Параметры обновлены для выбранных направлений (%1 шт.):\n"
                                         "🟢 Зелёный: %2 сек\n"
                                         "🟡 Жёлтый: %3 сек\n"
                                         "🔴 Красный: %4 сек")
                                     .arg(selectedWayIds.size())
                                     .arg(green).arg(yellow).arg(red));

        qDebug() << "[APPLY SELECTED] Applied to" << selectedWayIds.size() << "ways"
                 << "G:" << green << "s Y:" << yellow << "s R:" << red;
    }
}

void TrafficLightControlWidget::onResetClicked()
{
    if (!sv) {
        QMessageBox::warning(this, "Ошибка", "SimulationView не найден");
        return;
    }

    // Проверяем, есть ли выбранные направления в списке
    QList<long long> selectedWayIds;
    for (int i = 0; i < m_priorityList->count(); ++i) {
        QListWidgetItem *wayItem = m_priorityList->item(i);
        if (wayItem->checkState() == Qt::Checked) {
            selectedWayIds.append(wayItem->data(Qt::UserRole).toLongLong());
        }
    }

    // Если ни одно направление не выбрано, сбрасываем все светофоры на участке
    bool resetAll = selectedWayIds.isEmpty();

    if (resetAll) {
        // Сбрасываем все светофоры на участке
        for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
            long long tlId = it.key();
            sv->resetTrafficLightCycle(tlId);
        }

        m_greenInput->setText("30");
        m_yellowInput->setText("3");
        m_redInput->setText("30");

        QMessageBox::information(this, "Сброшено",
                                 QString("Параметры всех светофоров (%1 шт.) возвращены к значениям по умолчанию:\n"
                                         "🟢 Зелёный: 30 сек\n"
                                         "🟡 Жёлтый: 3 сек\n"
                                         "🔴 Красный: 30 сек")
                                     .arg(m_crossingsMap.size()));

        qDebug() << "[RESET ALL] Reset" << m_crossingsMap.size() << "traffic lights to default";
    } else {
        // Сбрасываем только выбранные направления
        for (long long wayId : selectedWayIds) {
            for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
                long long tlId = it.key();
                CrossingInfo info = it.value();
                if (info.name.contains(QString::number(wayId)) || wayId == tlId) {
                    sv->resetTrafficLightCycle(tlId);
                }
            }
        }

        m_greenInput->setText("30");
        m_yellowInput->setText("3");
        m_redInput->setText("30");

        QMessageBox::information(this, "Сброшено",
                                 QString("Параметры для выбранных направлений (%1 шт.) возвращены к значениям по умолчанию:\n"
                                         "🟢 Зелёный: 30 сек\n"
                                         "🟡 Жёлтый: 3 сек\n"
                                         "🔴 Красный: 30 сек")
                                     .arg(selectedWayIds.size()));

        qDebug() << "[RESET SELECTED] Reset" << selectedWayIds.size() << "ways to default";
    }
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

    // Заполняем список направлений из данных SimulationView
    m_priorityList->clear();
    
    // Получаем все направления (дороги) из SimulationView
    if (sv) {
        QList<PendingWay> allWays = sv->getAllWays();
        
        int colorIndex = 0;
        QColor colors[] = {QColor("#4CAF50"), QColor("#2196F3"), QColor("#FF9800"), QColor("#E91E63")};
        
        // Добавляем каждое направление как элемент списка с чекбоксом
        for (const PendingWay &way : allWays) {
            // Формируем красивое имя дороги
            QString directionName;
            if (!way.name.isEmpty()) {
                directionName = way.name;  // Используем название улицы (например, "Невский проспект")
            } else {
                directionName = QString("Дорога: %1").arg(way.highwayType);
            }
            
            if (way.isOneWay) {
                directionName += " (односторонняя)";
            }
            
            QListWidgetItem *item = new QListWidgetItem(directionName);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            // Сохраняем ID первого узла дороги как идентификатор направления
            if (!way.nodeRefs.isEmpty()) {
                item->setData(Qt::UserRole, way.nodeRefs.first());
            }
            item->setBackground(colors[colorIndex % 4]);
            item->setForeground(QColor("white"));
            m_priorityList->addItem(item);
            colorIndex++;
        }
        
        // Если дорог нет, добавляем направления по умолчанию
        if (allWays.isEmpty()) {
            QStringList directions;
            directions << "Север-Юг" << "Запад-Восток";
            
            for (const QString &dir : directions) {
                QListWidgetItem *item = new QListWidgetItem(dir);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Unchecked);
                item->setData(Qt::UserRole, id);
                item->setBackground(colors[colorIndex % 4]);
                item->setForeground(QColor("white"));
                m_priorityList->addItem(item);
                colorIndex++;
            }
        }
    }
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
