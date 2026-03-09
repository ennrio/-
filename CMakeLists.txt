cmake_minimum_required(VERSION 3.19)
project(asudd LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(asudd
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    vehicle.h vehicle.cpp
    vehicleitem.h vehicleitem.cpp
    simulationview.h simulationview.cpp
    roadgraph.h roadgraph.cpp
    osmparser.h osmparser.cpp
    trafficlight.h trafficlight.cpp
    trafficlightcontroller.h trafficlightcontroller.cpp
    intersectioncontroller.h intersectioncontroller.cpp
    trafficlightitem.h trafficlightitem.cpp
)

target_link_libraries(asudd
    PRIVATE
        Qt::Core
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
