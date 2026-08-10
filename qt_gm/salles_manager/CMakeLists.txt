cmake_minimum_required(VERSION 3.16)

project(salles_manager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network PrintSupport)

qt_add_executable(salles_manager
    main.cpp
    resources.qrc

    models/Salle.h
    models/Alerte.h
    models/AlerteModel.h

    engine/densite/DensiteEstimator.h
    engine/densite/DensiteEstimator.cpp

    data/DataSource.h
    data/DataSource.cpp
    data/DemoSource.h
    data/DemoSource.cpp
    data/MqttClient.h
    data/MqttClient.cpp
    data/MqttSource.h
    data/MqttSource.cpp

    ../vendor/qcustomplot.h
    ../vendor/qcustomplot.cpp
    ui/IntegratedPlotWidget.h
    ui/IntegratedPlotWidget.cpp
    ui/JaugeSaturation.h
    ui/JaugeSaturation.cpp
    ui/SalleDetailWidget.h
    ui/SalleDetailWidget.cpp
    ui/SalleGrid.h
    ui/SalleGrid.cpp
    ui/AlertePanelWidget.h
    ui/AlertePanelWidget.cpp
    ui/SalleConfigWidget.h
    ui/SalleConfigWidget.cpp
    ui/SallesWidget.h
    ui/SallesWidget.cpp
    ui/MainWindow.h
    ui/MainWindow.cpp
)

target_link_libraries(salles_manager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::PrintSupport
)

target_include_directories(salles_manager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
