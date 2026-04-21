cmake_minimum_required(VERSION 3.16)
project(FlowNodeEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(FlowNodeEditor
    src/main.cpp
    src/NodeTypeRegistry.cpp
    src/NodeTypeRegistry.h
    src/FlowSocketItem.cpp
    src/FlowSocketItem.h
    src/FlowNodeItem.cpp
    src/FlowNodeItem.h
    src/FlowEdgeItem.cpp
    src/FlowEdgeItem.h
    src/FlowEditorScene.cpp
    src/FlowEditorScene.h
    src/FlowGraphicsView.cpp
    src/FlowGraphicsView.h
    src/FlowEditorWidget.cpp
    src/FlowEditorWidget.h
    src/MainWindow.cpp
    src/MainWindow.h
)

target_include_directories(FlowNodeEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(FlowNodeEditor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set(_flow_assets "${CMAKE_SOURCE_DIR}/apps/execution_node_editor/assets")
set(_flow_defs "${CMAKE_SOURCE_DIR}/apps/execution_node_editor/execution_subsystem/node_type_definitions")

add_custom_command(TARGET FlowNodeEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:FlowNodeEditor>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${_flow_assets}" "$<TARGET_FILE_DIR:FlowNodeEditor>/assets"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:FlowNodeEditor>/execution_subsystem/node_type_definitions"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:FlowNodeEditor>/execution_subsystem"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${_flow_defs}" "$<TARGET_FILE_DIR:FlowNodeEditor>/execution_subsystem/node_type_definitions"
    COMMENT "Sync assets and node definitions next to FlowNodeEditor executable"
)
