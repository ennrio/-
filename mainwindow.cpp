#include "mainwindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("АСУДД Санкт-Петербург");
    resize(1400, 900);

    // Центральный виджет — переключатель экранов
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Создаём экраны
    m_mainScreen = new MainScreenWidget(this);
    m_stackedWidget->addWidget(m_mainScreen);

    m_trafficlightControllerWidget = new TrafficLightControlWidget(this);
    m_stackedWidget->addWidget(m_trafficlightControllerWidget);

    m_datageneratorwidget = new DataGeneratorWidget(this);
    m_stackedWidget->addWidget(m_datageneratorwidget);

    m_analyticsWidget = new AnalyticsWidget(this);
    m_stackedWidget->addWidget(m_analyticsWidget);

    // Меню навигации (как в макете index.html)
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *menuSystem = menuBar->addMenu("Меню системы");
    QAction *actMain = menuSystem->addAction("Г Главный экран");
    QAction *actLights = menuSystem->addAction("У Управление светофорами");
    QAction *actData = menuSystem->addAction("Д Генерация данных");
    QAction *actAnalytics = menuSystem->addAction("А Аналитика и отчёты");

    connect(actMain, &QAction::triggered, this, &MainWindow::showMainScreen);
    connect(actLights, &QAction::triggered, this, &MainWindow::showTrafficLights);
    connect(actData, &QAction::triggered, this, &MainWindow::showDataGenerator);
    connect(actAnalytics, &QAction::triggered, this, &MainWindow::showAnalytics);

    // Показываем главный экран по умолчанию
    showMainScreen();
}

void MainWindow::showMainScreen()
{
    m_stackedWidget->setCurrentWidget(m_mainScreen);
    setWindowTitle("АСУДД Санкт-Петербург - Главный экран");
}

void MainWindow::showTrafficLights() {
    m_stackedWidget->setCurrentWidget(m_trafficlightControllerWidget);
    setWindowTitle("АСУДД Санкт-Петербург - Управление светофорами");
}
void MainWindow::showDataGenerator() {
    m_stackedWidget->setCurrentWidget(m_datageneratorwidget);
    setWindowTitle("АСУДД Санкт-Петербург - Генерация тестовых данных");
}
void MainWindow::showAnalytics() {
    m_stackedWidget->setCurrentWidget(m_analyticsWidget);
    setWindowTitle("АСУДД Санкт-Петербург - Аналитика и отчёты");
}
