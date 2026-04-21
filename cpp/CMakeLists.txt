cmake_minimum_required(VERSION 3.16)
project(FlowNodeEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(FlowNodeEditor
    src/main.cpp
    src/MainWindow.cpp
    src/GraphicsScene.cpp
    src/GraphicsView.cpp
    src/EditorScene.cpp
    src/GraphicsNode.cpp
    src/GraphicsSocket.cpp
    src/GraphicsEdge.cpp
    src/EdgeDragging.cpp
    src/EdgeValidators.cpp
    include/MainWindow.hpp
    include/GraphicsScene.hpp
    include/GraphicsView.hpp
    include/EdgeDragging.hpp
)

target_include_directories(FlowNodeEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(FlowNodeEditor PRIVATE Qt6::Widgets)

if(MSVC)
    target_compile_definitions(FlowNodeEditor PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
