#include "trafficlightcontrolwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>
#include "simulationmanager.h"

TrafficLightControlWidget::TrafficLightControlWidget(QWidget *parent)
    : QWidget(parent)
{
    sv = SimulationManager::instance().simulationView();
    setStyleSheet("background-color: #2B2B2B; color: #FFFFFF;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // === 1. Статус (Жестко фиксирован) ===
    statusLabel = new QLabel("Ручной режим");
    statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusLabel->setMinimumHeight(40);
    // Запрещаем сжиматься меньше содержимого, но разрешаем расти по ширине
    statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statusLabel->setStyleSheet(
        "font-weight: bold; "
        "color: #FFAA00; " // Начальный цвет под ручной режим
        "background-color: #202020; "
        "border-radius: 4px; "
        "padding: 8px; "
        "font-size: 14px;"
        );
    mainLayout->addWidget(statusLabel);

    // === 2. Выбор режима (Жестко фиксирован) ===
    QHBoxLayout *modeLayout = new QHBoxLayout;
    modeLayout->addWidget(new QLabel("Режим работы:"));

    m_modeComboBox = new QComboBox;
    m_modeComboBox->addItems({"Ручной режим", "Автоматический (рекомендуется)", "Ночной режим"});
    m_modeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // Ограничим максимальную ширину комбобокса, чтобы он не растягивался на весь экран
    m_modeComboBox->setMaximumWidth(400);

    modeLayout->addWidget(m_modeComboBox);
    modeLayout->addStretch(); // Распорка справа, чтобы прижать влево

    mainLayout->addLayout(modeLayout);

    // === 3. Управление объектами ===
    m_controlGroupBox = new QGroupBox("Управление светофорными объектами");
    m_controlGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    // ФИКСАЦИЯ РАЗМЕРОВ ГРУППЫ
    m_controlGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *controlLayout = new QVBoxLayout(m_controlGroupBox);
    QGridLayout *crossingsLayout = new QGridLayout;
    crossingsLayout->setContentsMargins(10, 10, 10, 10);
    crossingsLayout->setVerticalSpacing(8);

    // Лейблы (без изменений)
    QLabel *crossing1 = new QLabel("Перекрёсток #1 Невский пр. / Литейный пр.");
    crossing1->setStyleSheet("color: #00AA00; font-weight: bold;");
    QLabel *status1 = new QLabel("Статус: Норма"); status1->setStyleSheet("color: #00AA00; font-size: 12px;");
    QLabel *crossing2 = new QLabel("Перекрёсток #2 Московский пр. / Кузнечный пер.");
    crossing2->setStyleSheet("color: #00AA00; font-weight: bold;");
    QLabel *status2 = new QLabel("Статус: Норма"); status2->setStyleSheet("color: #00AA00; font-size: 12px;");
    QLabel *crossing3 = new QLabel("Перекрёсток #3 Лиговский пр. / Разъезжая ул.");
    crossing3->setStyleSheet("color: #00AA00; font-weight: bold;");
    QLabel *status3 = new QLabel("Статус: Норма"); status3->setStyleSheet("color: #00AA00; font-size: 12px;");
    QLabel *crossing4 = new QLabel("Перекрёсток #4 Сампсониевский пр. / Б. Сампс. пр.");
    crossing4->setStyleSheet("color: #CC0000; font-weight: bold;");
    QLabel *status4 = new QLabel("Статус: Требует внимания"); status4->setStyleSheet("color: #CC0000; font-size: 12px;");

    crossingsLayout->addWidget(crossing1, 0, 0); crossingsLayout->addWidget(status1, 0, 1);
    crossingsLayout->addWidget(crossing2, 1, 0); crossingsLayout->addWidget(status2, 1, 1);
    crossingsLayout->addWidget(crossing3, 2, 0); crossingsLayout->addWidget(status3, 2, 1);
    crossingsLayout->addWidget(crossing4, 3, 0); crossingsLayout->addWidget(status4, 3, 1);

    controlLayout->addLayout(crossingsLayout);
    m_controlGroupBox->setLayout(controlLayout);
    mainLayout->addWidget(m_controlGroupBox);

    // === 4. Список объектов ===
    m_crossingListGroupBox = new QGroupBox("Список объектов");
    m_crossingListGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    m_crossingListGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *listLayout = new QVBoxLayout(m_crossingListGroupBox);
    m_crossingList = new QListWidget;
    m_crossingList->setStyleSheet("background-color: #303030; border: 1px solid #404040;");
    m_crossingList->addItem("1 Невский / Литейный");
    m_crossingList->addItem("2 Московский / Кузнечный");
    m_crossingList->addItem("3 Лиговский / Разъезжая");
    m_crossingList->addItem("4 Сампсониевский / Б. Сампс.");
    m_crossingList->addItem("5 Камская / Большой пр.");
    m_crossingList->setMaximumHeight(150); // Фиксируем высоту списка

    listLayout->addWidget(m_crossingList);
    m_crossingListGroupBox->setLayout(listLayout);
    mainLayout->addWidget(m_crossingListGroupBox);

    // === 5. Параметры светофора ===
    m_paramsGroupBox = new QGroupBox("Параметры светофора: Невский пр. / Литейный пр.");
    m_paramsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    m_paramsGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *paramsLayout = new QVBoxLayout(m_paramsGroupBox);

    connect(m_modeComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &TrafficLightControlWidget::onModeChanged);

    // Длительность фаз
    QGridLayout *phasesLayout = new QGridLayout;
    phasesLayout->addWidget(new QLabel("Длительность зелёного (сек):"), 0, 0);
    m_greenInput = new QLineEdit("45");
    phasesLayout->addWidget(m_greenInput, 0, 1);
    phasesLayout->addWidget(new QLabel("Длительность жёлтого (сек):"), 1, 0);
    m_yellowInput = new QLineEdit("3");
    phasesLayout->addWidget(m_yellowInput, 1, 1);
    phasesLayout->addWidget(new QLabel("Длительность красного (сек):"), 2, 0);
    m_redInput = new QLineEdit("30");
    phasesLayout->addWidget(m_redInput, 2, 1);
    paramsLayout->addLayout(phasesLayout);

    // Приоритетные направления
    m_priorityGroupBox = new QGroupBox("Приоритетные направления:");
    m_priorityGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; }");
    m_priorityGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *priorityLayout = new QVBoxLayout(m_priorityGroupBox);
    priorityLayout->addWidget(new QLabel("Невский пр. (север)"));
    priorityLayout->addWidget(new QLabel("Невский пр. (юг)"));
    priorityLayout->addWidget(new QLabel("Литейный пр. (запад)"));
    priorityLayout->addWidget(new QLabel("Литейный пр. (восток)"));
    m_priorityList = new QListWidget;
    m_priorityList->addItems({"Невский пр. (север)", "Невский пр. (юг)", "Литейный пр. (запад)", "Литейный пр. (восток)"});
    m_priorityList->setStyleSheet("background-color: #303030; border: 1px solid #404040;");
    m_priorityList->setMaximumHeight(100);
    priorityLayout->addWidget(m_priorityList);
    paramsLayout->addWidget(m_priorityGroupBox);

    // Кнопки
    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_applyButton = new QPushButton("Применить изменения");
    m_applyButton->setStyleSheet("QPushButton { background-color: #0077CC; color: white; border-radius: 4px; padding: 8px 16px; font-weight: bold; } QPushButton:hover { background-color: #0066BB; }");
    m_resetButton = new QPushButton("Сбросить");
    m_resetButton->setStyleSheet("QPushButton { background-color: #0077CC; color: white; border-radius: 4px; padding: 8px 16px; font-weight: bold; } QPushButton:hover { background-color: #0066BB; }");
    m_diagnosticsButton = new QPushButton("Диагностика");
    m_diagnosticsButton->setStyleSheet("QPushButton { background-color: #0077CC; color: white; border-radius: 4px; padding: 8px 16px; font-weight: bold; } QPushButton:hover { background-color: #0066BB; }");

    btnLayout->addWidget(m_applyButton);
    btnLayout->addWidget(m_resetButton);
    btnLayout->addWidget(m_diagnosticsButton);
    paramsLayout->addLayout(btnLayout);

    m_paramsGroupBox->setLayout(paramsLayout);
    mainLayout->addWidget(m_paramsGroupBox);

    // === ВАЖНО: Распорка вниз, чтобы всё прижалось к верху и не прыгало ===
    mainLayout->addStretch();

    // Инициализация
    onModeChanged(m_modeComboBox->currentText());
}

