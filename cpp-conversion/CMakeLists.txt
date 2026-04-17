cmake_minimum_required(VERSION 3.10)
project(FlowNodeEditor)

set(CMAKE_CXX_STANDARD 11)

include_directories(core graphics ui)

add_executable(FlowNodeEditor main.cpp)
target_sources(FlowNodeEditor PRIVATE core/FlowNode.cpp core/NodeGraph.cpp graphics/GraphicsRenderer.cpp ui/UIManager.cpp)