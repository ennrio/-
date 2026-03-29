#include "trafficlightcontrolwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QMessageBox>
#include <QCheckBox>
#include <QSet>
#include <QMouseEvent>
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
    
    // Устанавливаем фильтр событий для списка перекрёстков, чтобы отлавливать клики вне элементов
    m_crossingList->viewport()->installEventFilter(this);

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
    connect(m_priorityList, &QListWidget::itemClicked, this, &TrafficLightControlWidget::onWaySelected);
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

    if (m_crossingList->count() > 0) {
        QListWidgetItem *firstItem = m_crossingList->item(0);
        m_crossingList->setCurrentItem(firstItem);
        onCrossingSelected();
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
    
    // Сбрасываем выделение в списке направлений при выборе нового светофора
    for (int i = 0; i < m_priorityList->count(); ++i) {
        QListWidgetItem *wayItem = m_priorityList->item(i);
        wayItem->setCheckState(Qt::Unchecked);
    }
}

void TrafficLightControlWidget::resetSelection()
{
    // Сбрасываем выделение в списке перекрёстков
    m_crossingList->clearSelection();
    m_crossingList->setCurrentItem(nullptr);
    
    // Сбрасываем выбранный ID
    m_selectedCrossingId = -1;
    
    // Очищаем заголовок и параметры
    m_paramsGroupBox->setTitle("Параметры светофора");
    m_greenInput->setText("30");
    m_yellowInput->setText("3");
    m_redInput->setText("30");
    m_greenInput->setEnabled(false);
    m_yellowInput->setEnabled(false);
    m_redInput->setEnabled(false);
    m_priorityList->clear();
    m_priorityList->setEnabled(false);
    m_applyButton->setEnabled(false);
    m_resetButton->setEnabled(false);
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

    // 1. Читаем и валидируем значения
    int green = m_greenInput->text().toInt();
    int yellow = m_yellowInput->text().toInt();
    int red = m_redInput->text().toInt();

    QString error;
    if (!validatePhaseTimes(green, yellow, red, error)) {
        QMessageBox::warning(this, "Ошибка", error);
        return;
    }

    // 2. Определяем целевые светофоры (унифицированная логика)
    QList<long long> targetTls = getTargetTrafficLights();
    if (targetTls.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не выбран объект для применения настроек");
        return;
    }

    // Если выбрано "Все дороги участка" или ни одно направление не выбрано
    bool applyToAll = allRoadsSelected || selectedWayIds.isEmpty();

    if (applyToAll) {
        // Применяем ко всем светофорам на участке (аналогично ночному режиму)
        QSet<long long> affectedTrafficLights;
        
        // Получаем все дороги и собираем все узлы дорог участка
        QList<PendingWay> allWays = sv->getAllWays();
        QSet<long long> allSectionNodes;
        
        for (const PendingWay &way : allWays) {
            for (long long nodeId : way.nodeRefs) {
                allSectionNodes.insert(nodeId);
            }
        }
        
        // Находим все светофоры, которые принадлежат этим узлам
        for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
            long long tlId = it.key();
            long long osmNodeId = sv->getTrafficLightOsmNodeId(tlId);
            
            if (allSectionNodes.contains(osmNodeId)) {
                affectedTrafficLights.insert(tlId);
            }
        }
        
        // Применяем настройки ко всем найденным светофорам
        for (long long tlId : affectedTrafficLights) {
            sv->setTrafficLightCycle(tlId, green * 1000, yellow * 1000, red * 1000);
        }

        QMessageBox::information(this, "Применено",
                                 QString("Параметры обновлены для всех светофоров участка (%1 шт.):\n"
                                         "🟢 Зелёный: %2 сек\n"
                                         "🟡 Жёлтый: %3 сек\n"
                                         "🔴 Красный: %4 сек")
                                     .arg(affectedTrafficLights.size())
                                     .arg(green).arg(yellow).arg(red));

        qDebug() << "[APPLY ALL] Applied to" << affectedTrafficLights.size() << "traffic lights"
                 << "G:" << green << "s Y:" << yellow << "s R:" << red;
    } else {
        // Применяем к выбранным направлениям - находим все светофоры на этих дорогах
        // и на дорогах, которые пересекаются с ними
        QSet<long long> affectedTrafficLights;
        
        for (long long selectedWayId : selectedWayIds) {
            // Получаем все дороги
            QList<PendingWay> allWays = sv->getAllWays();
            
            // Находим выбранную дорогу по ID первого узла или по названию
            PendingWay selectedWay;
            bool found = false;
            for (const PendingWay &way : allWays) {
                if (!way.nodeRefs.isEmpty() && way.nodeRefs.first() == selectedWayId) {
                    selectedWay = way;
                    found = true;
                    break;
                }
            }
            
            if (!found) continue;
            
            // Находим все светофоры на этой дороге
            for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
                long long tlId = it.key();
                long long osmNodeId = sv->getTrafficLightOsmNodeId(tlId);
                
                // Проверяем, принадлежит ли светофор к выбранной дороге
                if (selectedWay.nodeRefs.contains(osmNodeId)) {
                    affectedTrafficLights.insert(tlId);
                }
            }
            
            // ТЕПЕРЬ: Находим все дороги, которые пересекаются с выбранной
            // (имеют общие узлы со светофорами)
            for (const PendingWay &otherWay : allWays) {
                if (otherWay.nodeRefs == selectedWay.nodeRefs) continue; // пропускаем саму дорогу
                
                // Проверяем пересечение по узлам
                bool hasIntersection = false;
                for (long long nodeId : otherWay.nodeRefs) {
                    if (selectedWay.nodeRefs.contains(nodeId)) {
                        hasIntersection = true;
                        break;
                    }
                }
                
                if (hasIntersection) {
                    // Добавляем все светофоры на пересекающейся дороге
                    for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
                        long long tlId = it.key();
                        long long osmNodeId = sv->getTrafficLightOsmNodeId(tlId);
                        
                        if (otherWay.nodeRefs.contains(osmNodeId)) {
                            affectedTrafficLights.insert(tlId);
                        }
                    }
                }
            }
        }
        
        // Применяем настройки ко всем найденным светофорам
        for (long long tlId : affectedTrafficLights) {
            sv->setTrafficLightCycle(tlId, green * 1000, yellow * 1000, red * 1000);
        }

        QMessageBox::information(this, "Применено",
                                 QString("Параметры обновлены для выбранных направлений (%1 шт.):\n"
                                         "🟢 Зелёный: %2 сек\n"
                                         "🟡 Жёлтый: %3 сек\n"
                                         "🔴 Красный: %4 сек")
                                     .arg(affectedTrafficLights.size())
                                     .arg(green).arg(yellow).arg(red));

        qDebug() << "[APPLY SELECTED] Applied to" << affectedTrafficLights.size() << "traffic lights"
                 << "G:" << green << "s Y:" << yellow << "s R:" << red;
    }
    // 3. Применяем
    applySettingsToTrafficLights(targetTls, green, yellow, red);

    // 4. Показываем результат
    QMessageBox::information(this, "Применено", formatApplyMessage(targetTls, green, yellow, red));

    qDebug() << "[APPLY] Applied to" << targetTls.size() << "traffic light(s)"
             << "G:" << green << "s Y:" << yellow << "s R:" << red;
}