void TrafficLightControlWidget::onModeChanged(const QString &modeText)
{
    QString newText;
    QString colorCode;
    QString bgColor = "#202020";

    bool isNight = modeText.contains("Ночной");
    bool isAuto = modeText.contains("Автоматический");
    bool isManual = modeText.contains("Ручной");

    // === ЛОГИКА ВИДИМОСТИ (Без лишних проверок на null) ===
    if (isNight || isAuto) {
        // СКРЫВАЕМ всё лишнее
        m_controlGroupBox->hide();
        m_crossingListGroupBox->hide();
        m_paramsGroupBox->hide();
        // m_priorityGroupBox скроется сам, т.к. внутри m_paramsGroupBox, но можно и явно
    }
    else if (isManual) {
        // ПОКАЗЫВАЕМ всё обратно
        m_controlGroupBox->show();
        m_crossingListGroupBox->show();
        m_paramsGroupBox->show();
        // m_priorityGroupBox->show(); // Не обязательно, если родитель видим
    }

    // === ЛОГИКА ТЕКСТА И ЦВЕТА ===
    if (isAuto) {
        newText = "Автоматический режим";
        colorCode = "#00AA00";
        if(sv) sv->setLightAutoMode();
    }
    else if (isManual) {
        newText = "Ручное управление";
        colorCode = "#FFAA00";
        if(sv) sv->setLightManualOperation();
    }
    else if (isNight) {
        newText = "Ночной режим";
        colorCode = "#00AAFF";
        if(sv) sv->setLightNightMode();
    }
    else {
        newText = modeText;
        colorCode = "#FFFFFF";
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

    // Принудительно обновляем layout, чтобы убрать артефакты
    this->layout()->update();
}
