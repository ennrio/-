cmake_minimum_required(VERSION 3.19)
project(asudd LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Concurrent Widgets)


qt_standard_project_setup()

qt_add_executable(asudd
    WIN32 MACOSX_BUNDLE
    mainwindow.h
    mainwindow.ui
    main.cpp
    mainwindow.cpp
    visualization/vehicle.h visualization/vehicle.cpp
    visualization/vehicleitem.h visualization/vehicleitem.cpp
    visualization/simulationview.h visualization/simulationview.cpp
    visualization/roadgraph.h visualization/roadgraph.cpp
    visualization/osmparser.h visualization/osmparser.cpp
    visualization/trafficlight.h visualization/trafficlight.cpp
    visualization/trafficlightcontroller.h visualization/trafficlightcontroller.cpp
    visualization/intersectioncontroller.h visualization/intersectioncontroller.cpp
    visualization/trafficlightitem.h visualization/trafficlightitem.cpp
    mainscreenwidget.h mainscreenwidget.cpp
    trafficlightcontrolwidget.h trafficlightcontrolwidget.cpp
    datageneratorwidget.h datageneratorwidget.cpp
    analyticswidget.h analyticswidget.cpp
)

target_link_libraries(asudd
    PRIVATE
        Qt::Core
        Qt6::Concurrent
        Qt::Widgets
)

include(GNUInstallDirs)

install(TARGETS asudd
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET asudd
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
