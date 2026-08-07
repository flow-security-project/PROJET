cmake_minimum_required(VERSION 3.16)

project(qt_dashboard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

qt_add_executable(qt_dashboard
    main.cpp
    resources.qrc

    data/Salle.h
    data/Alerte.h
    data/DataSource.h
    data/DataSource.cpp
    data/DemoSource.h
    data/DemoSource.cpp
    data/MqttClient.h
    data/MqttClient.cpp
    data/MqttSource.h
    data/MqttSource.cpp

    widgets/Couleurs.h
    widgets/PlotWidget.h
    widgets/PlotWidget.cpp
    widgets/BarreSeuil.h
    widgets/BarreSeuil.cpp
    widgets/LcdMirror.h
    widgets/LcdMirror.cpp

    ui/StatusBar.h
    ui/StatusBar.cpp
    ui/SalleGrid.h
    ui/SalleGrid.cpp
    ui/VisualisationTab.h
    ui/VisualisationTab.cpp
    ui/ConfigurationTab.h
    ui/ConfigurationTab.cpp
    ui/AlertesTab.h
    ui/AlertesTab.cpp
    ui/AlertPanel.h
    ui/AlertPanel.cpp
    ui/SalleDetail.h
    ui/SalleDetail.cpp

    ui/MainWindow.h
    ui/MainWindow.cpp
)

target_link_libraries(qt_dashboard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

target_include_directories(qt_dashboard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