void TrafficLightControlWidget::onResetClicked()
{
    if (!sv) {
        QMessageBox::warning(this, "Ошибка", "SimulationView не найден");
        return;
    }

    // 1. Определяем целевые светофоры
    QList<long long> targetTls = getTargetTrafficLights();
    if (targetTls.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не выбран объект для сброса настроек");
        return;
    }

    // 2. Сбрасываем
    resetSettingsForTrafficLights(targetTls);

    if (resetAll) {
        // Сбрасываем все светофоры на участке (аналогично ночному режиму)
        QSet<long long> affectedTrafficLights;
        
        // Получаем все дороги и собираем все узлы дорог участка
        QList<PendingWay> allWays = sv->getAllWays();
        QSet<long long> allSectionNodes;
        
        for (const PendingWay &way : allWays) {
            for (long long nodeId : way.nodeRefs) {
                allSectionNodes.insert(nodeId);
            }
        }
        
        // Находим все светофоры, которые принадлежат этим узлам
        for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
            long long tlId = it.key();
            long long osmNodeId = sv->getTrafficLightOsmNodeId(tlId);
            
            if (allSectionNodes.contains(osmNodeId)) {
                affectedTrafficLights.insert(tlId);
            }
        }
        
        // Сбрасываем настройки для всех найденных светофоров
        for (long long tlId : affectedTrafficLights) {
            sv->resetTrafficLightCycle(tlId);
        }
    // 3. Возвращаем поля ввода к дефолту
    m_greenInput->setText("30");
    m_yellowInput->setText("3");
    m_redInput->setText("30");

    // 4. Показываем результат
    QMessageBox::information(this, "Сброшено", formatResetMessage(targetTls));

        QMessageBox::information(this, "Сброшено",
                                 QString("Параметры всех светофоров участка (%1 шт.) возвращены к значениям по умолчанию:\n"
                                         "🟢 Зелёный: 30 сек\n"
                                         "🟡 Жёлтый: 3 сек\n"
                                         "🔴 Красный: 30 сек")
                                     .arg(affectedTrafficLights.size()));

        qDebug() << "[RESET ALL] Reset" << affectedTrafficLights.size() << "traffic lights to default";
    } else {
        // Сбрасываем только выбранные направления - включая дороги с пересечениями
        QSet<long long> affectedTrafficLights;
        
        for (long long selectedWayId : selectedWayIds) {
            // Получаем все дороги
            QList<PendingWay> allWays = sv->getAllWays();
            
            // Находим выбранную дорогу по ID первого узла
            PendingWay selectedWay;
            bool found = false;
            for (const PendingWay &way : allWays) {
                if (!way.nodeRefs.isEmpty() && way.nodeRefs.first() == selectedWayId) {
                    selectedWay = way;
                    found = true;
                    break;
                }
            }
            
            if (!found) continue;
            
            // Находим все светофоры на этой дороге
            for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
                long long tlId = it.key();
                long long osmNodeId = sv->getTrafficLightOsmNodeId(tlId);
                
                if (selectedWay.nodeRefs.contains(osmNodeId)) {
                    affectedTrafficLights.insert(tlId);
                }
            }
            
            // Находим дороги, которые пересекаются с выбранной
            for (const PendingWay &otherWay : allWays) {
                if (otherWay.nodeRefs == selectedWay.nodeRefs) continue;
                
                bool hasIntersection = false;
                for (long long nodeId : otherWay.nodeRefs) {
                    if (selectedWay.nodeRefs.contains(nodeId)) {
                        hasIntersection = true;
                        break;
                    }
                }
                
                if (hasIntersection) {
                    for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
                        long long tlId = it.key();
                        long long osmNodeId = sv->getTrafficLightOsmNodeId(tlId);
                        
                        if (otherWay.nodeRefs.contains(osmNodeId)) {
                            affectedTrafficLights.insert(tlId);
                        }
                    }
                }
            }
        }
        
        // Сбрасываем настройки для всех найденных светофоров
        for (long long tlId : affectedTrafficLights) {
            sv->resetTrafficLightCycle(tlId);
        }
    qDebug() << "[RESET] Reset" << targetTls.size() << "traffic light(s) to default";
}

