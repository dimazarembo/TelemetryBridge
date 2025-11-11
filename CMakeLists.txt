
cmake_minimum_required(VERSION 3.20)
project(TelemetryBridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Xml)

add_executable(TelemetryBridge
    src/main.cpp
    src/TelemetryManager.cpp
    include/TelemetryManager.h
    config.json
)

target_include_directories(TelemetryBridge PRIVATE include)

target_link_libraries(TelemetryBridge PRIVATE Qt6::Core Qt6::Network Qt6::Xml)

configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
