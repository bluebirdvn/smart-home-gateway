find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(GDBUS REQUIRED gio-2.0 glib-2.0)
pkg_check_modules(CJSON REQUIRED libcjson)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(common STATIC ${SOURCES})

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${GDBUS_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
)


target_link_libraries(common PUBLIC ${GDBUS_LIBRARIES}
                        Threads::Threads ${CJSON_LIBRARIES})
target_link_directories(common PUBLIC ${GDBUS_LIBRARY_DIRS} ${CJSON_LIBRARY_DIRS})
target_compile_options(common PUBLIC ${GDBUS_FLAGS_OTHERS} ${CJSON_CFLAGS_OTHER})