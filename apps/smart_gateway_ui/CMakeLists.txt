cmake_minimum_required(VERSION 3.16)

project(smart_gateway_ui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Charts Widgets DBus)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(smart_gateway_ui
    main.cpp
)
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
#     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/gateway
)

qt_add_qml_module(smart_gateway_ui
    URI smart_gateway_ui
    QML_FILES
        Main.qml
        HomeTab.qml
        SettingTab.qml
        TaskbarTab.qml
        TopHeaderTab.qml
        LogTab.qml
        Chart.qml
        ChipItem.qml
        DeviceTab.qml
        Keypad.qml
        InfoTab.qml 
        MyButton.qml 
        MyField.qml 
        MySwitch.qml
    SOURCES
        devicemodel.h devicemodel.cpp
        config_manager.h
        dbus_event_adapter.h dbus_event_adapter.cpp 
        rpc.h rpc.cpp
        mesh_utils.h
        mesh_vendor_id.h
        log_model.h
        dbus.h dbus.cpp
        ipc.h
        ipc_message.h
        ipc_dto.h
        ipcevent.h ipcevent.cpp
        controller.h controller.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(smart_gateway_ui PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.smart_gateway_ui
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(smart_gateway_ui
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Charts
    PRIVATE Qt6::Widgets
    PRIVATE Qt6::DBus
)

include(GNUInstallDirs)
install(TARGETS smart_gateway_ui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
