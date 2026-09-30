pkg_check_modules(GDBUS REQUIRED gio-2.0 glib-2.0)

find_package(Threads REQUIRED)
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(common STATIC ${SOURCES})

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${GDBUS_INCLUDE_DIRS}
)


target_link_libraries(common PUBLIC ${GDBUS_LIBRARIES}
                        Threads::Threads)
target_link_directories(common PUBLIC ${GDBUS_LIBRARY_DIRS})
target_compile_options(common PUBLIC ${GDBUS_FLAGS_OTHERS})