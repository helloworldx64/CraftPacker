cmake_minimum_required(VERSION 3.16)
project(CraftPacker VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(CraftPacker
    CraftPacker.h
    CraftPacker.cpp
)

# This line tells the linker to create a Windows GUI application.
set_property(TARGET CraftPacker PROPERTY WIN32_EXECUTABLE TRUE)

target_link_libraries(CraftPacker PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

install(TARGETS CraftPacker
    RUNTIME DESTINATION bin
)
