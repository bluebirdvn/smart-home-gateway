cmake_minimum_required(VERSION 3.16)
project(SmartHomeGateway VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

option(BUILD_GATEWAY  "Build Gateway Daemon" OFF)
option(BUILD_DATABASE "Build Local Database" OFF)
option(BUILD_MQTT "Build MQTT Connect" OFF)
option(BUILD_UI "Build Qt UI" OFF)

if(BUILD_GATEWAY)
    add_subdirectory(gateway-app)
endif()

if(BUILD_DATABASE)
    add_subdirectory(local-database)
endif()

if(BUILD_MQTT)
    add_subdirectory(mqtt-connect)
endif()

if(BUILD_UI)
    add_subdirectory(smart_gateway_ui)
endif()