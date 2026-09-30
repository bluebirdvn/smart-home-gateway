pkg_check_modules(SQLITE3 REQUIRED sqlite3)

file(GLOB_RECURSE SOURCES "main.cpp" "src/*.cpp")

add_executable(local-database ${SOURCES})

target_include_directories(local-database PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/db 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repositories
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${SQLITE3_INCLUDE_DIRS}
    )

target_link_libraries(local-database PRIVATE common Threads::Threads ${SQLITE3_LIBRARIES})