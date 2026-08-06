cmake_minimum_required(VERSION 3.16)

project(qt_gm VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network PrintSupport)

qt_add_executable(qt_gm
    main.cpp
    resources.qrc

    models/SalleGm.h

    engine/GmSource.h
    engine/demo/DemoGmSource.h
    engine/demo/DemoGmSource.cpp
    engine/mqtt/MqttClient.h
    engine/mqtt/MqttClient.cpp
    engine/mqtt/MqttGmSource.h
    engine/mqtt/MqttGmSource.cpp

    vendor/qcustomplot.h
    vendor/qcustomplot.cpp

    ui/PlotGm.h
    ui/PlotGm.cpp
    ui/SalleWindow.h
    ui/SalleWindow.cpp
    ui/MainWindow.h
    ui/MainWindow.cpp
)

target_link_libraries(qt_gm PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::PrintSupport
)

target_include_directories(qt_gm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
