cmake_minimum_required(VERSION 3.5)

project(QMapViewSample)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/MapViewCore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/MapView.h

    ${CMAKE_CURRENT_SOURCE_DIR}/include/Web/TMSLayer.h

    ${CMAKE_CURRENT_SOURCE_DIR}/include/MainWindow.h    # EXAMPLE
)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/Web)

set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MapViewCore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MapView.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/Web/TMSLayer.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.cpp      # EXAMPLE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp            # EXAMPLE
)

set(CMAKE_AUTOUIC_SEARCH_PATHS 
    ${CMAKE_CURRENT_SOURCE_DIR}/forms
)

find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
add_executable(${CMAKE_PROJECT_NAME} ${SRCS} ${HDRS})
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets Qt6::Network)
