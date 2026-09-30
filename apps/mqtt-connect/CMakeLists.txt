file(GLOB_RECURSE SOURCES "main.cpp" "src/*.cpp")

add_executable(mqtt-connect ${SOURCES})

target_include_directories(mqtt-connect PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(mqtt-connect PRIVATE common paho-mqtt3as Threads::Threads)