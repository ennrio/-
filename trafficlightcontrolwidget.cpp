#include "trafficlightcontrolwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>

TrafficLightControlWidget::TrafficLightControlWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #2B2B2B; color: #FFFFFF;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // === Верхняя панель статуса ===
    QLabel *statusLabel = new QLabel("Автоматический режим");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "font-weight: bold; "
        "color: #00AA00; "
        "background-color: #202020; "
        "border-radius: 4px; "
        "padding: 8px; "
        "font-size: 14px;"
        );
    mainLayout->addWidget(statusLabel);

    // === Управление светофорными объектами ===
    QGroupBox *controlGroupBox = new QGroupBox("Управление светофорными объектами");
    controlGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroupBox);

    // Список перекрёстков
    QGridLayout *crossingsLayout = new QGridLayout;
    crossingsLayout->setContentsMargins(10, 10, 10, 10);
    crossingsLayout->setVerticalSpacing(8);

    QLabel *crossing1 = new QLabel("Перекрёсток #1 Невский пр. / Литейный пр.");
    crossing1->setStyleSheet("color: #00AA00; font-weight: bold;");
    QLabel *status1 = new QLabel("Статус: Норма");
    status1->setStyleSheet("color: #00AA00; font-size: 12px;");

    QLabel *crossing2 = new QLabel("Перекрёсток #2 Московский пр. / Кузнечный пер.");
    crossing2->setStyleSheet("color: #00AA00; font-weight: bold;");
    QLabel *status2 = new QLabel("Статус: Норма");
    status2->setStyleSheet("color: #00AA00; font-size: 12px;");

    QLabel *crossing3 = new QLabel("Перекрёсток #3 Лиговский пр. / Разъезжая ул.");
    crossing3->setStyleSheet("color: #00AA00; font-weight: bold;");
    QLabel *status3 = new QLabel("Статус: Норма");
    status3->setStyleSheet("color: #00AA00; font-size: 12px;");

    QLabel *crossing4 = new QLabel("Перекрёсток #4 Сампсониевский пр. / Б. Сампс. пр.");
    crossing4->setStyleSheet("color: #CC0000; font-weight: bold;");
    QLabel *status4 = new QLabel("Статус: Требует внимания");
    status4->setStyleSheet("color: #CC0000; font-size: 12px;");

    crossingsLayout->addWidget(crossing1, 0, 0);
    crossingsLayout->addWidget(status1, 0, 1);
    crossingsLayout->addWidget(crossing2, 1, 0);
    crossingsLayout->addWidget(status2, 1, 1);
    crossingsLayout->addWidget(crossing3, 2, 0);
    crossingsLayout->addWidget(status3, 2, 1);
    crossingsLayout->addWidget(crossing4, 3, 0);
    crossingsLayout->addWidget(status4, 3, 1);

    controlLayout->addLayout(crossingsLayout);

    controlGroupBox->setLayout(controlLayout);
    mainLayout->addWidget(controlGroupBox);

    // === Список объектов ===
    QGroupBox *crossingListGroupBox = new QGroupBox("Список объектов");
    crossingListGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QVBoxLayout *listLayout = new QVBoxLayout(crossingListGroupBox);

    m_crossingList = new QListWidget;
    m_crossingList->setStyleSheet("background-color: #303030; border: 1px solid #404040;");
    m_crossingList->addItem("1 Невский / Литейный");
    m_crossingList->addItem("2 Московский / Кузнечный");
    m_crossingList->addItem("3 Лиговский / Разъезжая");
    m_crossingList->addItem("4 Сампсониевский / Б. Сампс.");
    m_crossingList->addItem("5 Камская / Большой пр.");

    listLayout->addWidget(m_crossingList);
    crossingListGroupBox->setLayout(listLayout);
    mainLayout->addWidget(crossingListGroupBox);

    // === Фильтры ===
    QGroupBox *filtersGroupBox = new QGroupBox("Фильтры");
    filtersGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QGridLayout *filtersLayout = new QGridLayout(filtersGroupBox);

    filtersLayout->addWidget(new QLabel("Все районы"), 0, 0);
    filtersLayout->addWidget(new QLabel("Центральный район"), 0, 1);
    filtersLayout->addWidget(new QLabel("Адмиралтейский район"), 0, 2);
    filtersLayout->addWidget(new QLabel("Василеостровский район"), 0, 3);

    filtersLayout->addWidget(new QLabel("Все статусы"), 1, 0);
    filtersLayout->addWidget(new QLabel("Норма"), 1, 1);
    filtersLayout->addWidget(new QLabel("Требует внимания"), 1, 2);
    filtersLayout->addWidget(new QLabel("Неисправность"), 1, 3);

    filtersGroupBox->setLayout(filtersLayout);
    mainLayout->addWidget(filtersGroupBox);

    // === Параметры светофора ===
    QGroupBox *paramsGroupBox = new QGroupBox("Параметры светофора: Невский пр. / Литейный пр.");
    paramsGroupBox->setStyleSheet("QGroupBox { border: 1px solid #404040; border-radius: 4px; margin-top: 1ex; }");
    QVBoxLayout *paramsLayout = new QVBoxLayout(paramsGroupBox);

    // Режим работы
    QHBoxLayout *modeLayout = new QHBoxLayout;
    modeLayout->addWidget(new QLabel("Режим работы:"));
    m_modeComboBox = new QComboBox;
    m_modeComboBox->addItems({"Автоматический (рекомендуется)", "Ручной", "Ночной режим", "Режим ЧП"});
    modeLayout->addWidget(m_modeComboBox);
    paramsLayout->addLayout(modeLayout);

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
    QVBoxLayout *priorityLayout = new QVBoxLayout(m_priorityGroupBox);

    priorityLayout->addWidget(new QLabel("Невский пр. (север)"));
    priorityLayout->addWidget(new QLabel("Невский пр. (юг)"));
    priorityLayout->addWidget(new QLabel("Литейный пр. (запад)"));
    priorityLayout->addWidget(new QLabel("Литейный пр. (восток)"));

    m_priorityList = new QListWidget;
    m_priorityList->addItems({"Невский пр. (север)", "Невский пр. (юг)", "Литейный пр. (запад)", "Литейный пр. (восток)"});
    m_priorityList->setStyleSheet("background-color: #303030; border: 1px solid #404040;");
    priorityLayout->addWidget(m_priorityList);

    paramsLayout->addWidget(m_priorityGroupBox);

    // Кнопки управления
    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_applyButton = new QPushButton("Применить изменения");
    m_applyButton->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    m_resetButton = new QPushButton("Сбросить к автоматическому режиму");
    m_resetButton->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    m_diagnosticsButton = new QPushButton("Запустить диагностику");
    m_diagnosticsButton->setStyleSheet(
        "QPushButton { "
        "   background-color: #0077CC; "
        "   color: white; "
        "   border-radius: 4px; "
        "   padding: 8px 16px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #0066BB; }"
        );

    btnLayout->addWidget(m_applyButton);
    btnLayout->addWidget(m_resetButton);
    btnLayout->addWidget(m_diagnosticsButton);

    paramsLayout->addLayout(btnLayout);
    paramsGroupBox->setLayout(paramsLayout);
    mainLayout->addWidget(paramsGroupBox);

}
