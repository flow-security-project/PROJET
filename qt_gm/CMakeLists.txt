cmake_minimum_required(VERSION 3.16)

project(i_plus_plus VERSION 0.1 LANGUAGES CXX)

option(BUILD_LEGACY_QT_GM "Build the archived qt_gm application" OFF)

add_subdirectory(salles_manager)

if(BUILD_LEGACY_QT_GM)
    add_subdirectory(legacy/qt_gm)
endif()