void TrafficLightControlWidget::onWaySelected()
{
    // Сбрасываем выбранный светофор
    m_selectedCrossingId = -1;

    // Снимаем выделение со списка светофоров
    m_crossingList->clearSelection();
    m_crossingList->setCurrentItem(nullptr);

        QMessageBox::information(this, "Сброшено",
                                 QString("Параметры для выбранных направлений (%1 шт.) возвращены к значениям по умолчанию:\\n"
                                         "🟢 Зелёный: 30 сек\\n"
                                         "🟡 Жёлтый: 3 сек\\n"
                                         "🔴 Красный: 30 сек")
                                     .arg(affectedTrafficLights.size()));

        qDebug() << "[RESET SELECTED] Reset" << affectedTrafficLights.size() << "traffic lights to default";
    }
    // Возвращаем заголовок к исходному состоянию
    m_paramsGroupBox->setTitle("Параметры светофора");

    // Очищаем поля ввода
    m_greenInput->setText("30");
    m_yellowInput->setText("3");
    m_redInput->setText("30");

    // Список направлений остаётся доступным для выбора
    m_priorityList->setEnabled(true);

    qDebug() << "[WAY SELECTED] Traffic light selection cleared, way list remains active";
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
        
        // Добавляем элемент "Все дороги участка" с оранжевым фоном
        QListWidgetItem *allRoadsItem = new QListWidgetItem("🛣 Все дороги участка");
        allRoadsItem->setFlags(allRoadsItem->flags() | Qt::ItemIsUserCheckable);
        allRoadsItem->setCheckState(Qt::Unchecked);
        allRoadsItem->setData(Qt::UserRole, -1);  // Специальный ID для всех дорог
        allRoadsItem->setBackground(QColor("#FF9800"));  // Оранжевый фон
        allRoadsItem->setForeground(QColor("white"));
        allRoadsItem->setFont(QFont("", -1, QFont::Bold));
        m_priorityList->addItem(allRoadsItem);
        
        // Используем QSet для отслеживания уникальных названий
        QSet<QString> uniqueNames;
        
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
            
            // Проверяем на дубликаты
            if (uniqueNames.contains(directionName)) {
                continue;  // Пропускаем дубликат
            }
            uniqueNames.insert(directionName);
            
            QListWidgetItem *item = new QListWidgetItem(directionName);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            // Сохраняем ID первого узла дороги как идентификатор направления
            if (!way.nodeRefs.isEmpty()) {
                item->setData(Qt::UserRole, way.nodeRefs.first());
            }
            // Убираем цветовое выделение - используем стандартный фон
            item->setForeground(QColor("white"));
            m_priorityList->addItem(item);
        }
        
        // Если дорог нет, добавляем направления по умолчанию
        if (allWays.isEmpty()) {
            QStringList directions;
            directions << "Север-Юг" << "Запад-Восток";
            
            for (const QString &dir : directions) {
                // Проверяем на дубликаты
                if (uniqueNames.contains(dir)) {
                    continue;
                }
                uniqueNames.insert(dir);
                
                QListWidgetItem *item = new QListWidgetItem(dir);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Unchecked);
                item->setData(Qt::UserRole, id);
                item->setForeground(QColor("white"));
                m_priorityList->addItem(item);
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

bool TrafficLightControlWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_crossingList->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QModelIndex index = m_crossingList->indexAt(mouseEvent->pos());
        
        // Если клик вне элемента списка - сбрасываем выделение
        if (!index.isValid()) {
            resetSelection();
            return true;
        }
    }
    
    // Также обрабатываем клики в списке направлений для сброса выделения светофора
    if (obj == m_priorityList->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QModelIndex index = m_priorityList->indexAt(mouseEvent->pos());
        
        // Если клик по элементу списка направлений - сбрасываем выбранный ID светофора
        // чтобы применение настроек шло только по выбранным дорогам
        if (index.isValid()) {
            m_selectedCrossingId = -1;
            m_crossingList->clearSelection();
            m_crossingList->setCurrentItem(nullptr);
            
            // Сбрасываем чекбоксы всех направлений при выборе нового направления
            for (int i = 0; i < m_priorityList->count(); ++i) {
                QListWidgetItem *wayItem = m_priorityList->item(i);
                wayItem->setCheckState(Qt::Unchecked);
            }
        }
    }
    
    return QWidget::eventFilter(obj, event);
