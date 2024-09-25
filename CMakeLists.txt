cmake_minimum_required(VERSION 3.5)

project(QMapViewSample)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HDRS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_AUTOUIC_SEARCH_PATHS 
    ${CMAKE_CURRENT_SOURCE_DIR}/forms
)

find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
add_executable(${CMAKE_PROJECT_NAME} ${SRCS} ${HDRS})
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets Qt6::Network)
