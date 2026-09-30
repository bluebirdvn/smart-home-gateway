cmake_minimum_required(VERSION 3.16)
project(SmartHomeGateway VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)


add_subdirectory(common)
add_subdirectory(gateway-daemon)
add_subdirectory(local-database)
add_subdirectory(mqtt-connect)
add_subdirectory(smart_gateway_ui)