bool TrafficLightControlWidget::validatePhaseTimes(int green, int yellow, int red, QString &error)
{
    if (green <= 5 || green >= 120) {
        error = "Зелёный сигнал должен быть от 5 до 120 секунд";
        return false;
    }
    if (yellow <= 2 || yellow >= 10) {
        error = "Жёлтый сигнал должен быть от 2 до 10 секунд";
        return false;
    }
    if (red <= 5 || red >= 120) {
        error = "Красный сигнал должен быть от 5 до 120 секунд";
        return false;
    }
    return true;
}

QList<long long> TrafficLightControlWidget::getTargetTrafficLights() const
{
    QList<long long> result;

    // Приоритет 1: явно выбранный светофор в списке перекрёстков
    if (m_selectedCrossingId != -1 && m_crossingsMap.contains(m_selectedCrossingId)) {
        result.append(m_selectedCrossingId);
        return result;
    }

    // Приоритет 2: выбранные направления в m_priorityList
    QList<long long> selectedWayIds;
    bool allRoadsSelected = false;

    for (int i = 0; i < m_priorityList->count(); ++i) {
        QListWidgetItem *wayItem = m_priorityList->item(i);
        if (wayItem->checkState() == Qt::Checked) {
            long long wayId = wayItem->data(Qt::UserRole).toLongLong();
            if (wayId == -1) {
                allRoadsSelected = true;  // "Все дороги участка"
            } else {
                selectedWayIds.append(wayId);
            }
        }
    }

    // Если выбрано "Все дороги" или ничего не выбрано — применяем ко всем
    if (allRoadsSelected) {
        for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
            result.append(it.key());
        }
    } else {
        // Применяем только к светофорам на выбранных дорогах
        for (long long wayId : selectedWayIds) {
            for (auto it = m_crossingsMap.begin(); it != m_crossingsMap.end(); ++it) {
                long long tlId = it.key();
                const CrossingInfo &info = it.value();
                // Сопоставление: по ID или по вхождению номера дороги в имя
                if (info.name.contains(QString::number(wayId)) || wayId == tlId) {
                    if (!result.contains(tlId)) {
                        result.append(tlId);
                    }
                }
            }
        }
    }

    return result;
}

