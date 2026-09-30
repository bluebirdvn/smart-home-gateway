file(GLOB_RECURSE SOURCES "main.cpp" "src/*.cpp")

add_executable(gateway-daemon ${SOURCES})

target_include_directories(gateway-daemon PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mesh
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport
    )
target_link_libraries(gateway-daemon PRIVATE common Threads::Threads)