void TrafficLightControlWidget::applySettingsToTrafficLights(const QList<long long> &tlIds, int green, int yellow, int red)
{
    if (!sv) return;
    for (long long tlId : tlIds) {
        sv->setTrafficLightCycle(tlId, green * 1000, yellow * 1000, red * 1000);
    }
}

void TrafficLightControlWidget::resetSettingsForTrafficLights(const QList<long long> &tlIds)
{
    if (!sv) return;
    for (long long tlId : tlIds) {
        sv->resetTrafficLightCycle(tlId);
    }
}

QString TrafficLightControlWidget::formatApplyMessage(const QList<long long> &tlIds, int green, int yellow, int red) const
{
    if (tlIds.size() == 1 && m_crossingsMap.contains(tlIds.first())) {
        // Единичный светофор
        return QString("Параметры для %1 обновлены:\n"
                       "🟢 Зелёный: %2 сек\n"
                       "🟡 Жёлтый: %3 сек\n"
                       "🔴 Красный: %4 сек")
            .arg(m_crossingsMap[tlIds.first()].name)
            .arg(green).arg(yellow).arg(red);
    } else {
        // Множественное применение
        return QString("Параметры обновлены для %1 светофоров:\n"
                       "🟢 Зелёный: %2 сек\n"
                       "🟡 Жёлтый: %3 сек\n"
                       "🔴 Красный: %4 сек")
            .arg(tlIds.size())
            .arg(green).arg(yellow).arg(red);
    }
}

QString TrafficLightControlWidget::formatResetMessage(const QList<long long> &tlIds) const
{
    if (tlIds.size() == 1 && m_crossingsMap.contains(tlIds.first())) {
        return QString("Параметры для %1 возвращены к значениям по умолчанию:\n"
                       "🟢 Зелёный: 30 сек | 🟡 Жёлтый: 3 сек | 🔴 Красный: 30 сек")
            .arg(m_crossingsMap[tlIds.first()].name);
    } else {
        return QString("Параметры для %1 светофоров возвращены к значениям по умолчанию:\n"
                       "🟢 Зелёный: 30 сек | 🟡 Жёлтый: 3 сек | 🔴 Красный: 30 сек")
            .arg(tlIds.size());
    }
}
